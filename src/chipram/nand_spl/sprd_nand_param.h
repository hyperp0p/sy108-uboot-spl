/* SPDX-License-Identifier: GPL-2.0-only
 * Recovered parameter facts from the exact original U-Boot payload.
 * Layout and every parameter-table byte checked against the original.
 * This is a reconstruction, not the missing vendor-generated source header.
 */
#ifndef _SY108_RECOVERED_NAND_PARAM_H
#define _SY108_RECOVERED_NAND_PARAM_H
#include <linux/types.h>
#define BW_08 0
#define BW_16 1
struct sprd_nand_timing { u8 ace_ns, rwl_ns, rwh_ns; };
struct sprd_nand_oob { u16 nOOBSize; u8 nEccBits; u16 nEccPos, nEccSize, nInfoPos, nInfoSize; };
struct sprd_nand_param {
 u8 id[5], idmaker, id_device;
 u32 nBlkSize, nBlkNum, nPageSize;
 u16 nSecSize, nSpareSize;
 u8 nBusWidth, nCycles;
 struct sprd_nand_timing sTiming;
 struct sprd_nand_oob sOOB;
};
typedef char recovered_nand_record_must_be_44_bytes[(sizeof(struct sprd_nand_param)==44)?1:-1];

/* Descriptor strings, per-image order and table sharing recovered independently.
 * These are data initializers and source-level lookup/check routines.
 */
struct recovered_nand_device { u8 id; const char *name, *description; };
struct recovered_nand_maker { u8 id; const char *name; struct recovered_nand_device *devices; };
/* Original generated NAND headers pooled all descriptor literals in one
 * read-only region. Some old model tables were removed by --gc-sections while
 * their strings remained in that same region. Keep the complete recovered
 * descriptor data, without inventing the discarded device-table records. */
static const char spl_nand_descriptor_strings[]
 __attribute__((section(".rodata"),aligned(1))) =
 "KSLxxxxx\0"
 "1Gb,x08,1.8V\0"
 "KSLCCBL2GA2H2A\0"
 "2Gb,x08,1.8V\0"
 "F59D4G81KA \0"
 "4Gb,x08,1.8V\0"
 "F50D1G41MYB\0"
 "FM6BD1G1GMB\0"
 "F59D2G81KASN1\0"
 "F50D1G41LB-66YIC2M\0"
 "W29N01HZ\0"
 "W71NW20GF3FW\0"
 "NCSPN4N2A\0"
 "FMN2ET2BCJ-18IH\0"
 "FMN4ET2TCM-25EF\0"
 "JSFCBB3YH3FBG-425A\0"
 "JSFCBH3YHHBBL\0"
 "JSFCBF3YHHBBL\0"
 "JS27AP4G15SD \0"
 "JSFAAA3YHABBE-425\0"
 "MT29RZ4B2DZZHHWD\0"
 "MT29RZ4B2DZZHGSK\0"
 "MT29F4G16ABBEA\0"
 "GD9FS4G8F3ALGI\0"
 "GD5FxGQ4xBxIG\0"
 "MX30UF2G18AC\0"
 "APZZLA4GAZ-N22MAB\0"
 "PN611G8C2TA-B8BI0\0"
 "S34MS02G2\0"
 "TC58NYG1S3HBAI6\0"
 "NM1482KSLAXCL-3B\0"
 "Micron\0"
 "XTX\0"
 "JSC\0"
 "ESMT\0"
 "MXIC\0"
 "Winbond\0"
 "SkyHigh";
#define SPL_NAND_STRING(offset) (spl_nand_descriptor_strings + (offset))

