"""Envelope validation, AArch64 SPL verification and boot regression checks."""
import gzip
import hashlib
import json
import os
from importlib.metadata import version as package_version
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import zlib
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from elftools.elf.elffile import ELFFile
from emulate import verify_instructions, section


def sha(b):
    return hashlib.sha256(b).hexdigest()


def image_fields(data):
    assert data[:8] == b'DHTB\1\0\0\0'
    size = int.from_bytes(data[48:52], 'little')
    footer = data[512+size:512+size+96]
    assert len(footer) == 96
    signed_size, signed_offset, cert_size, cert_offset = struct.unpack_from('<4Q', footer, 16)
    assert signed_offset >= 512 and signed_offset+signed_size <= 512+size
    assert cert_offset >= 512+size+96 and cert_offset+cert_size <= len(data)
    return size, signed_size, signed_offset, cert_size, cert_offset


def certificate(data):
    _, length, offset, count, start = image_fields(data)
    cert = data[start:start+count]
    kind, bits = struct.unpack_from('<II', cert)
    assert bits == 2048 and count == (596 if kind == 1 else 564)
    # Exponent bytes are big endian even though the surrounding C layout is LE.
    pub = rsa.RSAPublicNumbers(int.from_bytes(cert[8:12], 'big'),
                              int.from_bytes(cert[12:268], 'big')).public_key()
    assert hashlib.sha256(data[offset:offset+length]).digest() == cert[268:300]
    end = 340 if kind == 1 else 308
    pub.verify(cert[end:end+256], cert[268:end],
               padding.PSS(mgf=padding.MGF1(hashes.SHA256()), salt_length=32), hashes.SHA256())
    return cert


def guest(binary, arguments):
    r = subprocess.run(['timeout', '-s', 'KILL', '25', 'qemu-aarch64', '-cpu', 'max', str(binary), *map(str, arguments)],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60)
    return r.returncode, (r.stdout+r.stderr).decode(errors='replace')[-2000:]


def fdt_nodes(data):
    assert data[:4] == bytes.fromhex('d00dfeed')
    total, pos, strings = struct.unpack_from('>III', data, 4)
    assert 40 <= total <= len(data)
    stack, nodes = [], {}
    while pos+4 <= total:
        token = int.from_bytes(data[pos:pos+4], 'big'); pos += 4
        if token == 1:
            end = data.index(0, pos)
            stack.append(data[pos:end].decode()); pos = (end+4)&~3
            nodes['/'.join(stack) or '/'] = {}
        elif token == 2:
            stack.pop()
        elif token == 3:
            n, key = struct.unpack_from('>II', data, pos); pos += 8
            end = data.index(0, strings+key)
            name = data[strings+key:end].decode()
            nodes['/'.join(stack) or '/'][name] = data[pos:pos+n]; pos = (pos+n+3)&~3
        elif token == 4:
            continue
        elif token == 9:
            assert not stack
            return nodes
        else:
            raise ValueError('bad FDT token')
    raise ValueError('incomplete FDT')


def capsule(data, target):
    assert data[:8] == b'DHTB\1\0\0\0' and data[512:520] == b'ANDROID!'
    assert int.from_bytes(data[48:52], 'little') == len(data)-512
    k, rd, dt = [int.from_bytes(data[p:p+4], 'little') for p in (520, 528, 552)]
    align = lambda n: (n+2047)&~2047
    rdoff = 2560+align(k); dtoff = rdoff+align(rd)
    assert dtoff+dt == len(data)
    header = struct.unpack_from('<16I', data, 2560+128)
    assert header[:3] == (0x315a4758, 1, target) and not any(header[13:])
    assert zlib.crc32(data[2688:2736]) == header[12]
    zipped = data[2560+4096:2560+k]
    image = gzip.decompress(zipped)
    ramdisk = data[rdoff:rdoff+rd]
    tree = data[dtoff+2048:dtoff+2048+header[10]]
    assert (len(zipped), len(image), len(ramdisk), len(tree)) == (header[3],header[4],header[8],header[10])
    for b, crc in [(zipped,header[6]),(image,header[7]),(ramdisk,header[9]),(tree,header[11])]:
        assert zlib.crc32(b) == crc
    nodes = fdt_nodes(tree)
    chosen = nodes['/chosen']
    assert chosen['udx,boot-policy'] == b'openwrt-v1\0'
    assert chosen['udx,boot-target'] == (b'openwrt\0' if target == 2 else b'recovery\0')
    assert 'udx,trial-local-lease' in chosen
    assert image[56:60] == b'ARMd' and int.from_bytes(image[8:16],'little') == 0
    return {'image':image, 'ramdisk':ramdisk, 'dtb':tree, 'nodes':nodes,
            'memory_size':header[5], 'gzip':zipped}


