/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef UDX_RTC_STAGE_H
#define UDX_RTC_STAGE_H
#include "adi.h"
#include "boot-stages.h"
enum udx_rtc_result {
 UDX_RTC_PROFILE = -10,
 UDX_RTC_ACK_TIMEOUT = -11,
 UDX_RTC_VERIFY = -12,
 UDX_RTC_CLOCK = -13,
};
int udx_rtc_stage_write(u16 target, u64 frequency);
#endif