struct recovered_nand_device micron_device_table[];
struct recovered_nand_device xtx_device_table[];
struct recovered_nand_device jsc_device_table[];
struct recovered_nand_device esmt_device_table[];
struct recovered_nand_device mxic_device_table[];
struct recovered_nand_device winbond_device_table[];
struct recovered_nand_device skyhigh_device_table[];
struct recovered_nand_device esmt_device_table[] = {
 {172, SPL_NAND_STRING(50), SPL_NAND_STRING(62)},
 {97, SPL_NAND_STRING(75), SPL_NAND_STRING(9)},
 {97, SPL_NAND_STRING(87), SPL_NAND_STRING(9)},
 {90, SPL_NAND_STRING(99), SPL_NAND_STRING(37)},
 {17, SPL_NAND_STRING(113), SPL_NAND_STRING(9)},
 {0, 0, 0}
};
struct recovered_nand_device jsc_device_table[] = {
 {172, SPL_NAND_STRING(196), SPL_NAND_STRING(62)},
 {172, SPL_NAND_STRING(215), SPL_NAND_STRING(62)},
 {188, SPL_NAND_STRING(229), SPL_NAND_STRING(37)},
 {188, SPL_NAND_STRING(243), SPL_NAND_STRING(37)},
 {161, SPL_NAND_STRING(257), SPL_NAND_STRING(9)},
 {0, 0, 0}
};
struct recovered_nand_maker maker_table[] = {
 {44, SPL_NAND_STRING(445), micron_device_table},
 {152, SPL_NAND_STRING(452), xtx_device_table},
 {236, SPL_NAND_STRING(456), jsc_device_table},
 {140, SPL_NAND_STRING(460), esmt_device_table},
 {200, SPL_NAND_STRING(460), esmt_device_table},
 {194, SPL_NAND_STRING(465), mxic_device_table},
 {173, SPL_NAND_STRING(456), jsc_device_table},
 {239, SPL_NAND_STRING(470), winbond_device_table},
 {1, SPL_NAND_STRING(478), skyhigh_device_table},
 {0, 0, 0}
};
struct recovered_nand_device micron_device_table[] = {
 {172, SPL_NAND_STRING(275), SPL_NAND_STRING(62)},
 {172, SPL_NAND_STRING(292), SPL_NAND_STRING(62)},
 {188, SPL_NAND_STRING(309), SPL_NAND_STRING(37)},
 {0, 0, 0}
};
struct recovered_nand_device mxic_device_table[] = {
 {170, SPL_NAND_STRING(353), SPL_NAND_STRING(37)},
 {0, 0, 0}
};
struct recovered_nand_device skyhigh_device_table[] = {
 {170, SPL_NAND_STRING(402), SPL_NAND_STRING(37)},
 {0, 0, 0}
};
struct sprd_nand_param sprd_nand_param_table[] = {
 { {0x2c,0xbc,0x90,0x66,0x54}, 44,188,131072,2048,2048,512,112,0,5, {10,15,10}, {28,8,14,14,14,0} },
 { {0x98,0xbc,0x90,0x66,0x76}, 152,188,131072,2048,2048,512,112,0,5, {10,20,20}, {28,8,14,14,14,0} },
 { {0xad,0xbc,0x90,0x55,0x56}, 173,188,65536,4096,1024,512,32,0,5, {10,20,20}, {16,4,9,7,9,0} },
 { {0xc8,0xbc,0x90,0x55,0x54}, 200,188,65536,4096,1024,512,32,0,5, {10,20,20}, {16,4,9,7,9,0} },
 { {0x98,0xba,0x90,0x55,0x76}, 152,186,65536,2048,1024,512,64,0,5, {10,20,20}, {32,8,18,14,18,0} },
 { {0xec,0xbc,0x0,0x6a,0x56}, 236,188,131072,2048,2048,512,128,0,5, {10,20,20}, {32,8,18,14,18,0} },
 { {0x1,0xbc,0x90,0x55,0x56}, 1,188,65536,4096,1024,512,32,0,5, {10,20,20}, {16,4,9,7,9,0} },
 { {0x98,0xac,0x90,0x26,0x76}, 152,172,262144,2048,4096,512,256,0,5, {30,20,20}, {32,8,18,14,18,0} },
 { {0x1,0xac,0x90,0x15,0x56}, 1,172,131072,4096,2048,512,64,0,5, {10,20,20}, {16,4,9,7,9,0} },
 { {0x2c,0xac,0x90,0x15,0x56}, 44,172,131072,4096,2048,512,64,0,5, {35,20,20}, {16,4,9,7,9,0} },
 { {0x2c,0xac,0x90,0x26,0x54}, 44,172,262144,2048,4096,512,224,0,5, {35,15,15}, {28,8,14,14,14,0} },
 { {0x98,0xaa,0x90,0x15,0x76}, 152,170,131072,2048,2048,512,128,0,5, {30,15,10}, {32,8,18,14,18,0} },
 { {0x98,0xa1,0x80,0x15,0x72}, 152,161,131072,1024,2048,512,128,0,5, {30,15,10}, {32,8,18,14,18,0} },
 { {0xc8,0x61,0x80,0x15,0x40}, 200,97,131072,1024,2048,512,64,0,5, {30,25,20}, {16,4,9,7,9,0} },
 { {0xad,0xa1,0x80,0x15,0xad}, 173,161,131072,1024,2048,512,64,0,5, {30,30,20}, {16,4,9,7,9,0} },
 { {0xef,0xa1,0x0,0x95,0x0}, 239,161,131072,1024,2048,512,64,0,5, {10,15,15}, {16,4,9,7,9,0} },
 { {0xef,0xaa,0x90,0x15,0x4}, 239,170,131072,2048,2048,512,64,0,5, {40,15,10}, {16,4,9,7,9,0} },
 { {0xad,0xaa,0x90,0x15,0x46}, 173,170,131072,2048,2048,512,64,0,5, {30,30,20}, {16,4,9,7,9,0} },
 { {0xad,0xac,0x80,0x16,0x20}, 173,172,262144,2048,4096,512,128,0,5, {30,30,20}, {16,4,9,7,9,0} },
 { {0xc8,0xc1,0x0,0x0,0x0}, 200,193,131072,1024,2048,512,128,0,5, {10,25,15}, {32,8,4,14,4,0} },
 { {0xc8,0x11,0x0,0x0,0x0}, 200,17,131072,1024,2048,512,64,0,5, {10,25,15}, {16,4,9,7,9,0} },
 { {0x8c,0x61,0x0,0x0,0x0}, 140,97,131072,1024,2048,512,64,0,5, {10,25,15}, {16,4,9,7,9,0} },
 { {0x1,0xaa,0x90,0x15,0x46}, 1,170,131072,2048,2048,512,128,0,5, {10,25,15}, {32,8,18,14,18,0} },
 { {0xad,0xac,0x90,0x15,0x56}, 173,172,131072,4096,2048,512,128,0,5, {10,25,15}, {32,4,25,7,25,0} },
 { {0xc2,0xaa,0x90,0x15,0x6}, 194,170,131072,2048,2048,512,64,0,5, {10,15,15}, {16,4,9,7,9,0} },
 { {0xc8,0x5a,0x90,0x4,0x34}, 200,90,131072,2048,2048,512,128,0,5, {32,12,10}, {32,8,18,14,18,0} },
 { {0xc8,0xac,0x90,0x15,0x56}, 200,172,131072,4096,2048,512,64,0,5, {10,20,20}, {16,4,9,7,9,0} },
 { {0xc8,0xac,0x80,0x19,0x30}, 200,172,262144,2048,4096,512,256,0,5, {5,12,10}, {32,8,18,14,18,0} },
 { {0x0,0x0,0x0,0x0,0x0}, 0,0,262144,2048,4096,512,128,1,5, {10,21,15}, {16,8,2,14,0,0} },
};
struct recovered_nand_device winbond_device_table[] = {
 {161, SPL_NAND_STRING(132), SPL_NAND_STRING(9)},
 {170, SPL_NAND_STRING(141), SPL_NAND_STRING(37)},
 {0, 0, 0}
};
struct recovered_nand_device xtx_device_table[] = {
 {161, SPL_NAND_STRING(384), SPL_NAND_STRING(9)},
 {0, 0, 0}
};

