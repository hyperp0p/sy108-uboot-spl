#include "sci_types.h"
#include "ddrc_r1p1.h"
#include "ddrc_r1p1_phy.h"
extern DRAM_CHIP_INFO ddr_chip_cur;
extern TRAIN_CONDITIONS_TABLE phy_train;
extern DDR_CLK_ASSO_CFG ddr_clk_asso_cfg_table[FREQ_POINT_NUM_MAX];
extern u16 dram_init_powerup_times;

LPDDR4_ODT_CFG lpddr4_odt_cfg[FREQ_POINT_NUM_MAX] = {
	//F0 mr value 256M
	{
		.mr1 = 0x4,
		.mr2 = 0x0,
		.mr3 = 0xb3,
		.mr11 = 0x00,
		.mr12 = 0x4d,
		.mr14 = 0x4d,
		.mr22 = 0x03
	},
	//F1 mr value 384M
	{
		.mr1 = 0x14,
		.mr2 = 0x09,
		.mr3 = 0xb3,
		.mr11 = 0x00,
		.mr12 = 0x4d,
		.mr14 = 0x4d,
		.mr22 = 0x03
	},
	//F2 mr value 667M
	{
		.mr1 = 0x24,
		.mr2 = 0x12,
		.mr3 = 0xb3,
		.mr11 = 0x00,
		.mr12 = 0x4d,
		.mr14 = 0x4d,
		.mr22 = 0x03
	},
	//F3 mr value 768M
	{
		.mr1 = 0x24,
		.mr2 = 0x12,
		.mr3 = 0xb3,
		.mr11 = 0x00,
		.mr12 = 0x4d,
		.mr14 = 0x4d,
		.mr22 = 0x03
	},
	//F4 mr value 1024M
	{
		.mr1 = 0x3C,
		.mr2 = 0x1b,
		.mr3 = 0xb3,
		.mr11 = 0x04,
		.mr12 = 0x4d,
		.mr14 = 0x0d,
		.mr22 = 0x03
	},
	//F5 mr value 1333M
	{
		.mr1 = 0x3C,
		.mr2 = 0x24,
		.mr3 = 0xb3,
		.mr11 = 0x04,
		.mr12 = 0x4d,
		.mr14 = 0x0d,
		.mr22 = 0x03
	},
	//F6 mr value 1536M
	{
		.mr1 = 0x5C,
		.mr2 = 0x2d,
		.mr3 = 0xb3,
		.mr11 = 0x04,
		.mr12 = 0x4d,
		.mr14 = 0x0d,
		.mr22 = 0x03
	},
	//F7 mr value 1866M
	{
		.mr1 = 0x6C,
		.mr2 = 0x36,
		.mr3 = 0xb3,
		.mr11 = 0x04,
		.mr12 = 0x4d,
		.mr14 = 0x0d,
		.mr22 = 0x03
	}
};

LPDDR4_ODT_CFG lpddr4x_odt_cfg[FREQ_POINT_NUM_MAX] = {
 {.mr1=0x04, .mr2=0x00, .mr3=0xa7, .mr11=0x00, .mr12=0x5d, .mr14=0x5d, .mr22=0x3c},
 {.mr1=0x14, .mr2=0x09, .mr3=0xa7, .mr11=0x00, .mr12=0x5d, .mr14=0x5d, .mr22=0x3c},
 {.mr1=0x24, .mr2=0x12, .mr3=0xa7, .mr11=0x00, .mr12=0x5d, .mr14=0x5d, .mr22=0x3c},
 {.mr1=0x24, .mr2=0x12, .mr3=0xa7, .mr11=0x00, .mr12=0x5e, .mr14=0x5e, .mr22=0x3c},
 {.mr1=0x3c, .mr2=0x1b, .mr3=0xb7, .mr11=0x03, .mr12=0x5e, .mr14=0x1e, .mr22=0x3c},
 {.mr1=0x3c, .mr2=0x24, .mr3=0xb7, .mr11=0x05, .mr12=0x5e, .mr14=0x1e, .mr22=0x3c},
 {.mr1=0x5c, .mr2=0x2d, .mr3=0xb7, .mr11=0x42, .mr12=0x5d, .mr14=0x19, .mr22=0x36},
 {.mr1=0x6c, .mr2=0x36, .mr3=0xb7, .mr11=0x05, .mr12=0x5e, .mr14=0x1e, .mr22=0x3c}
};


