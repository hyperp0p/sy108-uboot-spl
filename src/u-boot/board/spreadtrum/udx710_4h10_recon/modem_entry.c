#include <common.h>
#include <asm/arch/common.h>
#include <asm/arch/sprd_reg.h>

#include "cp_boot.h"

/*add macro for memset agdsp/tddsp/ldsp share memory*/
#define DSP_BOOT_IRAM_START   0x10000//2k
#define LDSP_SHARE_MEM_START  0x10800//10k
#define TGDSP_SHARE_MEM_START 0x13000//4k
#define MULTI_MODE_MEM_START  0x14000//4k


/*bug:533275  memset tg/l/agdsp share memory
root cause: In agdsp share IRAM memory ,this area exits command for agdsp,when interunpt happens
from ap or cp,the agdsp will use this area. While the area exit the dirty binary data ,ap or cp
do not clean up this area when interrupt happens at the right time,so error command will cause
unexcepted problems.*/

void memset_dsp_share_memory()
{
	memset((void *)DSP_BOOT_IRAM_START,0x0,0x800);//2k
	memset((void *)LDSP_SHARE_MEM_START,0x0,0x2800);//10k
	memset((void *)TGDSP_SHARE_MEM_START,0x0,0x1000);//4k
	memset((void *)MULTI_MODE_MEM_START,0x0,0x1000);//4k
}

#include <asm/io.h>
noinline void audcp_share_memory_init(void)
{
 memset((void *)0x7400,0,0x1000);
 memset((void *)0x87400000,0,0xe10);
}

noinline void audcp_boot(unsigned int addr)
{
 if (memcmp((void *)(unsigned long)addr,"ORCA_AUDCP",strlen("ORCA_AUDCP")))return;
 addr=(addr+0x80)>>1;
 __raw_writel(__raw_readl((void *)0x64010060)&~0x02000000,(void *)0x64010060);
 __raw_writel(__raw_readl((void *)0x64010544)&~0x10,(void *)0x64010544);
 __raw_writel(__raw_readl((void *)0x6401005c)&~0x03000000,(void *)0x6401005c);
 __raw_writel(__raw_readl((void *)0x6402060c)&0xffff0000,(void *)0x6402060c);
 __raw_writel(__raw_readl((void *)0x6402060c)|0x9620,(void *)0x6402060c);
 __raw_writel(addr,(void *)0x64020640);
 __raw_writel(__raw_readl((void *)0x64020644)|1,(void *)0x64020644);
 __raw_writel(__raw_readl((void *)0x640100b0)&~0x600,(void *)0x640100b0);
 __raw_writel(__raw_readl((void *)0x640106b0)&~4,(void *)0x640106b0);
}
void modem_entry(void)
{
 audcp_share_memory_init();
 audcp_boot(0x86e00000);
}
