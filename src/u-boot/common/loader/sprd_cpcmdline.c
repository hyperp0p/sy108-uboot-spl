#include "sprd_cpcmdline.h"
#include "loader_common.h"

char *g_CPcmdlineBuf = NULL;
#if defined( CONFIG_KERNEL_BOOT_CP )
char CPcmdlineBuf[MAX_CP_CMDLINE_LEN];
#endif

/* Original CP command whitelist: eighteen entries including NULL. */
static const char *cmd_arr[] = {
 BOOT_MODE, CALIBRATION_MODE, LTE_MODE, AP_VERSION, RF_BOARD_ID,
 RF_HW_INFO, K32_LESS, AUTO_TEST, CRYSTAL_TYPE, RF_HW_ID,
 PMIC_CHIP_ID, CHIPUID, MODEM_BOOT_METHOD, WCN_CLK_ID,
 "nrphyiqmem.status", PRODUCT_NAME, HARDWARE_ID, NULL
};

static bool is_invalid_cmd(const char *cmd)
{
	int i = 0;
	while(NULL != cmd_arr[i])
	{
		if(0 == strcmp(cmd_arr[i], cmd)){
			return true;
		}
		i++;
	}
	return false;
}

static char *cpcmdline_get_lte_mode(void)
{
	char *ltemode = NULL;

#ifdef CONFIG_SUPPORT_TDLTE
	ltemode = "tcsfb";
#elif defined CONFIG_SUPPORT_WLTE
	ltemode = "fcsfb";
#elif defined CONFIG_SUPPORT_LTE
	ltemode = "lcsfb";
#endif

	return ltemode;
}

static char *cpcmdline_get_modem_boot_method(void)
{
#ifdef CONFIG_DDR_BOOT
	return "ddrboot";
#elif defined CONFIG_EMMC_BOOT
	return "emmcboot";
#elif defined CONFIG_NAND_BOOT
	return "nandboot";
#endif
	return NULL;
}


static void cmdline_prepare(void)
{
#if defined( CONFIG_KERNEL_BOOT_CP )
	g_CPcmdlineBuf = CPcmdlineBuf;
#else
#ifdef CONFIG_MEM_LAYOUT_DECOUPLING
extern void *parse_cpcmdline_addr();
	g_CPcmdlineBuf = (char*)parse_cpcmdline_addr();
#else
#ifdef CALIBRATION_FLAG_CP0
	g_CPcmdlineBuf = (char*)CALIBRATION_FLAG_CP0;
#endif
#ifdef CALIBRATION_FLAG_CP1
	g_CPcmdlineBuf = (char*)CALIBRATION_FLAG_CP1;
#endif
#endif
#endif
	if (g_CPcmdlineBuf)
		memset(g_CPcmdlineBuf, 0, MAX_CP_CMDLINE_LEN);
	debugf("g_CPcmdlineBuf = 0x%p\n" , g_CPcmdlineBuf);
}

static void cmdline_add_cp_cmdline(char *cmd, char* value)
{
	char *p;
	int len;
	//printf("add cmd, cmd = %s, value = %s\n", cmd, value);

	if(!is_invalid_cmd(cmd))return;

	if (NULL == g_CPcmdlineBuf)
		return;

	len = strlen(g_CPcmdlineBuf);
	p = g_CPcmdlineBuf + len;

	sprintf(p, "%s=%s ", cmd, value);
	//printf("cmd = %s\n" , p);
}