LPDDR4_MR_INFO lpddr4_mr_info = {
	DEFAULT_LPDDR4_BL,  /*burst length*/
	0,                 /*burst type*/
	0,                 /*wrap*/
	3,                 /*nwr*/
	32,               /*read latency*/
	14,                /*write latency*/
	DDR_DRV_CFG,       /*driver strength*/
	1,                 /*DBI en*/
	lpddr4x_odt_cfg
};


static DDRC_R1P1_TIMING_CFG ddrc_local_timing_lpddr4[] = {
 {1866, {
  0x4b161313, 0x03164e05, 0xa75474af, 0xe70c252a, 0x22831807, 0x00000809, 0x0000120e, 0x0176074a, 0x23a6001f, 0x00030c80, 0x151c0005, 0x00000000, 0x00000000, 0x015c0236, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {1536, {
  0x49120f0f, 0x03134c04, 0x8a466490, 0xc60a1f22, 0x22731706, 0x00000708, 0x0000100e, 0x01340c00, 0x23010017, 0x00030c00, 0x15180005, 0x00000000, 0x00000000, 0x014c022d, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {1333, {
  0x48100d0d, 0x03104b03, 0x773c587d, 0xa5081b1e, 0x22631605, 0x00000607, 0x00000d0c, 0x010b0a6a, 0x229c0017, 0x00030a70, 0x15140005, 0x00000000, 0x00000000, 0x01340224, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {1024, {
  0x460c0a0a, 0x020e2902, 0x5c2f4860, 0x84061417, 0x22531504, 0x00000506, 0x00000b08, 0x00cd0800, 0x12010000, 0x00030c80, 0x15100005, 0x00000000, 0x00000000, 0x013c021b, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {768, {
  0x450a0808, 0x020b2802, 0x45233848, 0x63040f12, 0x22431403, 0x00000f05, 0x00000805, 0x009a0600, 0x11810000, 0x00030600, 0x150c0005, 0x00000000, 0x00000000, 0x01240212, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {667, {
  0x44090707, 0x020b2802, 0x3c1f343f, 0x63040d10, 0x22431403, 0x00000f05, 0x00000805, 0x00860536, 0x514f0000, 0x00030540, 0x150b0005, 0x00000000, 0x00000000, 0x01240212, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {384, {
  0x42050404, 0x02092502, 0x22122434, 0x33040809, 0x22431302, 0x00000f04, 0x00000505, 0x004d0300, 0x50c10000, 0x00030300, 0x15060005, 0x00000000, 0x00000000, 0x01140209, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {256, {
  0x42040303, 0x02082402, 0x170c1c34, 0x23040506, 0x22431201, 0x00000f03, 0x00000301, 0x00340200, 0x50810000, 0x00030200, 0x15040005, 0x00000000, 0x00000000, 0x01040200, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }},
 {156, {
  0x42030202, 0x01082402, 0x0e081c0f, 0x23040404, 0x22431201, 0x00000f03, 0x00000301, 0x0020009c, 0x50510000, 0x00030c80, 0x15040005, 0x00000000, 0x00000000, 0x01040200, 0x03830383, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000
 }}
};
/*****************************/
//channel 0, bit0 set 1
//channel 1, bit1 set 1
//channel 2, bit2 set 1
//channel 3, bit3 set 1
/*****************************/
void lpddr_dmc_mrw_chn_sel(DMC_CMD_CHN_INDEX channel)
{
	DDRC_R1P1_REG_INFO_PTR pdmc = (DDRC_R1P1_REG_INFO_PTR)DMC_REG_ADDR_BASE_PHY;
	uint32 regval;
	/*enable channel xx software command.*/
	regval = pdmc->dmc_dcfg0;
	regval = u32_bits_set(regval, 20, 4, channel);
	pdmc->dmc_dcfg0 = regval;
}


void lpddr_dmc_mrw(DMC_CMD_CS_INDEX cs, int mr_addr, unsigned char val)
{
	DDRC_R1P1_REG_INFO_PTR pdmc = (DDRC_R1P1_REG_INFO_PTR)DMC_REG_ADDR_BASE_PHY;
	uint32 tmp;

	/*set reg val*/
	tmp = pdmc->dmc_dcfg2;
	tmp = u32_bits_set(tmp, 0, 8, val);
	pdmc->dmc_dcfg2 = tmp;
	dmc_sprd_delay(10);

	tmp = pdmc->dmc_dcfg1;
	/*set mr register addr*/
	tmp = u32_bits_set(tmp, 0, 16, mr_addr);
	/*set cs index*/
	if (CMD_CS_0 == cs)
	{
		/*1 cs mode*/
		tmp = u32_bits_set(tmp, 31, 1, 0);
		/*cs index*/
		tmp = u32_bits_set(tmp, 28, 1, 0);
	}
	else if (CMD_CS_1 == cs)
	{
		/*1 cs mode*/
		tmp = u32_bits_set(tmp, 31, 1, 0);
		/*cs index*/
		tmp = u32_bits_set(tmp, 28, 1, 1);
	}
	/*2 cs mode*/
	else
	{
		tmp = u32_bits_set(tmp, 31, 1, 1);
	}
	/*triger MRW*/
	tmp = u32_bits_set(tmp, 24, 1, 1);
	pdmc->dmc_dcfg1 = tmp;

	/* wait until [27:19]==0 */
	while(((pdmc->dmc_dcfg1 >> 19) & 0x1ff) != 0);
}
int search_for_freq_point(const DDRC_R1P1_TIMING_CFG *dmc_timing, int size,  uint32 clk)
{
	int i;
	for(i = 0; i < size; i++)
	{
		if (clk == dmc_timing[i].clk)
		{
			return i;
		}
	}
	return -1;
}

int  lpddr_timing_init(DRAM_TYPE ddr_type, dfs_freq_table ddr_frequency[8])
{
	int i,k;
	int  size;
	DDRC_R1P1_REG_INFO_PTR pdmc = (DDRC_R1P1_REG_INFO_PTR)DMC_REG_ADDR_BASE_PHY;
	const DDRC_R1P1_TIMING_CFG * dmc_local_timing_ptr;
	uint32  ddr_clk;

	dmc_local_timing_ptr = ddrc_local_timing_lpddr4;
	size = sizeof(ddrc_local_timing_lpddr4)/sizeof(ddrc_local_timing_lpddr4[0]);


	/*timing array */
	for(i = 0; i < 8; i++)
	{
		k = search_for_freq_point(dmc_local_timing_ptr, size, ddr_frequency[i].freq);
		if (k < 0)
		{
			/* Can not find timing parameters for this ddr_clk */
			return -1;
		}
		pdmc->dmc_dtmg_f[i][0]=dmc_local_timing_ptr[k].dtmg[0];
		pdmc->dmc_dtmg_f[i][1]=dmc_local_timing_ptr[k].dtmg[1];
		pdmc->dmc_dtmg_f[i][2]=dmc_local_timing_ptr[k].dtmg[2];
		pdmc->dmc_dtmg_f[i][3]=dmc_local_timing_ptr[k].dtmg[3];
		pdmc->dmc_dtmg_f[i][4]=dmc_local_timing_ptr[k].dtmg[4];
		pdmc->dmc_dtmg_f[i][5]=dmc_local_timing_ptr[k].dtmg[5];
		pdmc->dmc_dtmg_f[i][6]=dmc_local_timing_ptr[k].dtmg[6];
		pdmc->dmc_dtmg_f[i][7]=dmc_local_timing_ptr[k].dtmg[7];
		pdmc->dmc_dtmg_f[i][8]=dmc_local_timing_ptr[k].dtmg[8];
		pdmc->dmc_dtmg_f[i][9]=dmc_local_timing_ptr[k].dtmg[9];
		pdmc->dmc_dtmg_f[i][10]=dmc_local_timing_ptr[k].dtmg[10];
		pdmc->dmc_dtmg_f[i][11]=dmc_local_timing_ptr[k].dtmg[11];
		pdmc->dmc_dtmg_f[i][12]=dmc_local_timing_ptr[k].dtmg[12];
		pdmc->dmc_dtmg_f[i][13]=dmc_local_timing_ptr[k].dtmg[13];
		pdmc->dmc_dtmg_f[i][14]=dmc_local_timing_ptr[k].dtmg[14];
		pdmc->dmc_dtmg_f[i][15]=dmc_local_timing_ptr[k].dtmg[15];
		pdmc->dmc_dtmg_f[i][16]=dmc_local_timing_ptr[k].dtmg[16];
		pdmc->dmc_dtmg_f[i][17]=dmc_local_timing_ptr[k].dtmg[17];
		pdmc->dmc_dtmg_f[i][18]=dmc_local_timing_ptr[k].dtmg[18];
		pdmc->dmc_dtmg_f[i][19]=dmc_local_timing_ptr[k].dtmg[19];
	}
	return 0;
}


static unsigned char lpddr4_get_mr1_val(uint32 clk)
{
	uint32 val;

	if (clk <= 266)
	{
		val = 0x04;
	}
	else if (clk <= 533)
	{
		val = 0x14;
	}
	else if (clk <= 800)
	{
		val = 0x24;
	}
	else if (clk <= 1333)
	{
		val = 0x34;
	}
	else if (clk <= 1600)
	{
		val = 0x44;
	}
	else
	{
		val = 0x34;
	}

    return val;
}
static unsigned char lpddr4_get_mr2_val(uint32 clk)
{

	unsigned char val;
	if(clk <= 266)
	{
		val = 0;
	}
	else if (clk <= 533)
	{
		val = 1|(1<<3);
	}
	else if (clk <= 800)
	{
		val = 2|(2<<3);
	}
	else if (clk <= 1066)
	{
		val = 3|(3<<3);
	}
	else if (clk <= 1333)
	{
		val = 4|(4<<3);
	}
	else if (clk <= 1600)
	{
		val = 5|(5<<3);
	}
	else if (clk <= 1866)
	{
		val = 6|(6<<3);
	}
	else if (clk <= 2133)
	{
		val = 7|(7<<3);
	}
	else /*<=1066*/
	{
		val = 7|(7<<3);
	}
	return val;
}
static unsigned char lpddr4_get_mr3_val(clock)
{
	unsigned char mr3_val = 0x3;
	/*pdds*/
	mr3_val |= (0x6 << 3);
#ifdef RD_DBI_EN
	mr3_val |= (1 << 6);
#else
	mr3_val |= (0 << 6);
#endif
	mr3_val |= (1 << 7);

	return mr3_val;
}

static void mr_set_drv(int drv)
{
	unsigned char val;

	switch(drv)
	{
	case 34:
		val = 1;
		break;
	case 40:
		val = 2;
		break;
	case 48:
		val = 3;
		break;
	case 60:
		val = 4;
		break;
	case 80:
		val = 6;
		break;
	case 120:
		val = 7;
		break;
	default:
		val = 2;;
	}
	lpddr_dmc_mrw(CMD_CS_BOTH, 3, val);
}

static void mr_set_zq(void)
{
	lpddr_dmc_mrw(CMD_CS_0, 10, 0xff);
	dmc_sprd_delay(10);
	lpddr_dmc_mrw(CMD_CS_1, 10, 0xff);
}

static void mpc_set_zq(uint32 channel)
{
	DDRC_R1P1_REG_INFO_PTR pdmc = (DDRC_R1P1_REG_INFO_PTR)DMC_REG_ADDR_BASE_PHY;
	uint32 regval;
	/*enable channel xx software command.*/
	regval = pdmc->dmc_dcfg0;
	regval = u32_bits_set(regval, 20, 4, channel);
	pdmc->dmc_dcfg0 = regval;

	/*mpc zqcal start cs0*/
	pdmc->dmc_dcfg2 = 0x4f;
	pdmc->dmc_dcfg1 = 0x08000000;
	while((pdmc->dmc_dcfg1 >> 19 & 0x1ff) != 0);
	dmc_sprd_delay(2);

	/*mpc zqcal latch cs0*/
	pdmc->dmc_dcfg2 = 0x51;
	pdmc->dmc_dcfg1 = 0x08000000;
	while((pdmc->dmc_dcfg1 >> 19 & 0x1ff) != 0);
	dmc_sprd_delay(2);

	/*mpc zqcal start cs1*/
	pdmc->dmc_dcfg2 = 0x4f;
	pdmc->dmc_dcfg1 = 0x18000000;
	while((pdmc->dmc_dcfg1 >> 19 & 0x1ff) != 0);
	dmc_sprd_delay(2);

	/*mpc zqcal latch cs1*/
	pdmc->dmc_dcfg2 = 0x51;
	pdmc->dmc_dcfg1 = 0x18000000;
	while((pdmc->dmc_dcfg1 >> 19 & 0x1ff) != 0);

	pdmc->dmc_dcfg0 |= (0xf << 20);
}

void dram_init_mrws_lp4(uint32 clock)
{
	unsigned int regval;
	unsigned char mrval;
	uint32 i = 0;
	LPDDR4_ODT_CFG *p_odt_cfg = lpddr4x_odt_cfg; /* original fixed LPDDR4X path */

	lpddr4_mr_info.p_odt_cfg = p_odt_cfg;

	for(i = 0; i < 2; i++)
	{
		/*MR13*/
		mrval = ((1 - i) << 6) | ((1) << 3);
		lpddr_dmc_mrw(CMD_CS_BOTH, 13, mrval);

	/*MR2 RL&WL*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 2, (p_odt_cfg + phy_train.freq_sel)->mr2);

	/*MR1*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 1, (p_odt_cfg + phy_train.freq_sel)->mr1);

	 /*MR3 dbi en*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 3, (p_odt_cfg + phy_train.freq_sel)->mr3);

	/*MR11 odt set*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 11, (p_odt_cfg + phy_train.freq_sel)->mr11);

	 /*MR12*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 12, (p_odt_cfg + phy_train.freq_sel)->mr12);

	 /*MR14*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 14, (p_odt_cfg + phy_train.freq_sel)->mr14);

	 /*MR22*/
	lpddr_dmc_mrw(CMD_CS_BOTH, 22, (p_odt_cfg + phy_train.freq_sel)->mr22);
	dmc_sprd_delay(10);

	}
}

void dram_init_mrws(uint32 clock)
{
	dram_init_mrws_lp4(clock);
}
static void  lpddr4_powerup_seq(uint32 clock)
{
	DDRC_R1P1_REG_INFO_PTR pdmc = (DDRC_R1P1_REG_INFO_PTR)DMC_REG_ADDR_BASE_PHY;
	unsigned int regval;
	unsigned char mrval;

	/*1) **reset_n hold low*/
	dmc_sprd_delay(300);//tINIT0=200us(min)

	/*2)reset_n high*/
	pdmc->dmc_dcfg0 |= (1 << 15);
	dmc_sprd_delay(3000);//tINIT3=2ms+2us

	/*3) cke up*/
	pdmc->dmc_dcfg0 |= (1 << 14);
	dmc_sprd_delay(300);//tINIT5=2us
	dram_init_mrws(clock);

	/*rankx mpc -zqcal start*/
	mpc_set_zq(1);
	mpc_set_zq(2);

}
static void dram_power_up_set_post(void)
{
	/*PHY update requeset adn controllor ack counter value setting and cke
	 * ,reset_n SW control mode*/
	/*bit[16] pwrup_cke_rst_en*/
	reg_bits_set(DMC_GUCPHY0_BASE + 0xd1*4, 16, 1, 1);
	reg_bits_set(DMC_GUCPHY1_BASE + 0xd1*4, 16, 1, 1);
}
static void dram_init_power_up(uint32 clock)
{
	lpddr4_powerup_seq(clock);
}

void dram_powerup_seq(uint32 clock)
{
	if (dram_init_powerup_times == 0)
	{
		dram_init_power_up(clock);
		dram_power_up_set_post();
		if(FALSE == ddr_device_vaild_check(0))
		{
			ddrc_print_err("No DDR devices detect.\r\n");
		}
	}
	dram_init_mrws(clock);
	dram_init_powerup_times ++;

}