#define CALC_ECC_SIZE(n) (((n)*14+7)/8)
STATIC_FUNC struct recovered_nand_maker* __FindMaker(uint8_t idMaker) {
	struct recovered_nand_maker* pmaker = maker_table;

	while (pmaker->id != 0) {
		if(pmaker->id == idMaker) {
			return pmaker;
		}
		pmaker++;
	}
	return NULL;
}

STATIC_FUNC struct recovered_nand_device* __FindDevice(struct recovered_nand_maker* pmaker, uint8_t idDevice) {
	struct recovered_nand_device* pdevice = pmaker->devices;

	while (pdevice->id != 0) {
		if(pdevice->id == idDevice) {
			return pdevice;
		}
		pdevice++;
	}
	return NULL;
}

STATIC_FUNC void __PrintNandInfo(struct sprd_nand_param* p) {
	struct recovered_nand_device* pdevice;
	struct recovered_nand_maker* pmaker;

	DPRINT("%s\n", __func__);

	pmaker = __FindMaker(p->idmaker);
	pdevice = __FindDevice(pmaker, p->id_device);
	
	DPRINT("device is %s:%s\n", pmaker->name, pdevice->name);
	DPRINT("block size is %d\n", p->nBlkSize);
	DPRINT("page size is %d\n", p->nPageSize);
	DPRINT("spare size is %d\n", p->nSpareSize);
	DPRINT("eccbits is %d\n", p->sOOB.nEccBits);
}