void cp_cmdline_fixup(void)
{
	char *value = NULL;
	int boardid = 0;
	unsigned int pmicid = 0;
	int value_less = 0;
	char buf[30] = {0};
	unsigned char v32kless;
	int batid = 0;
	int lcmid = 0;
	unsigned int hw_version = -1;
	unsigned int wcn_sel = -1;

	cmdline_prepare();

	// androd boot mode
	value = getenv("bootmode");
	if (NULL != value)
	{
		cmdline_add_cp_cmdline(BOOT_MODE, value);
	}

	// calibration parameters
	value = get_calibration_parameter();
	if (NULL != value)
	{
		if (NULL != strstr(value, CALIBRATION_MODE))
		{
			// "calibration=%d,%d,146 ", must skipp calibration=
			value += strlen(CALIBRATION_MODE) + 1;
			cmdline_add_cp_cmdline(CALIBRATION_MODE, value);
			cmdline_add_cp_cmdline(BOOT_MODE,"cali");
		} else if (NULL != strstr(value, AUTO_TEST)) {
			// "autotest=1", must skipp autotest=
			value += strlen(AUTO_TEST) + 1;
			cmdline_add_cp_cmdline(AUTO_TEST, value);
		}
	}

	// lte mode
	value = cpcmdline_get_lte_mode();
	if(NULL != value)
	{
		cmdline_add_cp_cmdline(LTE_MODE, value);
	}

	//chip uid
#ifdef CONFIG_READ_UID
	{
		unsigned int block0;
		unsigned int block1;
		get_efuse_uid_ex(&block0,&block1);
		sprintf(buf,"0x%08x,0x%08x",block0,block1);
		cmdline_add_cp_cmdline(CHIPUID,buf);
	}
#endif
/* crystal and rf band auto adaption used on some platforms, such as sharkl, pike series */
/* Using RF band adaption in each board configuration and print information in cpcmdline*/
#if (defined(CONFIG_BOARD_ID) || defined(CONFIG_BAND_DETECT))
	//rf band auto adaption
	boardid = sprd_get_bandinfo();
	sprintf(buf,"%d",boardid);
	cmdline_add_cp_cmdline(RF_BOARD_ID, buf);
 { static const char product_name[][15]={"Dorado","SY108-618","SY108-658","SY108-688"};
   int id=sprd_get_versionid();
   printf("mg_board_id = %d\n",id);
   sprintf(buf,"%s",product_name[id]);
   cmdline_add_cp_cmdline(PRODUCT_NAME,(char *)product_name[id]);
   sprintf(buf,"%d",sprd_get_boardid());cmdline_add_cp_cmdline(HARDWARE_ID,buf);
 }
#endif

#ifdef CONFIG_BAT_ID
/* add bat id in cpcmdline*/
	batid = sprd_get_batid();
	sprintf(buf, "%d", batid);
	cmdline_add_cp_cmdline(BAT_ID, buf);
#endif

#ifdef CONFIG_LCM_ID
/* add lcm id in cpcmdline*/
	lcmid = sprd_get_lcmid();
	sprintf(buf, "%d", lcmid);
	cmdline_add_cp_cmdline(LCM_ID, buf);
#endif

#ifdef CONFIG_HW_VERSION
/* get wcn sel*/
	hw_version = sprd_get_hw_version();
	sprintf(buf,"%d",hw_version);
	cmdline_add_cp_cmdline(HW_VERSION, buf);
#endif

#ifdef CONFIG_WCN_SEL
/* get hw version*/
	wcn_sel = sprd_get_wcn_sel();
	sprintf(buf,"%d",wcn_sel);
	cmdline_add_cp_cmdline(WCN_SEL, buf);
#endif

/* Printing the clock and crystal type in cpcmdline by clk & crystal adaption */
#if (defined(CONFIG_BOARD_ID) || defined(CONFIG_ADIE_SC2730))
	//board id
	value_less=sprd_get_boardid();
	sprintf(buf,"%d",value_less);
	cmdline_add_cp_cmdline(RF_HW_ID, buf);

	//26m crystal auto adaption
	sprintf(buf,"%d", sprd_get_crystal());
	cmdline_add_cp_cmdline(CRYSTAL_TYPE, buf);
#endif
	//32k crystal auto adaption
	sprintf(buf,"%d", sprd_get_32k());
	cmdline_add_cp_cmdline(K32_LESS, buf);
#ifdef CONFIG_WCN_DETECT
	//wcn crystal auto adaption
	sprintf(buf,"%d", sprd_get_wcn_crystal());
	cmdline_add_cp_cmdline(WCN_CLK_ID,buf);
#endif

#ifdef CONFIG_SC9833
	{
		extern u8 DRV_RF_Get_Type(void);

		u8 type = DRV_RF_Get_Type();
		sprintf(buf, "%d", type);
		cmdline_add_cp_cmdline(RF_CHIP_ID, buf);
	}
#endif

#ifdef CONFIG_PMIC_CHIP_ID
	/* get pmic chipid */
	extern int sprd_get_pmic_chipid(void);
	pmicid = sprd_get_pmic_chipid();
	sprintf(buf,"%x",pmicid);
	cmdline_add_cp_cmdline(PMIC_CHIP_ID, buf);
#endif

	extern unsigned char sy108_get_nr_iq_status(void);
	sprintf(buf,"%d",sy108_get_nr_iq_status());
	cmdline_add_cp_cmdline("nrphyiqmem.status",buf);

	// modem boot method
	value = cpcmdline_get_modem_boot_method();
	if(NULL != value)
	{
		cmdline_add_cp_cmdline(MODEM_BOOT_METHOD, value);
	}
	debugf("cp cmdline: %s\n", g_CPcmdlineBuf);
}

char *cp_getcmdline(void)
{
	return g_CPcmdlineBuf;
}
