"""AArch64 instruction emulation for SML handoff and U-Boot native boot flow."""
import gzip
from pathlib import Path
from functools import lru_cache
import struct
import zlib
import unicorn.arm64_const as arm64
from capstone import Cs, CS_ARCH_ARM64, CS_MODE_ARM
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn.arm64_const import UC_ARM64_REG_X0, UC_ARM64_REG_X18, UC_ARM64_REG_X30, UC_ARM64_REG_SP, UC_ARM64_REG_PC, UC_ARM64_REG_PSTATE

def section(elf, name):
    # U-Boot 2015 discards .dynsym but leaves an unused orphan .hash section.
    # Enumerate names without asking pyelftools to instantiate that orphan.
    for index in range(elf.num_sections()):
        header=elf._get_section_header(index)
        if elf._get_section_name(header)==name:
            return elf.get_section(index)
    raise ValueError('Missing ELF section '+name)


def gd_layout(elf):
    elf._section_name_map={elf._get_section_name(elf._get_section_header(i)):i for i in range(elf.num_sections())}
    for cu in elf.get_dwarf_info(relocate_dwarf_sections=False).iter_CUs():
        name=cu.get_top_DIE().attributes.get('DW_AT_name')
        if not name or b'sy108/native.c' not in name.value:continue
        types={}
        for die in cu.iter_DIEs():
            name=die.attributes.get('DW_AT_name')
            if die.tag=='DW_TAG_structure_type' and name and name.value in (b'global_data',b'arch_global_data'):
                fields={c.attributes['DW_AT_name'].value.decode():c.attributes['DW_AT_data_member_location'].value for c in die.iter_children() if c.tag=='DW_TAG_member'}
                types[name.value]=(die.attributes['DW_AT_byte_size'].value,fields)
        size,g=types[b'global_data'];_,a=types[b'arch_global_data']
        return [size]+[g['arch']+a[n] for n in ['sy108_mode','sy108_stage','sy108_pending','sy108_valid','sy108_error','sy108_frequency']]+[g[n] for n in ['relocaddr','reloc_off','start_addr_sp','bd','fdt_blob','mon_len','ram_size']]
    raise ValueError('Missing native GD DWARF type')


BASE=0x9f000000
STOP=0x8f000000


@lru_cache(maxsize=2)
def elf_metadata(path,mtime,size):
    with Path(path).open('rb') as f:
        elf=ELFFile(f)
        symbols={s.name:int(s['st_value']) for s in section(elf,'.symtab').iter_symbols()}
        segments=[(s['p_vaddr'],s.data()) for s in elf.iter_segments() if s['p_type']=='PT_LOAD']
        text=section(elf,'.text')
        return symbols,segments,(int(text['sh_addr']),text.data()),gd_layout(elf)


@lru_cache(maxsize=2)
def system_operations(address,data):
    md=Cs(CS_ARCH_ARM64,CS_MODE_ARM);md.skipdata=True
    return [(i.address,i.mnemonic,i.op_str) for i in md.disasm(data,address)
            if i.mnemonic in ['tlbi','ic','dc','msr','mrs']]