def main(out, expected):
    report = {'status':'failed',
              'source_commit':os.environ.get('SOURCE_COMMIT'),
              'source_dirty':os.environ.get('SOURCE_DIRTY') == '1',
              'source_date_epoch':os.environ.get('SOURCE_DATE_EPOCH'),
              'oem_commit':os.environ.get('OEM_COMMIT'), 'image_id':os.environ.get('BUILD_IMAGE_ID')}
    try:
        inputs = {name:Path('/inputs/'+name+'.bin').read_bytes() for name in ['spl','sml','main','recovery']}
        report['inputs'] = {n:{'source':os.environ.get(n.upper()+'_SOURCE'),
                               'mount':'/inputs/'+n+'.bin',
                               'bytes':len(b),'sha256':sha(b)} for n,b in inputs.items()}
        data = (out/'uboot.bin').read_bytes()
        assert len(data) == 0x140000 and sha(data) == expected
        root = certificate(inputs['spl']); donor = certificate(inputs['sml'])
        size, length, offset, count, certoff = image_fields(data[:0xc0000])
        cert = certificate(data[:0xc0000])
        assert cert == donor and sha(cert[4:268]) == root[300:332].hex()
        raw = (out/'uboot/u-boot.bin').read_bytes()
        dtb = (out/'uboot/u-boot.dtb').read_bytes()
        code = (out/'uboot/u-boot-dtb.bin').read_bytes()
        assert code == raw+dtb and data[512:512+len(code)] == code
        assert data[offset:offset+length] == inputs['sml'][512:512+length]
        assert not any(b != 255 for b in data[certoff+count:])
        assert data[8:40] == hashlib.sha256(data[512:512+size]).digest()
        assert int.from_bytes(data[40:48],'little') == 0x9f000000
        assert offset == 512+((len(code)+15)&~15)
        assert data[512+len(code):offset] == bytes(offset-512-len(code))
        assert certoff == 512+size+96 and certoff+count <= 0xc0000
        old_size=int.from_bytes(inputs['sml'][48:52],'little')
        original_descriptor=inputs['sml'][512+old_size:512+old_size+96]
        normalized_descriptor=bytearray(data[512+size:512+size+96])
        normalized_descriptor[24:32]=original_descriptor[24:32]
        normalized_descriptor[40:48]=original_descriptor[40:48]
        assert normalized_descriptor == original_descriptor
        report['envelope'] = {'partition_bytes':len(data),'required_bytes':certoff+count,
                              'spl_read_bytes':0xc0000,'spare_read_bytes':0xc0000-certoff-count,
                              'new_uboot_dtb_bytes':len(code),'donor_bytes':length,
                              'signed_offset':offset,'certificate_kind':'SML content certificate',
                              'regions':{'DHTB':[0,512],'new_uboot_and_DTB':[512,512+len(code)],
                                         'signed_SML':[offset,offset+length],
                                         'signature_descriptor':[512+size,512+size+96],
                                         'original_certificate':[certoff,certoff+count]},
                              'root_next_public_key_SHA256':sha(cert[4:268]),
                              'certificate_fields':{'type':int.from_bytes(cert[:4],'little'),
                                                    'firmware_type':int.from_bytes(cert[300:304],'little'),
                                                    'version':int.from_bytes(cert[304:308],'little')},
                              'independent_RSA_PSS':'passed'}
        with (out/'uboot/u-boot').open('rb') as f:
            elf=ELFFile(f)
            symbols={s.name:s['st_value'] for s in section(elf,'.symtab').iter_symbols()}
            assert elf['e_entry'] == 0x9f000000 == symbols['_start']
            assert symbols['_end']-symbols['_start'] == len(raw)
            forbidden=['sprd_secure_check','sprd_verify_img','orca_cpe_boot_verify','orca_cpe_dl_verify','sprd_anti_rollback','update_sec_version']
            assert not any(symbols.get(n) for n in forbidden)
            report['ELF']={'entry':elf['e_entry'],'raw_bytes':len(raw),'dtb_bytes':len(dtb),
                           'DTB_source_address':symbols['_end'],
                           'bss_start':symbols['__bss_start'],'bss_end':symbols['__bss_end'],
                           'OEM_security_symbols_absent':True}
        with tempfile.TemporaryDirectory(prefix='sy108-offline-') as directory:
            directory=Path(directory); cases=[]
            def spl_case(name, blob, passed, version=0, limit=0xc0000, bundle=False):
                p=directory/'candidate.bin';p.write_bytes(blob)
                rc, log=guest(out/'spl-check',['/inputs/spl.bin',p,limit,version]+(['/inputs/sml.bin'] if bundle else []))
                print('SPL check: '+name+' rc='+str(rc),flush=True)
                assert rc==(0 if passed else 96 if name=='read_window_truncated' else 1), (name,rc,log)
                cases.append({'name':name,'expected_accept':passed,'returncode':rc,'detail':log.strip()})
            spl_case('original_SML',inputs['sml'],True,limit=0x10000)
            spl_case('new_envelope',data,True)
            spl_case('NAND_four_verifications_and_version_commit',data,True,bundle=True)
            for name,position in [('donor_corrupted',offset+100),('signature_corrupted',certoff+count-1),('public_key_corrupted',certoff+20)]:
                bad=bytearray(data);bad[position]^=1;spl_case(name,bad,False)
            spl_case('rollback_version_rejected',data,False,version=1)
            spl_case('read_window_truncated',data,False,limit=certoff+count-1)
            report['SPL_source_checks']=cases
            rc,log=guest(out/'boot-check',[]);assert rc==0,log
            report['boot_policy']={'status':'passed','detail':log.strip()}
            packages={}
            for target,name in [(2,'main'),(1,'recovery')]:
                parsed=capsule(inputs[name],target);packages[name]=parsed
                rc,log=guest(out/'boot-check',['/inputs/'+name+'.bin',target]);assert rc==0,log
                wrong,_=guest(out/'boot-check',['/inputs/'+name+'.bin',3-target]);assert wrong!=0
                p=directory/'bad-capsule.bin';b=bytearray(inputs[name]);b[7000]^=1;p.write_bytes(b)
                rc,_=guest(out/'boot-check',[p,target]);assert rc!=0
                b=bytearray(inputs[name]);h=2688;size=int.from_bytes(b[h+12:h+16],'little')
                b[6666:6698]=b'\xff'*32
                struct.pack_into('<I',b,h+24,zlib.crc32(b[6656:6656+size]))
                struct.pack_into('<I',b,h+48,zlib.crc32(b[h:h+48]))
                p=directory/'bad-deflate.bin';p.write_bytes(b)
                rc,log=guest(out/'boot-check',[p,target]);assert rc!=0 and 'DECODE_RESULT=' in log,log
            report['capsules']={n:{'status':'passed','kernel_bytes':len(p['image']),
                                   'kernel_memory_bytes':p['memory_size'],'dtb_bytes':len(p['dtb']),
                                   'initrd_bytes':len(p['ramdisk']),'corruption_and_wrong_target_rejected':True,
                                   'actual_uboot_gunzip_and_zlib':'passed','malformed_deflate_rejected':True}
                                for n,p in packages.items()}
            report['instruction_emulation']=verify_instructions(out,inputs,packages,fdt_nodes)
            allocation=report['instruction_emulation']['allocation']
            load=0x9f000000-512
            report['envelope']['donor_or_certificate_reused_by_BSS_after_verification']=(
                load+offset < allocation['BSS_end'] and allocation['BSS_start'] < load+certoff+count)
        report['tools']=(out/'toolchain.txt').read_text().splitlines()
        report['verification_runtime']={'python':sys.version, 'packages':{n:package_version(n) for n in ['cryptography','unicorn','capstone','pyelftools']},'qemu':subprocess.check_output(['qemu-aarch64','--version'],text=True).splitlines()[0]}
        source_hash=hashlib.sha256()
        for base in [Path('/src/u-boot'),Path('/src/chipram'),Path('/opt/boot')]:
            for p in sorted(base.rglob('*')):
                if p.is_file() and '__pycache__' not in str(p):
                    source_hash.update(str(p).encode()+b'\0'+p.read_bytes())
        report['source_tree_sha256']=source_hash.hexdigest()
        report['status']='passed'
    except Exception as error:
        report['error']=str(error)
        raise
    finally:
        (out/'offline-report.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps({'status':report['status'],'required_bytes':report['envelope']['required_bytes'],
                      'spare_read_bytes':report['envelope']['spare_read_bytes'],'sha256':expected},indent=2),flush=True)


if __name__ == '__main__':
    main(Path(sys.argv[1]),sys.argv[2])
