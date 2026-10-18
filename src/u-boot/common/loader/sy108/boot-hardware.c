/* SPDX-License-Identifier: MIT */
#include "boot-hardware.h"
int udx_boot_clock_enable(void)
{
 const unsigned regs[]={UDX_PMIC_MODULE_ENABLE,UDX_PMIC_CLOCK_ENABLE};
 for(unsigned i=0;i<2;i++){
  uint16_t value;int ret=udx_adi_read(regs[i],&value);if(ret)return ret;
  /* Original U-Boot enables RTC bit1 and watchdog bit2 in both gates. */
  ret=udx_adi_write(regs[i],value|6u);if(ret)return ret;
 }
 return 0;
}
int udx_boot_watchdog_pending(unsigned *pending)
{
 uint16_t raw;int ret;
 ret=udx_adi_read(UDX_PMIC_WDT_STATUS,&raw);if(ret)return ret;
 *pending=!!(raw&8u);return 0;
}
static int boot_watchdog_idle(void)
{
 for(unsigned i=0;i<2000;i++){
  uint16_t raw;int ret=udx_adi_read(UDX_PMIC_WDT_STATUS,&raw);if(ret)return ret;
  if(!(raw&0x10))return 0;
 }
 return UDX_ADI_TIMEOUT;
}
int udx_boot_watchdog_start(void)
{
 int ret=boot_watchdog_idle();if(ret)return ret;
 const unsigned regs[]={UDX_PMIC_WDT_LOCK,UDX_PMIC_WDT_CONTROL,UDX_PMIC_WDT_LOAD_HIGH,UDX_PMIC_WDT_LOAD_LOW,UDX_PMIC_WDT_CONTROL,UDX_PMIC_WDT_LOCK};
 const unsigned values[]={0xe551,4,30,0,14,0x1aae};
 for(unsigned i=0;i<6;i++){
  ret=udx_adi_write(regs[i],values[i]);if(ret)return ret;
  if(i==3){ret=boot_watchdog_idle();if(ret)return ret;}
 }
 return 0;
}