class Machine:
    def __init__(self, out, envelope, level=8):
        self.uc=Uc(UC_ARCH_ARM64,UC_MODE_ARM)
        self.uc.mem_map(0x80000000,0x0a000000)
        self.uc.mem_map(0x9e000000,0x02000000)
        self.uc.mem_map(STOP,4096)
        self.uc.mem_map(0x63a00000,0x40000)
        path=out/'uboot/u-boot';stat=path.stat()
        self.syms,segments,self.text,self.layout=elf_metadata(str(path),stat.st_mtime_ns,stat.st_size)
        for address,data in segments:self.uc.mem_write(address,data)
        self.uc.mem_write(BASE-512,envelope[:0xc0000])
        self.gd=0x9e100000;self.heap=0x88000000
        self.uc.reg_write(UC_ARM64_REG_X18,self.gd)
        self.uc.reg_write(UC_ARM64_REG_SP,0x9e200000)
        self.uc.reg_write(UC_ARM64_REG_X30,STOP)
        self.events=[];self.result=None;self.level=level;self.ticks=0;self.control_registers={}
        self.uc.reg_write(UC_ARM64_REG_PSTATE,0x3c0|level|1)
        self.pm={0x1808:6,0x1810:6,0x250:0xb45a,0x234:0,0x48:14,0x4c:0,0x50:0,0x60:0,0x1bac:0xf0,
                 0x1bb8:0,0x1bec:0,0x1bb0:0,0x1be4:0,0x1be8:0}
        self.command=0;self.mode=0;self.adi_fault=None
        self.hook_address(STOP,lambda: self.uc.emu_stop())
        for name in ['printf','puts','putc','serial_puts','serial_putc']:
            self.hook(name,lambda:self.ret(0))
        self.hook('malloc',self.allocate)
        self.hook('free',lambda:self.ret(0))
        self.hook('setenv',lambda:self.ret(0))
        self.hook('udx_crc32',lambda:self.ret(zlib.crc32(self.read(self.x(0),self.x(1)))))
        self.hook('gunzip',self.inflate)
        self.fixup=lambda:self.ret(0)
        self.hook('fdt_fixup_all',lambda:self.fixup())
        self.hook('_boot_load_required_image',self.firmware)
        self.hook('modem_entry',lambda:self.event_ret('audcp_start',0))
        self.hook('usb_driver_exit',lambda:self.event_ret('usb_exit',0))
        self.hook('smp_kick_all_cpus',lambda:self.event_ret('secondary_CPU_kick',0))
        self.hook('disable_interrupts',self.mask_interrupts)
        self.hook('cleanup_before_linux',self.cleanup)
        self.hook('armv8_switch_to_el2',self.switch_level)
        self.hook('sci_adi_read',self.legacy_adi_read)
        self.hook('sci_adi_write',self.legacy_adi_write)
        self.hook('sci_adi_write_fast',self.legacy_adi_fast_write)
        self.hook('hw_watchdog_rst_pending',self.consume_pending)
        for name in ['is_7s_reset','is_7s_reset_for_systemdump','is_hw_smpl_enable','is_smpl_bootup']:
            self.hook(name,lambda:self.ret(0))
        self.hook('reboot_devices',self.reset)
        self.hook('lowlevel_init',lambda:self.event_ret('GIC_init',0))
        self.uc.hook_add(UC_HOOK_MEM_READ,self.adi_read,begin=0x63a00000,end=0x63a3ffff)
        self.uc.hook_add(UC_HOOK_MEM_WRITE,self.adi_write,begin=0x63a00000,end=0x63a3ffff)
        self.system_instructions()

    def x(self,n):return self.uc.reg_read(getattr(arm64,'UC_ARM64_REG_X'+str(n)))
    def clear_bss(self):
        begin=self.syms['__bss_start'];end=self.syms['__bss_end']
        self.uc.mem_write(begin,bytes(end-begin))
    def read(self,address,size):return bytes(self.uc.mem_read(address,size))
    def put(self,address,value,size=8):self.uc.mem_write(address,int(value).to_bytes(size,'little',signed=False))
    def ret(self,value=0):
        self.uc.reg_write(UC_ARM64_REG_X0,value&((1<<64)-1))
        self.uc.reg_write(UC_ARM64_REG_PC,self.uc.reg_read(UC_ARM64_REG_X30))
    def hook_address(self,address,fn):
        self.uc.hook_add(UC_HOOK_CODE,lambda uc,a,s,u:fn() if uc.reg_read(UC_ARM64_REG_PC)==a else None,begin=address,end=address)
    def hook(self,name,fn,delta=0):
        if self.syms.get(name):self.hook_address(self.syms[name]+delta,fn)
    def event_ret(self,name,value):self.events.append(name);self.ret(value)
    def mask_interrupts(self):
        self.uc.reg_write(UC_ARM64_REG_PSTATE,self.uc.reg_read(UC_ARM64_REG_PSTATE)|0x3c0)
        self.event_ret('interrupts_masked',1)
    def cleanup(self):
        for el in (1,2,3):
            name='sctlr_el'+str(el)
            self.control_registers[name]=self.control_registers.get(name,0)&~0x1005
        self.event_ret('cache_mmu_disabled',0)
    def legacy_adi_read(self):
        reg=self.x(0)-0x63a20000
        assert reg in self.pm,hex(reg)
        self.ret(self.pm[reg])
    def legacy_adi_write(self):
        reg=self.x(0)-0x63a20000
        assert reg in self.pm,hex(reg)
        self.pm[reg]=((self.pm[reg]&~self.x(2))|self.x(1))&0xffff
        self.ret(0)
    def legacy_adi_fast_write(self):
        reg=self.x(0)-0x63a20000
        assert reg in self.pm,hex(reg)
        self.pm[reg]=self.x(1)&0xffff
        if reg==0x4c:self.pm[0x50]&=~self.x(1)
        self.ret(0)
    def consume_pending(self):
        self.pm[0x50]=0
        self.event_ret('live_WDT_flags_consumed',0)
    def allocate(self):
        ptr=self.heap;self.heap+=(self.x(0)+31)&~15
        assert self.heap<0x89f00000
        self.ret(ptr)
    def inflate(self):
        n=int.from_bytes(self.read(self.x(3),8),'little')
        try:data=gzip.decompress(self.read(self.x(2),n))
        except (gzip.BadGzipFile,EOFError,zlib.error):self.ret(-1);return
        assert len(data)<=self.x(1)
        self.uc.mem_write(self.x(0),data);self.put(self.x(3),len(data));self.ret(0)
    def firmware(self):
        ptr=int.from_bytes(self.read(self.x(0),8),'little')
        self.events.append('firmware:'+self.string(ptr));self.ret(1)
    def string(self,address):
        return self.read(address,256).split(b'\0',1)[0].decode()
    def switch_level(self):
        if self.level==12:self.level=8
        self.event_ret('EL_transition',0)
    def reset(self):
        self.result={'reset_mode':self.x(0),'stage':self.pm.get(0x250)}
        self.uc.emu_stop()
    def adi_read(self,uc,access,address,size,value,user):
        if address==0x63a00030:v=0x800 if self.adi_fault=="fifo-full" else 0x400
        elif address==0x63a0002c:
            v=(((self.command&0x1fffc)>>2)<<16)|self.pm.get(self.command-0x63a20000,0)
            if self.adi_fault=="read-busy":v|=1<<31
            elif self.adi_fault=="read-echo":v^=1<<16
        else:raise AssertionError('unexpected ADI read '+hex(address))
        self.put(address,v,4)
    def adi_write(self,uc,access,address,size,value,user):
        if address==0x63a00028:self.command=value;return
        reg=address-0x63a20000
        assert reg in [0x40,0x44,0x48,0x60,0x1808,0x1810,0x238,0x254,0x1bac],hex(address)
        self.pm[reg]=value
        if reg==0x238:self.pm[0x234]&=~value
        if reg==0x254:
            self.pm[0x250]=value^(0x200 if self.adi_fault=="rtc-mismatch" else 0)
            if self.adi_fault!="rtc-no-ack":self.pm[0x234]|=0x80
    def system_instructions(self,delta=0):
        for address,mnemonic,operands in system_operations(*self.text):
            if mnemonic in ['tlbi','ic'] or (mnemonic=='dc' and not operands.startswith('zva')):
                self.hook_address(address+delta,lambda:self.uc.reg_write(UC_ARM64_REG_PC,self.uc.reg_read(UC_ARM64_REG_PC)+4))
                continue
            if mnemonic=='msr':
                register,operand=[v.strip().lower() for v in operands.split(',')]
                if register not in ['vbar_el2','vbar_el3','cptr_el2','cptr_el3','scr_el3','cntfrq_el0','sctlr_el1','sctlr_el2','sctlr_el3']:continue
                def write_control(register=register,operand=operand):
                    import unicorn.arm64_const as regs
                    value=0 if operand=='xzr' else self.uc.reg_read(getattr(regs,'UC_ARM64_REG_'+operand.upper()))
                    self.control_registers[register]=value
                    self.uc.reg_write(UC_ARM64_REG_PC,self.uc.reg_read(UC_ARM64_REG_PC)+4)
                self.hook_address(address+delta,write_control)
                continue
            if mnemonic!='mrs':continue
            dest,source=[v.strip().lower() for v in operands.split(',')]
            if source not in ['currentel','cntfrq_el0','cntpct_el0','cntvct_el0','sctlr_el1','sctlr_el2','sctlr_el3','scr_el3']:continue
            reg=int(dest[1:])
            def intercept(reg=reg,source=source):
                self.ticks+=1000
                value=self.control_registers.get(source,0) if source.startswith(('sctlr','scr_')) else self.level if source=='currentel' else 24000000 if source=='cntfrq_el0' else self.ticks
                self.uc.reg_write(getattr(arm64,'UC_ARM64_REG_X'+str(reg)),value)
                self.uc.reg_write(UC_ARM64_REG_PC,self.uc.reg_read(UC_ARM64_REG_PC)+4)
            self.hook_address(address+delta,intercept)
    def run(self,address,*args,count=100000000):
        for n,v in enumerate(args):self.uc.reg_write(getattr(arm64,'UC_ARM64_REG_X'+str(n)),v)
        self.uc.reg_write(UC_ARM64_REG_X30,STOP)
        try:
            self.uc.emu_start(address,STOP,timeout=60000000,count=count)
        except Exception as error:
            pc=self.uc.reg_read(UC_ARM64_REG_PC)
            md=Cs(CS_ARCH_ARM64,CS_MODE_ARM)
            details=[i.mnemonic+' '+i.op_str for i in md.disasm(self.read(pc,12),pc)]
            raise RuntimeError(str(error)+' at '+hex(pc)+' '+str(details)) from error
        pc=self.uc.reg_read(UC_ARM64_REG_PC)
        assert self.result is not None or pc==STOP,('instruction budget or unsupported handoff',hex(pc))


