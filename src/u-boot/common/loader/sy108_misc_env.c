/* SPDX-License-Identifier: GPL-2.0+
 * Recovered OEM misc format: CRC32 followed by sorted key=value NUL strings.
 * The measured reader does not reject a bad CRC. Only /tmp offline builds.
 */
#include <common.h>
#include <malloc.h>
#include <boot_mode.h>
#include <sprd_common_rw.h>
#include "loader_common.h"
#undef crc32 /* Linux headers alias this to raw crc32_le; OEM uses complemented U-Boot CRC. */
#include <u-boot/crc.h>
#define SY108_MISC_SIZE 0x100000
static unsigned char update_recovery;
struct misc_pair {char *key,*value;};
unsigned char sy108_is_update_recovery(void){return update_recovery;}
static int pair_cmp(const void *a,const void *b){return strcmp(((struct misc_pair *)a)->key,((struct misc_pair *)b)->key);}
int get_mode_from_file(void)
{
 char *raw=malloc(SY108_MISC_SIZE),*p,*end;struct misc_pair pairs[256];unsigned count=0,i,j;
 char *mode=NULL,*update=NULL;int result=CMD_UNKNOW_REBOOT_MODE;
 if(!raw)return result;
 if(common_raw_read("misc",SY108_MISC_SIZE,0,raw)){free(raw);return result;}
 p=raw+4;end=raw+SY108_MISC_SIZE;
 while(p<end&&*p&&count<ARRAY_SIZE(pairs)) {
  size_t n=strnlen(p,end-p);char *eq;
  if(n==(size_t)(end-p))break;
  eq=strchr(p,'=');
  if(eq){*eq=0;for(j=0;j<count;j++)if(!strcmp(pairs[j].key,p))break;
   pairs[j].key=p;pairs[j].value=eq+1;if(j==count)count++;
  }
  p+=n+1;
 }
 for(i=0;i<count;i++) {
  if(!strcmp(pairs[i].key,"mode"))mode=pairs[i].value;
  if(!strcmp(pairs[i].key,"update_recovery"))update=pairs[i].value;
 }
 if(mode) {
  result=0;
  if(!strncmp("boot-recovery",mode,13))result=CMD_RECOVERY_MODE;
  else if(!strncmp("update-radio",mode,12)) {
   char *out=calloc(1,SY108_MISC_SIZE+4);size_t used=4;
   result=CMD_UNKNOW_REBOOT_MODE;
   for(i=0;i<count;i++)if(!strcmp(pairs[i].key,"mode"))pairs[i].value="boot-recovery";
   qsort(pairs,count,sizeof(pairs[0]),pair_cmp);
   if(out) {
    for(i=0;i<count;i++) {
     size_t n=strlen(pairs[i].key)+strlen(pairs[i].value)+2;
     if(used+n>=SY108_MISC_SIZE)break;
     used+=sprintf(out+used,"%s=%s",pairs[i].key,pairs[i].value)+1;
    }
    *(u32 *)out=crc32(0,(u8 *)out+4,SY108_MISC_SIZE-4);
    do_raw_data_write("misc",SY108_MISC_SIZE,SY108_MISC_SIZE,0,out);
    free(out);
   }
   reboot_devices(0);
  }
 } else if(update&&*update=='1')update_recovery=1;
 free(raw);return result;
}
