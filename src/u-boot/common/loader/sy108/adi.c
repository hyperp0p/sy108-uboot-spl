/* SPDX-License-Identifier: MIT */
#include <asm/io.h>
#include "adi.h"
#define ADI_CONTROLLER 0x63a00000u
#define ADI_SLAVE 0x63a20000u
#define ADI_RD_CMD 0x28u
#define ADI_RD_DATA 0x2cu
#define ADI_FIFO 0x30u
#define ADI_BUSY (1u << 31)
#define ADI_FULL (1u << 11)
#define ADI_EMPTY (1u << 10)
#define ADI_POLL_LIMIT 2000u

static void adi_write32(uintptr_t address, u32 value)
{
 writel(value, (void *)address);
 asm volatile("dsb oshst" ::: "memory");
}
static int adi_idle(void)
{
 for (unsigned i = 0; i < ADI_POLL_LIMIT; i++) {
  u32 value = readl((void *)(ADI_CONTROLLER + ADI_FIFO));
  if ((value & (ADI_FULL | ADI_EMPTY)) == ADI_EMPTY)
   return UDX_ADI_OK;
 }
 return UDX_ADI_TIMEOUT;
}
int udx_adi_read(u32 reg, u16 *value)
{
 u32 address = ADI_SLAVE + reg;
 int ret = adi_idle();
 if (ret) return ret;
 adi_write32(ADI_CONTROLLER + ADI_RD_CMD, address);
 for (unsigned i = 0; i < ADI_POLL_LIMIT; i++) {
  u32 data = readl((void *)(ADI_CONTROLLER + ADI_RD_DATA));
  if (data & ADI_BUSY) continue;
  /* The response echoes physical address bits [16:2]. */
  if (((data >> 16) & 0x7fff) != ((address & 0x1fffc) >> 2))
   return UDX_ADI_ECHO;
  *value = (u16)data;
  return UDX_ADI_OK;
 }
 return UDX_ADI_TIMEOUT;
}
int udx_adi_write(u32 reg, u16 value)
{
 int ret = adi_idle();
 if (ret) return ret;
 adi_write32(ADI_SLAVE + reg, value);
 /* Wait until the controller has drained the submitted write. */
 return adi_idle();
}
int udx_adi_reset_snapshot(u16 *source, u16 *monitor, u16 *rtc)
{
 int ret = udx_adi_read(UDX_PMIC_POR_SOURCE, source);
 if (ret) return ret;
 ret = udx_adi_read(UDX_PMIC_RESET_REASON, monitor);
 if (ret) return ret;
 return udx_adi_read(UDX_PMIC_RTC_RESET, rtc);
}
int udx_adi_boot_retention_reason(void)
{
 u16 value, target;
 int ret = udx_adi_read(UDX_PMIC_RESET_REASON, &value);
 if (ret) return ret;
 target = (value & 0x200u) | 0xf0;
 ret = udx_adi_write(UDX_PMIC_RESET_REASON, target);
 if (ret) return ret;
 ret = udx_adi_read(UDX_PMIC_RESET_REASON, &value);
 if (ret) return ret;
 return value == target ? UDX_ADI_OK : UDX_ADI_ECHO;
}
