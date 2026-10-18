/* SPDX-License-Identifier: GPL-2.0+ */
/* Load the selected SY108 system and hand its device tree to Linux. */
#include <common.h>
#include <asm/arch/check_reboot.h>
#include <boot_mode.h>
#include <chipram_env.h>
#include <sprd_common_rw.h>
#include <loader_common.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <image.h>
#include "capsule.h"
#include "boot-hardware.h"

DECLARE_GLOBAL_DATA_PTR;
extern void modem_entry(void);
extern void usb_driver_exit(void);
extern void fdt_fixup_all(u8 *);
extern int _boot_load_required_image(boot_image_required_t);
extern const boot_image_required_t s_boot_image_table[];
extern unsigned char raw_header[];
extern unsigned long boot_img_buffer;
static unsigned maintenance;

static int stage(unsigned value)
{
 return udx_rtc_stage_write(value, gd->arch.sy108_frequency);
}

int sy108_boot_early(void)
{
 unsigned pending = 0;
 uint16_t previous = 0;
 int ret;
 volatile chipram_env_t *env=CHIPRAM_ENV_LOCATION;
 if (env->magic==CHIPRAM_ENV_MAGIC && env->mode==BOOTLOADER_MODE_DOWNLOAD) return 0;
 asm volatile("mrs %0,cntfrq_el0" : "=r"(gd->arch.sy108_frequency));
 ret = udx_boot_clock_enable();
 if (!ret) ret = udx_boot_watchdog_pending(&pending);
 if (!ret) ret = udx_adi_read(UDX_PMIC_RTC_STAGE, &previous);
 if (!ret) ret = udx_adi_reset_snapshot(&gd->arch.sy108_por_source,
                &gd->arch.sy108_por_monitor,&gd->arch.sy108_rtc_reset);
 gd->arch.sy108_stage = previous;
 gd->arch.sy108_pending = pending;
 gd->arch.sy108_valid = 1;
 /* Arm even when the snapshot failed. Never hide an error with a new boot. */
 if (udx_boot_watchdog_start() && !ret) ret = -EIO;
 if (!ret) ret = stage(UDX_RTC_ENTRY_GUARD);
 if (!ret) ret = udx_adi_boot_retention_reason();
 gd->arch.sy108_error = ret;
 return 0; /* Console initialization must remain available for diagnosis. */
}

/* BSS and the legacy ADI driver's software state must exist before decoding. */
void sy108_boot_late(void)
{
 int ret;
 if (!gd->arch.sy108_valid) return;
 gd->arch.sy108_mode = check_reboot_mode();
 ret=udx_adi_boot_retention_reason();
 if (ret && !gd->arch.sy108_error) gd->arch.sy108_error=ret;
}

void sy108_boot_maintenance(void)
{
 extern void stop_watchdog(void);
 if (gd->arch.sy108_valid) {
  stop_watchdog();
  maintenance=1;
 }
}

static void __attribute__((noreturn)) failed(int error)
{
 printf("SY108 boot failed: %d; Recovery retained; PMIC deadline remains active\n", error);
 (void)stage(UDX_RTC_RECOVERY_READY);
 /* The original reset helper shortens the existing PMIC deadline. */
 reboot_devices(CMD_RECOVERY_MODE);
 for (;;) asm volatile("wfi");
}

static int volume(const char *name, void *out, size_t size)
{
 return common_raw_read((char *)name, size, 0, out) ? -EIO : 0;
}
static unsigned le32(const unsigned char *p)
{ return p[0] | p[1]<<8 | p[2]<<16 | (unsigned)p[3]<<24; }

