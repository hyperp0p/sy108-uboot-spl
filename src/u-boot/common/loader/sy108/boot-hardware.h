/* SPDX-License-Identifier: MIT */
#ifndef UDX_BOOT_HARDWARE_H
#define UDX_BOOT_HARDWARE_H
#include "rtc-stage.h"
int udx_boot_clock_enable(void);
int udx_boot_watchdog_pending(unsigned *);
int udx_boot_watchdog_start(void);
#endif
