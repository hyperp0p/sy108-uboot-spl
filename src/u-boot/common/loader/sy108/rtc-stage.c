/* SPDX-License-Identifier: GPL-2.0-only */
#include "rtc-stage.h"

static u64 rtc_ticks(void)
{
 u64 value, level;
 asm volatile("mrs %0,CurrentEL" : "=r"(level));
 if (level >= 8) asm volatile("isb; mrs %0,cntpct_el0" : "=r"(value));
 else asm volatile("isb; mrs %0,cntvct_el0" : "=r"(value));
 return value;
}
int udx_rtc_stage_write(u16 target, u64 frequency)
{
 u16 raw, value;
 u64 start;
 int ret;
 if (!udx_rtc_stage_value(target)) return UDX_RTC_PROFILE;
 if (!frequency) return UDX_RTC_CLOCK;
 ret = udx_adi_write(UDX_PMIC_RTC_CLEAR, UDX_PMIC_RTC_STAGE_ACK);
 if (ret) return ret;
 ret = udx_adi_write(UDX_PMIC_RTC_STAGE_UPDATE, target);
 if (ret) return ret;
 start = rtc_ticks();
 /* RTC updates cross a clock domain; allow 200 ms for acknowledgement. */
 for (unsigned i = 0; i < 50000; i++) {
  ret = udx_adi_read(UDX_PMIC_RTC_STATUS, &raw);
  if (ret) return ret;
  if (raw & UDX_PMIC_RTC_STAGE_ACK) {
   ret = udx_adi_read(UDX_PMIC_RTC_STAGE, &value);
   if (ret) return ret;
   if (value != target) return UDX_RTC_VERIFY;
   return udx_adi_write(UDX_PMIC_RTC_CLEAR, UDX_PMIC_RTC_STAGE_ACK);
  }
  if (rtc_ticks() - start >= frequency / 5u) return UDX_RTC_ACK_TIMEOUT;
 }
 return UDX_RTC_ACK_TIMEOUT;
}