STATIC_FUNC void __ParseNandParam(uint8_t* id) {
	struct sprd_nand_param* param = sprd_nand_param_table;

	while(param->idmaker != 0) {
		if ((param->id[0] == id[0]) && 
			(param->id[4] == id[1])) {
			
			__PrintNandInfo(param);
			return param;
		}
		param++;
	}
	
	return;
}

STATIC_FUNC int __SprdCheckNancParam(struct sprd_nand_param* param) {
	struct recovered_nand_device* pdevice;
	struct recovered_nand_maker* pmaker;
	struct sprd_nand_oob* poob = &param->sOOB;
	uint8_t sector_num = param->nPageSize / param->nSecSize;

	DPRINT("%s\n", __func__);

	if (param->idmaker != param->id[0]) {
		DPRINT("%s: id is not match!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}
	if (param->id_device != param->id[1]) {
		DPRINT("%s: id is not match!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nOOBSize * sector_num > param->nSpareSize) {
		DPRINT("%s: OOB size is not match!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nEccPos + poob->nEccSize > poob->nOOBSize) {
		DPRINT("%s: ECC size exceeds the oob!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}
	if (CALC_ECC_SIZE(poob->nEccBits) != poob->nEccSize) {
		DPRINT("%s: ECC size is wrong!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}
	if (poob->nInfoPos + poob->nInfoSize != poob->nEccPos) {
		DPRINT("%s: Info position is wrong!!!!!!!!!!!!!!!\n", __func__);
		SPRD_ASSERT(0);
	}	

	return 0;	
}

struct sprd_nand_param* SprdGetNandParam(uint8_t* id) {
	struct sprd_nand_param* param = sprd_nand_param_table;
	int i;

	DPRINT("%s\n", __func__);

	for (i = 0; i < 5; i++) {
		DPRINT("id[%d] is %x\n", i, id[i]);
	}

	//__ParseNandParam(id);

	while(param->idmaker != 0) {
		if ((param->id[0] == id[0]) && 
			(param->id[1] == id[1]) &&
			(param->id[2] == id[2]) &&
			(param->id[3] == id[3]) &&
			(param->id[4] == id[4])) {

			__PrintNandInfo(param);
			__SprdCheckNancParam(param);
			return param;
		}
		param++;
	}
	DPRINT("Nand params unconfigured, please check it. Halt on booting!!!\n");
	for(;;) /*relax*/;

	return NULL;
}

#endif