def verify_instructions(out,inputs,packages,fdt_parser=None):
    envelope=(out/'uboot.bin').read_bytes()
    entry=Machine(out,envelope)
    entry.hook('board_init_f',lambda:(setattr(entry,'result',{'board_init_f':True}),entry.uc.emu_stop()))
    entry.run(entry.syms['_start'])
    assert entry.result=={'board_init_f':True}
    gd=entry.uc.reg_read(UC_ARM64_REG_X18);sp=entry.uc.reg_read(UC_ARM64_REG_SP)
    assert entry.x(0)==0 and sp%16==0 and entry.read(gd,entry.layout[0])==bytes(entry.layout[0])

    smlsize=int.from_bytes(inputs['sml'][48:52],'little');sml=inputs['sml'][512:512+smlsize]
    md=Cs(CS_ARCH_ARM64,CS_MODE_ARM);md.skipdata=True
    ins=list(md.disasm(sml,0x85300000));resolver=None
    for a,b in zip(ins,ins[1:]):
        if a.mnemonic=='mov' and a.op_str=='x0, #0x9f000000' and b.mnemonic=='ret':resolver=a.address;break
    assert resolver is not None
    m=Machine(out,envelope);m.uc.mem_write(0x85300000,sml);m.run(resolver)
    assert m.x(0)==BASE

    handoff_start=next(a.address for a,b,c in zip(ins,ins[1:],ins[2:])
                       if (a.mnemonic,a.op_str,b.mnemonic,b.op_str,c.mnemonic,c.op_str)==
                       ('mov','x17, sp','msr','spsel, #1','str','x17, [sp, #0x108]'))
    handoff=Machine(out,envelope,level=12);handoff.uc.mem_write(0x85300000,sml)
    frame=0x9e200000;registers=[0x110000000+n for n in range(31)]
    for n,value in enumerate(registers):handoff.put(frame+n*8,value)
    handoff.put(frame+0xf8,0x9e300000)
    handoff.put(frame+0x100,0x501)
    handoff.put(frame+0x110,0x3c9);handoff.put(frame+0x118,BASE)
    return_end=None
    for i in ins:
        if not handoff_start<=i.address<=handoff_start+64:continue
        if i.mnemonic=='msr':
            name,operand=[v.strip() for v in i.op_str.split(',')]
            def context_write(name=name,operand=operand):
                handoff.control_registers[name]=int(operand[1:]) if operand.startswith('#') else handoff.x(int(operand[1:]))
                handoff.uc.reg_write(UC_ARM64_REG_PC,handoff.uc.reg_read(UC_ARM64_REG_PC)+4)
            handoff.hook_address(i.address,context_write)
    restore=next(a.address for a,b in zip(ins,ins[1:])
                 if a.mnemonic=='ldp' and a.op_str=='x0, x1, [sp]' and b.op_str=='x2, x3, [sp, #0x10]')
    for i in ins:
        if restore<=i.address<handoff_start:
            if i.mnemonic=='msr' and i.op_str=='sp_el0, x17':
                def set_sp_el0():
                    handoff.control_registers['sp_el0']=handoff.x(17)
                    handoff.uc.reg_write(UC_ARM64_REG_PC,handoff.uc.reg_read(UC_ARM64_REG_PC)+4)
                handoff.hook_address(i.address,set_sp_el0)
            if i.mnemonic=='eret':return_end=i.address
    assert return_end is not None
    restored={}
    def exception_return():
        assert [handoff.x(n) for n in range(31)]==registers,[(n,hex(handoff.x(n)),hex(v)) for n,v in enumerate(registers) if handoff.x(n)!=v]
        assert handoff.control_registers['elr_el3']==BASE
        assert handoff.control_registers['spsr_el3']==0x3c9
        assert handoff.control_registers['scr_el3']==0x501
        restored.update(registers_restored=31,target=BASE,
                        supplied_SPSR_EL3=0x3c9,supplied_SCR_EL3=0x501)
        handoff.level=8
        handoff.uc.reg_write(UC_ARM64_REG_PSTATE,0x3c9)
        handoff.uc.reg_write(UC_ARM64_REG_PC,BASE)
    handoff.hook_address(return_end,exception_return)
    handoff.hook('board_init_f',lambda:(setattr(handoff,'result',{'SML_to_Uboot':True}),handoff.uc.emu_stop()))
    handoff.run(handoff_start)
    assert handoff.result=={'SML_to_Uboot':True} and restored['target']==BASE
    restored.update(fragment_start=hex(handoff_start),fragment_eret=hex(return_end),
                    original_fragment_SHA256=__import__('hashlib').sha256(sml[restore-0x85300000:handoff_start+36-0x85300000]).hexdigest())

    relocation=Machine(out,envelope);layout=relocation.layout;allocation={};platform=[]
    relocation.hook('getenv_ulong',lambda:relocation.ret(relocation.x(2)))
    init=relocation.syms['init_sequence']
    for offset in range(0,256,8):
        address=int.from_bytes(relocation.read(init+offset,8),'little')
        if not address:break
        name=next(n for n,a in relocation.syms.items() if a==address)
        if name in ['sy108_boot_early','fdtdec_check_fdt']:continue
        platform.append(name)
        if name=='dram_init':
            def dram():
                gd=relocation.uc.reg_read(UC_ARM64_REG_X18)
                relocation.put(gd+layout[13],0x20000000);relocation.ret(0)
            relocation.hook_address(address,dram)
        else:relocation.hook_address(address,lambda:relocation.ret(0))
    relocation.hook('dram_init_banksize',lambda:relocation.ret(0));platform.append('dram_init_banksize')
    md=Cs(CS_ARCH_ARM64,CS_MODE_ARM)
    returnsite=next(i.address+4 for i in md.disasm(relocation.read(relocation.syms['_main'],256),relocation.syms['_main'])
                    if i.mnemonic=='bl' and int(i.op_str.lstrip('#'),16)==relocation.syms['board_init_f'])
    original_dtb=(out/'uboot/u-boot.dtb').read_bytes()
    def after_board_init_f():
        old=relocation.uc.reg_read(UC_ARM64_REG_X18)
        get=lambda index:int.from_bytes(relocation.read(old+layout[index],8),'little')
        dest=get(7);delta=dest-BASE;newgd=get(10)-layout[0];dt=get(11)
        assert relocation.read(dt,len(original_dtb))==original_dtb
        assert dt!=relocation.syms['_end'] and relocation.read(newgd,layout[0])==relocation.read(old,layout[0])
        bs=relocation.syms['__bss_start']+delta;be=relocation.syms['__bss_end']+delta
        assert not (dt<be and bs<dt+len(original_dtb))
        relocation.uc.mem_write(bs,b'\xa5'*(be-bs))
        allocation.update(destination=dest,global_data=newgd,stack=get(9),DTB=dt,BSS_start=bs,BSS_end=be,
                          RAM_bytes=get(13),relocation_offset=delta,code_copy_needed=bool(delta),
                          DTB_copied_before_BSS_clear=True)
        relocation.hook('board_init_r',lambda:(setattr(relocation,'result',{'relocated':True}),relocation.uc.emu_stop()),delta)
        relocation.system_instructions(delta)
    relocation.hook_address(returnsite,after_board_init_f)
    relocation.run(relocation.syms['_start'])
    assert relocation.result=={'relocated':True} and relocation.x(0)==allocation['global_data'] and relocation.x(1)==allocation['destination']
    bs=allocation['BSS_start'];be=allocation['BSS_end']
    assert relocation.read(bs,be-bs)==bytes(be-bs)
    assert relocation.read(allocation['DTB'],len(original_dtb))==original_dtb

    early=Machine(out,envelope);early.pm[0x250]=0xbe5a;early.pm[0x50]=8;early.pm[0x1bac]=0x40
    def premature_decoder():raise AssertionError('legacy ADI/BSS decoder called before initialization')
    forbidden=early.uc.hook_add(UC_HOOK_CODE,lambda uc,a,s,u:premature_decoder(),
                               begin=early.syms['check_reboot_mode'],end=early.syms['check_reboot_mode'])
    early.run(early.syms['sy108_boot_early'])
    early.uc.hook_del(forbidden)
    assert early.pm[0x250]==0xb45a and early.pm[0x48]==14 and early.pm[0x44]==30 and early.pm[0x1bac]==0xf0
    assert early.read(early.gd+early.layout[2],2)==b'\x5a\xbe'
    assert early.read(early.gd+early.layout[3],1)==b'\x01'
    early.clear_bss()
    early.run(early.syms['sy108_boot_late'])
    assert int.from_bytes(early.read(early.gd+early.layout[1],4),'little')==2
    assert early.pm[0x50]==0 and early.pm[0x1bac]==0xf0 and early.pm[0x250]==0xb45a

    faults=[]
    for fault,expected_error in [("fifo-full",-3),("read-busy",-3),("read-echo",-4),
                                 ("rtc-no-ack",-11),("rtc-mismatch",-12)]:
        m=Machine(out,envelope);m.adi_fault=fault
        m.run(m.syms['sy108_boot_early'])
        error=int.from_bytes(m.read(m.gd+m.layout[5],4),'little',signed=True)
        assert error==expected_error,(fault,error)
        faults.append({'fault':fault,'error':error})

    download=Machine(out,envelope)
    download.uc.mem_write(0x82000000,struct.pack('<II',0x43454e56,0x102))
    unchanged=dict(download.pm)
    download.run(download.syms['sy108_boot_early'])
    assert download.pm==unchanged and download.read(download.gd+download.layout[4],1)==b'\0'

    dispatch=[]
    for name,mode,legacy in [('normal',2,False),('fastboot',4,True),('U2S_autodloader',18,True)]:
        m=Machine(out,envelope)
        m.clear_bss()
        m.put(m.gd+m.layout[1],mode,4);m.put(m.gd+m.layout[4],1,1)
        strings=0x9e300100;argv=0x9e300000
        m.uc.mem_write(strings,b'cboot\0normal\0');m.put(argv,strings);m.put(argv+8,strings+6)
        m.hook('boot_pwr_check',lambda:m.ret(0));m.hook('write_log',lambda:m.ret(0))
        def native_dispatch():
            m.result={'native':m.x(0)};m.uc.emu_stop()
        def legacy_dispatch():
            m.result={'legacy':mode};m.uc.emu_stop()
        m.hook('sy108_native_boot',native_dispatch)
        m.hook('fastboot_mode' if mode==4 else 'autodloader_mode',legacy_dispatch)
        m.run(m.syms['do_cboot'],0,0,2,argv)
        assert ('legacy' in m.result)==legacy,(name,m.result)
        assert bool(m.pm[0x48]&2)!=legacy,(name,m.pm)
        dispatch.append({'name':name,'default_command':'cboot normal','result':m.result,
                         'PMIC_counter_enabled':bool(m.pm[0x48]&2)})

    uart=Machine(out,envelope);uart.clear_bss()
    uart.put(uart.gd+uart.layout[4],1,1)
    uart.put(uart.syms['stored_bootdelay'],0,4)
    uart.uc.mem_write(0x9e300000,b'cboot normal\0')
    uart.hook('tstc',lambda:uart.ret(1))
    uart.hook('getc',lambda:uart.event_ret('UART_input_consumed',32))
    def unexpected_command():raise AssertionError('UART abort still executed boot command')
    uart.hook('run_command_list',unexpected_command)
    uart.run(uart.syms['autoboot_command'],0x9e300000)
    assert 'UART_input_consumed' in uart.events and not uart.pm[0x48]&2

    direct=Machine(out,envelope);direct.clear_bss()
    direct.put(direct.gd+direct.layout[4],1,1)
    direct.hook('get_boot_role',lambda:direct.ret(0x104))
    direct.hook('sprd_get_chip_version',lambda:direct.ret(0))
    def download_entry():
        direct.result={'packet_initialization_reached':True};direct.uc.emu_stop()
    direct.hook('dl_packet_init',download_entry)
    direct.run(direct.syms['do_download'],0,0,0,0)
    assert direct.result and not direct.pm[0x48]&2

    scenarios=[]
    def boot(name,state,previous,mode,pending,expected,damaged=None,missing=None,invalid_misc=False,inflate_error=False,resume=False,fixup_overlap=False):
        m=Machine(out,envelope)
        m.clear_bss()
        m.control_registers['sctlr_el2']=0x1005
        for index,value,size in [(1,mode,4),(2,previous,2),(3,pending,1),(4,1,1),(5,0,4),(6,24000000,8)]:
            m.put(m.gd+m.layout[index],value,size)
        if resume:
            m.run(m.syms['sy108_boot_maintenance'])
            assert not m.pm[0x48]&2
        misc=bytearray(0x100000);value=('udx_boot_state='+state).encode();misc[4:4+len(value)]=value
        struct.pack_into('<I',misc,0,zlib.crc32(misc[4:]))
        if invalid_misc:misc[0]^=1
        volumes={'misc':misc,'boot':inputs['main'],'recovery':inputs['recovery']}
        if missing:del volumes[missing]
        if damaged:
            b=bytearray(volumes[damaged]);b[7000]^=1;volumes[damaged]=b
        if inflate_error:
            volume='boot' if expected=='main' else 'recovery'
            b=bytearray(volumes[volume]);h=2688;size=int.from_bytes(b[h+12:h+16],'little')
            b[6666:6698]=b'\xff'*32
            struct.pack_into('<I',b,h+24,zlib.crc32(b[6656:6656+size]))
            struct.pack_into('<I',b,h+48,zlib.crc32(b[h:h+48]))
            volumes[volume]=b
        def read_volume():
            if resume:assert m.pm[0x48]&2 and m.pm[0x250]==0xb45a and m.pm[0x44]==30
            vol=m.string(m.x(0));size=m.x(1);offset=m.x(2)
            if vol not in volumes or offset+size>len(volumes[vol]):m.ret(-1);return
            m.uc.mem_write(m.x(3),bytes(volumes[vol][offset:offset+size]));m.ret(0)
        m.hook('common_raw_read',read_volume)
        if fixup_overlap:
            def reserve_kernel():
                address=m.x(0)
                size=int.from_bytes(m.read(address+4,4),'big')
                data=bytearray(m.read(address,size))
                # Enlarge an existing reserved-memory range over Image output.
                reserved=next(props['reg'] for path,props in packages[expected]['nodes'].items()
                              if path.startswith('/reserved-memory/') and len(props.get('reg',b''))==16
                              and props.get('status')!=b'disabled\0')
                assert data.count(reserved)==1
                offset=data.index(reserved)
                data[offset:offset+16]=struct.pack('>QQ',0x80200000,0x100000)
                m.uc.mem_write(address,bytes(data));m.event_ret('fixup_reserved_kernel',0)
            m.fixup=reserve_kernel

        def kernel():
            assert m.x(0)==0x85400000 and [m.x(i) for i in [1,2,3]]==[0,0,0], ('kernel args',[hex(m.x(i)) for i in range(4)],m.events)
            assert m.pm[0x250]==0xb05a
            assert m.level==8 and m.uc.reg_read(UC_ARM64_REG_PSTATE)&0x3c0==0x3c0
            assert not (m.control_registers['sctlr_el2']&0x1005)
            p=packages[expected]
            assert m.read(0x80200000,len(p['image']))==p['image']
            assert m.read(0x85500000,len(p['ramdisk']))==p['ramdisk']
            bootargs=None
            if fdt_parser is not None:
                total=int.from_bytes(m.read(m.x(0)+4,4),'big')
                chosen=fdt_parser(m.read(m.x(0),total))['/chosen']
                assert chosen['udx,boot-target']==(b'openwrt\0' if expected=='main' else b'recovery\0')
                assert int.from_bytes(chosen['linux,initrd-start'],'big')==0x85500000
                assert int.from_bytes(chosen['linux,initrd-end'],'big')==0x85500000+len(p['ramdisk'])
                bootargs=chosen['bootargs'].rstrip(b'\0').decode()
                if expected=='main':
                    for arg in ['ubi.mtd=ubipac','ubi.block=0,rootfs','root=/dev/ubiblock0_21','rootfstype=squashfs','ro','rootwait']:
                        assert arg in bootargs.split()
                    assert sum(x.startswith('root=') for x in bootargs.split())==1
                else:assert chosen['bootargs']==p['nodes']['/chosen']['bootargs']
            m.result={'kernel_entry':0x80200000,'dtb':m.x(0),'stage':m.pm[0x250],'level':m.level,
                      'interrupts_masked':True,'MMU_and_cache_off':True,
                      'DTB_and_initrd_protocol_checked':fdt_parser is not None,'bootargs':bootargs}
            m.uc.emu_stop()
        m.hook_address(0x80200000,kernel)
        m.run(m.syms['sy108_native_boot'],2)
        if fixup_overlap:assert 'fixup_reserved_kernel' in m.events
        if damaged or missing or inflate_error or fixup_overlap:assert m.result['reset_mode']==3 and m.result['stage']==0xbc5a
        else:
            assert 'kernel_entry' in m.result
            assert ('audcp_start' in m.events)==(expected=='main')
            assert ('firmware:nr_agdsp' in m.events)==(expected=='main')
            assert 'cache_mmu_disabled' in m.events and 'interrupts_masked' in m.events
        scenarios.append({'name':name,'result':m.result,'events':m.events})
    boot('cold_accepted','ready',0xffff,0,0,'main')
    boot('clean_reboot','ready',0xb65a,2,1,'main')
    boot('watchdog_runtime','ready',0xbe5a,8,1,'recovery')
    boot('explicit_recovery','ready',0xbc5a,3,1,'recovery')
    boot('retained_recovery','ready',0xbc5a,0,0,'recovery')
    boot('pending_upgrade','pending',0xb65a,2,1,'recovery')
    boot('trial_once','trial',0xb65a,2,1,'main')
    boot('trial_cold','trial',0xffff,0,0,'recovery')
    boot('invalid_misc','ready',0xb65a,2,1,'recovery',invalid_misc=True)
    boot('main_crc_failure','ready',0xffff,0,0,'main','boot')
    boot('recovery_crc_failure','recovery',0xbc5a,3,1,'recovery','recovery')
    boot('main_read_failure','ready',0xffff,0,0,'main',missing='boot')
    boot('recovery_read_failure','recovery',0xbc5a,3,1,'recovery',missing='recovery')
    boot('main_decompression_failure','ready',0xffff,0,0,'main',inflate_error=True)
    boot('resume_after_UART_maintenance','ready',0xffff,0,0,'main',resume=True)
    boot('fixup_kernel_overlap','ready',0xffff,0,0,'main',fixup_overlap=True)
    return {'status':'passed','SML_target_resolver':hex(resolver),
            'SML_context_return_to_Uboot':restored,
            'entry_to_board_init_f':True,'entry_control_registers':entry.control_registers,'GD_bytes':layout[0],'stack_alignment':16,
            'production_relocation_path_and_BSS_clear':True,'allocation':allocation,
            'board_init_f_callbacks':platform,'early_snapshot_and_watchdog':True,
            'late_reset_decoder_uses_snapshot':True,'download_role_does_not_arm_watchdog':True,
            'default_command_dispatch':dispatch,
            'UART_pending_input_aborts_zero_delay_autoboot_and_stops_counter':True,
            'direct_download_command_stops_counter_before_packets':True,
            'adi_rtc_faults':faults,'scenarios':scenarios}