static int read_capsule(unsigned target, struct udx_boot_parts *parts)
{
 unsigned char *cache = (void *)UDX_IMAGE_CACHE;
 const char *name = target == UDX_BOOT_CHOICE_OPENWRT ? "boot" : "recovery";
 unsigned capacity = target == UDX_BOOT_CHOICE_OPENWRT ? UDX_BOOT_VOLUME_BYTES : UDX_RECOVERY_VOLUME_BYTES;
 unsigned length;
 if (volume(name, cache, 512) || memcmp(cache, "DHTB\1\0\0\0", 8)) return -EINVAL;
 length = le32(cache + 48);
 if (length < 8192 || length > capacity - 512) return -EFBIG;
 if (volume(name, cache, length + 512)) return -EIO;
 return udx_boot_parts(cache, length + 512, target, parts);
}

static int target_ok(void *dt, unsigned target)
{
 int node = fdt_path_offset(dt, "/chosen"), n;
 const void *value;
 const char *wanted = target == UDX_BOOT_CHOICE_OPENWRT ? "openwrt" : "recovery";
 if (node < 0) return -EINVAL;
 value = fdt_getprop(dt, node, "udx,boot-policy", &n);
 if (!value || n != 11 || memcmp(value, "openwrt-v1", 11)) return -EINVAL;
 value = fdt_getprop(dt, node, "udx,boot-target", &n);
 if (!value || n != strlen(wanted)+1 || memcmp(value, wanted, n)) return -EINVAL;
 if (!fdt_getprop(dt, node, "udx,trial-local-lease", &n) || n) return -EINVAL;
 return 0;
}

static int intersects(uint64_t a, uint64_t n, uint64_t b, uint64_t m)
{ return n && m && (n > (~0ull)-a || m > (~0ull)-b || (a < b+m && b < a+n)); }
static int ram_range(const void *dt, uint64_t start, uint64_t size)
{
 int node = -1, parent, len, found = 0;
 const fdt64_t *reg;
 uint64_t end = start + size;
 if (!size || end < start) return -EINVAL;
 while ((node = fdt_node_offset_by_prop_value(dt, node, "device_type", "memory", 7)) >= 0) {
  reg = fdt_getprop(dt, node, "reg", &len);
  if (!reg || len % 16) return -EINVAL;
  for (int i=0; i<len/8; i+=2) {
   uint64_t base=fdt64_to_cpu(reg[i]), count=fdt64_to_cpu(reg[i+1]);
   if (base <= start && count <= (~0ull)-base && end <= base+count) found=1;
  }
 }
 if (!found) return -ERANGE;
 parent=fdt_path_offset(dt,"/reserved-memory");
 for (node=fdt_first_subnode(dt,parent);parent>=0 && node>=0;node=fdt_next_subnode(dt,node)) {
  const char *status=fdt_getprop(dt,node,"status",NULL);
  if (status && !strcmp(status,"disabled")) continue;
  reg=fdt_getprop(dt,node,"reg",&len);
  if (!reg) continue;
  if (len%16) return -EINVAL;
  for (int i=0;i<len/8;i+=2)
   if (intersects(start,size,fdt64_to_cpu(reg[i]),fdt64_to_cpu(reg[i+1]))) return -ERANGE;
 }
 for (int i=0;i<fdt_num_mem_rsv(dt);i++) {
  uint64_t a,n;
  if (fdt_get_mem_rsv(dt,i,&a,&n) || intersects(start,size,a,n)) return -ERANGE;
 }
 return 0;
}

static int layout_ok(const void *dt, const struct udx_boot_parts *parts)
{
 return ram_range(dt, UDX_IMAGE_CACHE, parts->total) ||
        ram_range(dt, UDX_IMAGE_DESTINATION, parts->capsule.image_memory_size) ||
        ram_range(dt, UDX_FDT_ADDRESS, 0x100000) ||
        ram_range(dt, UDX_INITRD_ADDRESS, ALIGN(parts->ramdisk_size, 2048));
}

/* Exported for instruction-level verification of the real production handoff. */
void __attribute__((noreturn)) noinline sy108_linux_jump(void *dt, uintptr_t entry)
{
 void (*kernel)(void *, unsigned long, unsigned long, unsigned long) = (void *)entry;
 smp_kick_all_cpus();
 disable_interrupts();
 cleanup_before_linux();
 armv8_switch_to_el2();
 kernel(dt, 0, 0, 0);
 failed(-EFAULT);
}

