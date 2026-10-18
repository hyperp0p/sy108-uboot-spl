/* SPDX-License-Identifier: MIT */
/* Parse the misc transaction and select the boot target. */
#include "boot-policy.h"
uint32_t udx_crc32(const void *data,size_t size)
{
 const unsigned char *p=data;uint32_t crc=~0u;
 for(size_t i=0;i<size;i++){
  crc^=p[i];
  for(unsigned bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));
 }
 return ~crc;
}
static int equal(const unsigned char *p,size_t n,const char *s)
{
 size_t i=0;while(s[i]){if(i>=n||p[i]!=(unsigned char)s[i])return 0;i++;}
 return i==n;
}
enum udx_upgrade_state udx_misc_state(const void *data,size_t size,int read_error)
{
 const unsigned char *p=data;
 if(read_error||!p||size!=UDX_MISC_ENV_BYTES)return UDX_UPGRADE_INVALID;
 uint32_t expected=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
 if(udx_crc32(p+4,size-4)!=expected)return UDX_UPGRADE_INVALID;
 enum udx_upgrade_state state=UDX_UPGRADE_INVALID;
 unsigned found=0;size_t pos=4;
 while(pos<size){
  if(!p[pos])return pos>4&&p[pos-1]==0&&found==1?state:UDX_UPGRADE_INVALID;
  size_t end=pos,sep=pos;
  while(end<size&&p[end])end++;
  if(end==size)return UDX_UPGRADE_INVALID;
  while(sep<end&&p[sep]!='=')sep++;
  if(sep==pos||sep==end)return UDX_UPGRADE_INVALID;
  if(equal(p+pos,sep-pos,UDX_BOOT_STATE_KEY)){
   if(found++)return UDX_UPGRADE_INVALID;
   const unsigned char *v=p+sep+1;size_t n=end-sep-1;
   if(equal(v,n,"ready"))state=UDX_UPGRADE_READY;
   else if(equal(v,n,"trial"))state=UDX_UPGRADE_TRIAL;
   else if(equal(v,n,"pending"))state=UDX_UPGRADE_PENDING;
   else if(equal(v,n,"recovery"))state=UDX_UPGRADE_RECOVERY;
   else return UDX_UPGRADE_INVALID;
  }
  pos=end+1;
 }
 return UDX_UPGRADE_INVALID;
}
enum udx_boot_choice udx_boot_select(enum udx_upgrade_state state,int rtc_error,
                                    unsigned stage,unsigned mode,unsigned watchdog_pending)
{
 if(rtc_error)return UDX_BOOT_CHOICE_RECOVERY;
 /* A completed flash transaction permits one explicitly requested NORMAL
  * boot. The entry marker is replaced before any image is read. Until live
  * userspace accepts it, a cold boot or any failed attempt selects Recovery.
  * Boot selection leaves the misc transaction unchanged. */
 if(state==UDX_UPGRADE_TRIAL)
  return mode==2&&stage==0xb65a?UDX_BOOT_CHOICE_OPENWRT:UDX_BOOT_CHOICE_RECOVERY;
 if(state!=UDX_UPGRADE_READY)return UDX_BOOT_CHOICE_RECOVERY;
 if(mode==0){
  /* Retained production attempt stages are evidence of failure even when
   * original firmware has already consumed the watchdog reason. Unknown
   * or erased RTC contents still allow an accepted system to cold start. */
  unsigned attempt=(stage&0xf1ffu)==0xb05au;
  return watchdog_pending||(attempt&&stage!=0xbe5a&&stage!=0xb65a)?
   UDX_BOOT_CHOICE_RECOVERY:UDX_BOOT_CHOICE_OPENWRT;
 }
 if(mode!=2)return UDX_BOOT_CHOICE_RECOVERY;
 if(stage!=0xb65a&&stage!=0xbe5a)return UDX_BOOT_CHOICE_RECOVERY;
 return UDX_BOOT_CHOICE_OPENWRT;
}
static int root_argument(const unsigned char *p,size_t n)
{
 static const char *const prefixes[]={"root=","rootfstype=","rootflags=","rootdelay=",
  "rootwait=","ubi.mtd=","ubi.block=","rdinit=","init="};
 if(n&&p[0]=='"'){p++;n--;if(n&&p[n-1]=='"')n--;}
 if(equal(p,n,"ro")||equal(p,n,"rw")||equal(p,n,"rootwait"))return 1;
 for(unsigned i=0;i<sizeof(prefixes)/sizeof(prefixes[0]);i++){
  size_t j=0;while(prefixes[i][j]&&j<n&&p[j]==(unsigned char)prefixes[i][j])j++;
  if(!prefixes[i][j])return 1;
 }
 return 0;
}
int udx_bootargs_direct_root(const char *input,size_t bytes,char *output,size_t capacity)
{
 if(!input||!output||!bytes||bytes>UDX_BOOTARGS_MAX||input[bytes-1]||!capacity)return -1;
 size_t pos=0,used=0;
 while(pos+1<bytes){
  while(pos+1<bytes&&(input[pos]==' '||input[pos]=='\t'))pos++;
  if(pos+1==bytes)break;
  size_t start=pos;unsigned quoted=0;
  while(pos+1<bytes){
   unsigned char c=(unsigned char)input[pos];
   if(c=='"')quoted^=1;
   if(!quoted&&(c==' '||c=='\t'))break;
   if(c<32||c==127)return -2;
   pos++;
  }
  if(quoted)return -3;
  size_t n=pos-start;
  if(!root_argument((const unsigned char *)input+start,n)){
   if(n+(used?1:0)>=capacity-used)return -4;
   if(used)output[used++]=' ';
   for(size_t i=0;i<n;i++)output[used++]=input[start+i];
  }
 }
 static const char suffix[]=UDX_OPENWRT_ROOT_ARGS;
 if(sizeof(suffix)+(used?1:0)>capacity-used)return -4;
 if(used)output[used++]=' ';
 for(size_t i=0;i<sizeof(suffix);i++)output[used+i]=suffix[i];
 return 0;
}
