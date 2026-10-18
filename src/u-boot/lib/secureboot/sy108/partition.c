#include <common.h>
#include <sprd_common_rw.h>
#include <secureboot/sec_common.h>
#define secf(fmt,args...) do { printf("%s(): ",__func__); printf(fmt,##args); } while(0)
extern void orca_cpe_dl_verify(const char *,const char *,unsigned char *,unsigned int);
extern void orca_cpe_boot_verify(const char *,void *);
extern int get_buffer_base_size_from_dt(const char *,unsigned long *,unsigned long *);
extern unsigned long boot_img_buffer;
static unsigned char recovery_verified;

int orca_cpe_classify_download(const char *part,void *image)
{
 if(!strcmp("splloader",part)) {
  orca_cpe_dl_verify(part,"splloader0",image,0);return 2;
 }
 if(!strcmp("sml",part)||!strcmp("uboot",part)) {
  orca_cpe_dl_verify(part,"fdl1",image,0);return 2;
 }
 if(!strcmp("boot",part)||!strcmp("nr_agdsp",part)||!strcmp("nr_modem",part)||
    !strcmp("nr_v3phy",part)||!strcmp("nr_nrphy",part)||!strcmp("nr_nrdsp1",part)||
    !strcmp("recovery",part)||!strcmp("nr_nrdsp2",part)) {
  orca_cpe_dl_verify(part,"fdl2",image,0);return 2;
 }
 return 1;
}

extern __typeof__(orca_cpe_classify_download) sy108_classify_download
 __attribute__((alias("orca_cpe_classify_download")));

int orca_cpe_load_partition(const char *name,unsigned long offset,void *image)
{
 sys_img_header header;
 int size=0;
 if(!strcmp(name,"boot")||!strcmp(name,"recovery")) {
  secf("%s needs to verify in uboot\n",name);
  if(common_raw_read((char *)name,512,0,(char *)&header)) goto read_error;
  size=header.mImgSize;
  if(common_raw_read((char *)name,size+4096,0,image)) goto read_error;
 } else {
  secf("%s do not need to verify\n",name);size=0;
 }
 goto out;
read_error:
 errorf("read %s partition fail\n",name);size=-1;
out:
 return size;
}
void sy108_verify_partition(const char *name,void *image)
{
 unsigned long base,size;
 if(!name)return;
 if(!strcmp(name,"boot")) {
  get_buffer_base_size_from_dt("heap@4",&base,&size);
  if(orca_cpe_load_partition(name,0,(void *)base)>=0) {
   orca_cpe_boot_verify(name,(void *)base);
   boot_img_buffer=base;
  }
 } else if(!strcmp(name,"recovery")) {
  recovery_verified=1;
  get_buffer_base_size_from_dt("heap@4",&base,&size);
  if(orca_cpe_load_partition(name,0,(void *)base)>=0) {
   orca_cpe_boot_verify(name,(void *)base);
   boot_img_buffer=base;
  }
 } else if(!strcmp(name,"nr_agdsp")&&!recovery_verified)
  orca_cpe_boot_verify(name,image);
}
