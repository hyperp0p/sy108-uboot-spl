/* SPDX-License-Identifier: MIT
 * AArch64 test harness for chipram secure-boot verification objects.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
extern void update_swVersion(void);
static void bounds_fault(int sig) { (void)sig; _exit(96); }

extern int secboot_verify(void *,void *,void *,uint32_t,int);
static unsigned version;
unsigned int sprd_get_version(unsigned counter, unsigned *value)
{ (void)counter; *value=version; return 0; }
unsigned int sprd_set_version(unsigned counter, unsigned value)
{ (void)counter; (void)value; fputs("unexpected version write\n",stderr); exit(90); }

static void *load(const char *name, size_t limit)
{
 size_t page=sysconf(_SC_PAGESIZE), span=(limit+page-1)&~(page-1);
 unsigned char *map=mmap(NULL,span+2*page,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 unsigned char *out;
 FILE *file;
 if(map==MAP_FAILED || mprotect(map+page,span,PROT_READ|PROT_WRITE))exit(91);
 out=map+page+span-limit; /* End is exactly adjacent to the inaccessible guard. */
 memset(out,0xff,limit);
 file=fopen(name,"rb");if(!file)exit(92);
 size_t n=fread(out,1,limit,file);fclose(file);
 (void)n; /* Original SPL RSA temporarily reverses bytes in writable RAM. */
 return out;
}
int main(int argc,char **argv)
{
 if(argc!=5 && argc!=6)return 94;
 signal(SIGSEGV,bounds_fault); signal(SIGBUS,bounds_fault);
 size_t limit=strtoul(argv[3],NULL,0);
 version=strtoul(argv[4],NULL,0);
 if(limit<512 || limit>0xc0000)return 95;
 void *root=load(argv[1],0x40000), *image=load(argv[2],limit);
 int result=0;
 if(argc==6) {
  void *sml=load(argv[5],0x10000);
  result=secboot_verify(root,sml,NULL,0,0);
  if(!result)result=secboot_verify(root,image,NULL,0,0);
  if(!result)result=secboot_verify(root,sml,NULL,0,0);
  if(!result)result=secboot_verify(root,image,NULL,0,0);
  if(!result)update_swVersion();
 } else result=secboot_verify(root,image,NULL,0,0);
 printf("SPL_RESULT=%d efuse_model_version=%u source_crypto=original\n",result,version);
 return result?1:0;
}