void __attribute__((noreturn)) sy108_native_boot(unsigned requested)
{
 struct udx_boot_parts parts;
 unsigned target;
 enum udx_upgrade_state state;
 char args[UDX_BOOTARGS_MAX];
 void *dt=(void *)UDX_FDT_ADDRESS;
 int node, ret;
 if (!gd->arch.sy108_valid || gd->arch.sy108_error) failed(-EIO);
 if (maintenance) {
  if (udx_boot_watchdog_start() || stage(UDX_RTC_ENTRY_GUARD) ||
      udx_adi_boot_retention_reason()) failed(-EIO);
  maintenance=0;
 }
 ret=volume("misc",(void *)UDX_IMAGE_CACHE,UDX_MISC_ENV_BYTES);
 state=udx_misc_state((void *)UDX_IMAGE_CACHE,UDX_MISC_ENV_BYTES,ret);
 target=udx_boot_select(state,0,gd->arch.sy108_stage,gd->arch.sy108_mode,gd->arch.sy108_pending);
 if (requested == CMD_RECOVERY_MODE) target=UDX_BOOT_CHOICE_RECOVERY;
 printf("SY108 native boot: target=%s previous=%04x mode=%u wdt=%u misc=%u\n",
        target==UDX_BOOT_CHOICE_OPENWRT?"openwrt":"recovery",gd->arch.sy108_stage,
        gd->arch.sy108_mode,gd->arch.sy108_pending,state);
 if ((ret=read_capsule(target,&parts))) failed(ret);
 if (target_ok((void *)parts.dtb,target)) failed(-EINVAL);
 /* Check destinations before copying the DTB and initrd. */
 if (layout_ok(parts.dtb, &parts)) failed(-ERANGE);
 if (parts.dtb_size > 0xf0000 || fdt_open_into(parts.dtb,dt,0x100000)) failed(-EFBIG);
 memcpy((void *)UDX_INITRD_ADDRESS,parts.ramdisk,parts.ramdisk_size);
 memcpy(raw_header,(void *)(UDX_IMAGE_CACHE+512),2048);
 boot_img_buffer=UDX_IMAGE_CACHE;
 setenv("bootmode",target==UDX_BOOT_CHOICE_OPENWRT?"normal":"recovery");
 if (target==UDX_BOOT_CHOICE_OPENWRT) {
  for (const boot_image_required_t *p=s_boot_image_table;p->partition;p++)
   if (!_boot_load_required_image(*p)) failed(-EIO);
 }
 fdt_fixup_all(dt);
 /* Board fixups can change RAM and reserved-memory ranges. */
 if (layout_ok(dt, &parts)) failed(-ERANGE);
 node=fdt_path_offset(dt,"/chosen");
 if (target==UDX_BOOT_CHOICE_OPENWRT) {
  int size;
  const char *old=fdt_getprop(dt,node,"bootargs",&size);
  if (!old || udx_bootargs_direct_root(old,size,args,sizeof(args)) ||
      fdt_setprop_string(dt,node,"bootargs",args)) failed(-EINVAL);
 }
 if (fdt_initrd_norsvmem(dt,UDX_INITRD_ADDRESS,UDX_INITRD_ADDRESS+parts.ramdisk_size,1)) failed(-EINVAL);
 if (udx_capsule_decode(&parts,(void *)UDX_IMAGE_DESTINATION,UDX_IMAGE_LIMIT,gunzip)) failed(-EBADMSG);
 if (target==UDX_BOOT_CHOICE_OPENWRT) modem_entry();
 usb_driver_exit();
 if (stage(UDX_RTC_IMAGE_ENTRY)) failed(-EIO);
 sy108_linux_jump(dt,UDX_IMAGE_DESTINATION);
}
