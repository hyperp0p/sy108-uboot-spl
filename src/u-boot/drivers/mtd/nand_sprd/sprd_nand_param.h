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


/* Preserve the complete generated descriptor literal pool as mergeable
 * strings. Every live reference is a constant relocation to a string atom;
 * no runtime code indexes across string boundaries. The section suffix tells
 * GNU as to retain SHF_MERGE/SHF_STRINGS with an entry size of one byte. */
static const char uboot_nand_strings[] __attribute__((used,section(".rodata.str1.1,\"aMS\",%progbits,1 //"),aligned(1))) =
 "_inst_nop\0"
 "_inst_readId\0"
 "JSC\0"
 "MXIC\0"
 "Micron\0"
 "XTX\0"
 "ESMT\0"
 "SkyHigh\0"
 "Winbond\0"
 "NM1482KSLAXCL-3B\0"
 "4Gb,x08,1.8V\0"
 "W29N01HZ\0"
 "1Gb,x08,1.8V\0"
 "W71NW20GF3FW\0"
 "2Gb,x08,1.8V\0"
 "APZZLA4GAZ-N22MAB\0"
 "MX30UF2G18AC\0"
 "PN611G8C2TA-B8BI0\0"
 "TC58NYG1S3HBAI6\0"
 "S34MS02G2\0"
 "FMN4ET2TCM-25EF\0"
 "FMN2ET2BCJ-18IH\0"
 "NCSPN4N2A\0"
 "F50D1G41MYB\0"
 "FM6BD1G1GMB\0"
 "F59D4G81KA \0"
 "F50D1G41LB-66YIC2M\0"
 "F59D2G81KASN1\0"
 "GD5FxGQ4xBxIG\0"
 "GD9FS4G8F3ALGI\0"
 "JSFCBF3YHHBBL\0"
 "JSFCBB3YH3FBG-425A\0"
 "JS27AP4G15SD \0"
 "JSFCBH3YHHBBL\0"
 "JSFAAA3YHABBE-425\0"
 "KSLxxxxx\0"
 "KSLCCBL2GA2H2A\0"
 "MT29F4G16ABBEA\0"
 "MT29RZ4B2DZZHHWD\0"
 "MT29RZ4B2DZZHGSK";
#define UBOOT_NAND_STRING(offset) (uboot_nand_strings+(offset))

