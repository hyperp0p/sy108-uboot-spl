/* SPDX-License-Identifier: MIT */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "capsule.h"
extern int gunzip(void *,int,unsigned char *,unsigned long *);
/* Stub watchdog reset for userspace capsule decompression test. */
void hw_watchdog_reset(void) {}

static void policies(void)
{
 struct { unsigned state,stage,mode,wdt,choice; } cases[]={
  {1,0xffff,0,0,2}, {1,0,0,0,2}, {1,0xbe5a,0,0,2},
  {1,0xbe5a,0,1,1}, {1,0xbe5a,8,1,1}, {1,0xbc5a,0,0,1},
  {1,0xbc5a,3,1,1}, {1,0xb65a,2,1,2}, {1,0xbe5a,2,0,2},
  {4,0xb65a,2,1,2}, {4,0xbe5a,2,0,1}, {4,0xffff,0,0,1},
  {0,0xb65a,2,0,1}, {2,0xb65a,2,0,1}, {3,0xb65a,2,0,1}
 };
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++)
  assert(udx_boot_select(cases[i].state,0,cases[i].stage,cases[i].mode,cases[i].wdt)==cases[i].choice);
 const unsigned stages[]={0xb45a,0xb05a,0xb25a,0xb85a,0xba5a,0xbc5a};
 for(unsigned i=0;i<sizeof(stages)/sizeof(stages[0]);i++)
  assert(udx_boot_select(UDX_UPGRADE_READY,0,stages[i],0,0)==UDX_BOOT_CHOICE_RECOVERY);
 assert(udx_boot_select(UDX_UPGRADE_READY,-1,0xbe5a,2,0)==UDX_BOOT_CHOICE_RECOVERY);
 unsigned char *misc=calloc(1,UDX_MISC_ENV_BYTES);
 const char *states[]={"ready","pending","recovery","trial"};
 for(unsigned i=0;i<4;i++){
  memset(misc,0,UDX_MISC_ENV_BYTES);
  sprintf((char *)misc+4,"udx_boot_state=%s",states[i]);
  unsigned crc=udx_crc32(misc+4,UDX_MISC_ENV_BYTES-4);
  for(unsigned j=0;j<4;j++)misc[j]=crc>>(8*j);
  assert(udx_misc_state(misc,UDX_MISC_ENV_BYTES,0)==i+1);
  misc[80]^=1;assert(udx_misc_state(misc,UDX_MISC_ENV_BYTES,0)==UDX_UPGRADE_INVALID);
 }
 free(misc);
 puts("POLICY_CASES=30");
}
int main(int argc,char **argv)
{
 if(argc==1){policies();return 0;}
 if(argc!=3)return 2;
 FILE *f=fopen(argv[1],"rb");if(!f)return 3;
 fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);
 void *data=malloc(n);if(!data || fread(data,1,n,f)!=(size_t)n)return 4;
 fclose(f);
 struct udx_boot_parts parts;
 int ret=udx_boot_parts(data,n,atoi(argv[2]),&parts);
 if(!ret){
  void *image=malloc(parts.capsule.image_memory_size);
  if(!image)return 5;
  ret=udx_capsule_decode(&parts,image,parts.capsule.image_memory_size,gunzip);
  printf("DECODE_RESULT=%d IMAGE_CRC=%08x\n",ret,udx_crc32(image,parts.capsule.image_size));
  free(image);
 }
 printf("CAPSULE_RESULT=%d\n",ret);
 free(data);
 return ret?1:0;
}
