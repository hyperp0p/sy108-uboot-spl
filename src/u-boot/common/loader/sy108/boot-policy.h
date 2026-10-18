/* SPDX-License-Identifier: MIT */
#ifndef UDX_OPENWRT_BOOT_POLICY_H
#define UDX_OPENWRT_BOOT_POLICY_H
#include <stddef.h>
#ifdef UDX_UBOOT
#include <common.h>
#ifndef UINTPTR_MAX
#define UINTPTR_MAX (~(uintptr_t)0)
#endif
#else
#include <stdint.h>
#endif
#define UDX_MISC_ENV_BYTES 0x100000u
#define UDX_BOOT_VOLUME_BYTES (133u*126976u)
#define UDX_RECOVERY_VOLUME_BYTES (149u*126976u)
#define UDX_BOOT_STATE_KEY "udx_boot_state"
#define UDX_OPENWRT_ROOT_ARGS "ubi.mtd=ubipac ubi.block=0,rootfs root=/dev/ubiblock0_21 rootfstype=squashfs ro rootwait"
#define UDX_BOOTARGS_MAX 2048u
enum udx_upgrade_state { UDX_UPGRADE_INVALID, UDX_UPGRADE_READY, UDX_UPGRADE_PENDING, UDX_UPGRADE_RECOVERY, UDX_UPGRADE_TRIAL };
enum udx_boot_choice { UDX_BOOT_CHOICE_RECOVERY=1, UDX_BOOT_CHOICE_OPENWRT=2 };
uint32_t udx_crc32(const void *,size_t);
enum udx_upgrade_state udx_misc_state(const void *,size_t,int read_error);
enum udx_boot_choice udx_boot_select(enum udx_upgrade_state,int rtc_error,unsigned rtc_stage,unsigned original_mode,unsigned watchdog_pending);
int udx_bootargs_direct_root(const char *,size_t,char *,size_t);
#endif
