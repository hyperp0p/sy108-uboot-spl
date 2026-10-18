/* SPDX-License-Identifier: MIT */
#ifndef SY108_ADI_H
#define SY108_ADI_H
#include <common.h>

enum udx_adi_result {
 UDX_ADI_OK = 0,
 UDX_ADI_TIMEOUT = -3,
 UDX_ADI_ECHO = -4,
};
enum udx_pmic_register {
 UDX_PMIC_WDT_LOAD_LOW = 0x40,
 UDX_PMIC_WDT_LOAD_HIGH = 0x44,
 UDX_PMIC_WDT_CONTROL = 0x48,
 UDX_PMIC_WDT_STATUS = 0x50,
 UDX_PMIC_WDT_LOCK = 0x60,
 UDX_PMIC_RTC_STATUS = 0x234,
 UDX_PMIC_RTC_CLEAR = 0x238,
 UDX_PMIC_RTC_STAGE = 0x250,
 UDX_PMIC_RTC_STAGE_UPDATE = 0x254,
 UDX_PMIC_MODULE_ENABLE = 0x1808,
 UDX_PMIC_CLOCK_ENABLE = 0x1810,
 UDX_PMIC_RESET_REASON = 0x1bac,
 UDX_PMIC_POR_SOURCE = 0x1bb8,
 UDX_PMIC_RTC_RESET = 0x1bec,
};
#define UDX_PMIC_RTC_STAGE_ACK 0x80u
int udx_adi_read(u32 reg, u16 *value);
int udx_adi_write(u32 reg, u16 value);
int udx_adi_reset_snapshot(u16 *source, u16 *monitor, u16 *rtc);
int udx_adi_boot_retention_reason(void);
#endif