STATIC_FUNC struct recovered_nand_device jsc_device_table[];
STATIC_FUNC struct recovered_nand_device mxic_device_table[];
STATIC_FUNC struct recovered_nand_device micron_device_table[];
STATIC_FUNC struct recovered_nand_device xtx_device_table[];
STATIC_FUNC struct recovered_nand_device esmt_device_table[];
STATIC_FUNC struct recovered_nand_device skyhigh_device_table[];
STATIC_FUNC struct recovered_nand_device winbond_device_table[];
STATIC_FUNC struct recovered_nand_device esmt_device_table[] = {
 {97, UBOOT_NAND_STRING(259), UBOOT_NAND_STRING(103)},
 {97, UBOOT_NAND_STRING(271), UBOOT_NAND_STRING(103)},
 {172, UBOOT_NAND_STRING(283), UBOOT_NAND_STRING(81)},
 {17, UBOOT_NAND_STRING(295), UBOOT_NAND_STRING(103)},
 {90, UBOOT_NAND_STRING(314), UBOOT_NAND_STRING(129)},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_device jsc_device_table[] = {
 {188, UBOOT_NAND_STRING(357), UBOOT_NAND_STRING(129)},
 {172, UBOOT_NAND_STRING(371), UBOOT_NAND_STRING(81)},
 {188, UBOOT_NAND_STRING(390), UBOOT_NAND_STRING(129)},
 {172, UBOOT_NAND_STRING(404), UBOOT_NAND_STRING(81)},
 {161, UBOOT_NAND_STRING(418), UBOOT_NAND_STRING(103)},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_maker maker_table[] = {
 {236, UBOOT_NAND_STRING(23), jsc_device_table},
 {173, UBOOT_NAND_STRING(23), jsc_device_table},
 {194, UBOOT_NAND_STRING(27), mxic_device_table},
 {44, UBOOT_NAND_STRING(32), micron_device_table},
 {152, UBOOT_NAND_STRING(39), xtx_device_table},
 {140, UBOOT_NAND_STRING(43), esmt_device_table},
 {200, UBOOT_NAND_STRING(43), esmt_device_table},
 {1, UBOOT_NAND_STRING(48), skyhigh_device_table},
 {239, UBOOT_NAND_STRING(56), winbond_device_table},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_device micron_device_table[] = {
 {188, UBOOT_NAND_STRING(460), UBOOT_NAND_STRING(129)},
 {172, UBOOT_NAND_STRING(475), UBOOT_NAND_STRING(81)},
 {172, UBOOT_NAND_STRING(492), UBOOT_NAND_STRING(81)},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_device mxic_device_table[] = {
 {170, UBOOT_NAND_STRING(160), UBOOT_NAND_STRING(129)},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_device skyhigh_device_table[] = {
 {170, UBOOT_NAND_STRING(207), UBOOT_NAND_STRING(129)},
 {0, 0, 0}
};
STATIC_FUNC struct sprd_nand_param sprd_nand_param_table[] = {
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
STATIC_FUNC struct recovered_nand_device winbond_device_table[] = {
 {161, UBOOT_NAND_STRING(94), UBOOT_NAND_STRING(103)},
 {170, UBOOT_NAND_STRING(116), UBOOT_NAND_STRING(129)},
 {0, 0, 0}
};
STATIC_FUNC struct recovered_nand_device xtx_device_table[] = {
 {161, UBOOT_NAND_STRING(173), UBOOT_NAND_STRING(103)},
 {0, 0, 0}
};

STATIC_FUNC struct recovered_nand_maker *__FindNandMaker(u8 id)
{
 struct recovered_nand_maker *p=maker_table;
 for(;p->id;p++) if(p->id==id) return p;
 return 0;
}
STATIC_FUNC struct recovered_nand_device *__FindNandDevice(struct recovered_nand_maker *maker,u8 id)
{
 struct recovered_nand_device *p=maker->devices;
 for(;p->id;p++) if(p->id==id) return p;
 return 0;
}
STATIC_FUNC void __PrintNandInfo(struct sprd_nand_param *p)
{
 struct recovered_nand_maker *maker;
 struct recovered_nand_device *device;
 printf("%s\n",__func__);
 maker=__FindNandMaker(p->idmaker);
 device=__FindNandDevice(maker,p->id_device);
 if(!device) printf("idMaker (%02x) is in conflict with other\n",p->idmaker);
 else printf("device is %s:%s\n",maker->name,device->name);
 printf("block size is %d\n",p->nBlkSize);
 printf("page size is %d\n",p->nPageSize);
 printf("spare size is %d\n",p->nSpareSize);
 printf("eccbits is %d\n",p->sOOB.nEccBits);
}
STATIC_FUNC int __SprdCheckNancParam(struct sprd_nand_param *p)
{
 struct sprd_nand_oob *oob=&p->sOOB;
 u8 sectors=p->nPageSize/p->nSecSize;
 printf("%s\n",__func__);
 if(p->idmaker!=p->id[0]) {printf("%s: id is not match!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 if(p->id_device!=p->id[1]) {printf("%s: id is not match!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 if(sectors*oob->nOOBSize>p->nSpareSize) {printf("%s: OOB size is not match!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 if(oob->nOOBSize<oob->nEccPos+oob->nEccSize) {printf("%s: ECC size exceeds the oob!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 if(oob->nEccSize!=(oob->nEccBits*14+7)/8) {printf("%s: ECC size is wrong!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 if(oob->nEccPos!=oob->nInfoPos+oob->nInfoSize) {printf("%s: Info position is wrong!!!!!!!!!!!!!!!\n",__func__);for(;;);}
 return 0;
}
struct sprd_nand_param *SprdGetNandParam(u8 *id)
{
 struct sprd_nand_param *p=sprd_nand_param_table;
 unsigned i;
 printf("%s\n",__func__);
 for(i=0;i<5;i++) printf("id[%d] is %x\n",i,id[i]);
 for(;p->idmaker;p++) {
  if(p->id[0]==id[0] && p->id[1]==id[1] && p->id[2]==id[2] &&
     p->id[3]==id[3] && p->id[4]==id[4]) break;
 }
 if(!p->idmaker) {printf("Nand params unconfigured, please check it. Halt on booting!!!\n");for(;;);}
 __PrintNandInfo(p);
 __SprdCheckNancParam(p);
 return p;
}
#endif
