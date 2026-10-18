/* SPDX-License-Identifier: GPL-2.0+
 * SY108 OEM boot reconstruction from exact original instructions.
 * Certificate/RSA/SCI algorithms below are retained from the public BSP.
 * This file and the candidate image have not been booted on hardware.
 */
#include <common.h>
#include <crypto/sprdrsa.h>
#include <secureboot/sprdsec_header.h>
/* A distinct private copy is present at the original secure-module tail. */
#define sec_memset oem_sec_memset
#define sec_memcpy oem_sec_memcpy
#define sec_memcmp oem_sec_memcmp
#include <secureboot/sec_string.h>
#include <secureboot/sec_common.h>
#include <crypto/sprdsha.h>
#include <sprd_rsa.h>
#include <sprd_crypto_sw.h>
#include <sprd_common_rw.h>
#include <otp_helper.h>
#include <libfdt.h>
extern int fdt_chosen_bootargs_append(void *,const char *,int);
extern int get_buffer_base_size_from_dt(const char *,unsigned long *,unsigned long *);
extern int sprd_efuse_double_prog(u32,bool,bool,u32);
#define secf(fmt,args...) do { printf("%s(): ",__func__); printf(fmt,##args); } while (0)
#define MODEM_MAGIC "SCI1"
#define MODEM_HDR_SIZE 12
#define SCI_TYPE_MODEM_BIN 1
#define MODEM_SHA1_HDR 0x400
#define MODEM_SHA1_SIZE 20
typedef struct {u32 type_flags,offset,length;} data_block_header_t;
char uboot_hash[137];
unsigned char verify_type;
static unsigned int trusted_version=0x1234,nontrusted_version=0xffff;
static unsigned int nontrusted_count=0;
static unsigned int nontrusted_updates[10]={0};
static unsigned int trusted_count=0;
unsigned int sy108_counter_read(unsigned int,unsigned int *);
unsigned int sprd_get_version(unsigned int,unsigned int *);
void cal_sha256(uint8_t *,uint32_t,uint8_t *);
bool sprd_anti_rollback(uint32_t,uint32_t);
uint8_t *sprd_get_sechdr_addr(uint8_t *);
uint8_t *sprd_get_cert_addr(uint8_t *);
void sprd_get_hash_key(uint8_t *,uint8_t *);
void sprd_get_pubk(uint8_t *,uint8_t *);
bool sprd_verify_img(uint8_t *,uint8_t *);
unsigned short crc16_tab[256] = {
 0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
 0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
 0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
 0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
 0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
 0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
 0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
 0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
 0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
 0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
 0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
 0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
 0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
 0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
 0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
 0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
 0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
 0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
 0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
 0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
 0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
 0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
 0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
 0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
 0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
 0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
 0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
 0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
 0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
 0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
 0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
 0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0,
};
unsigned short oem_crc16(unsigned char *data, unsigned char len)
{
 unsigned int crc=0;
 while(len--) {
  unsigned int shifted=(crc<<8)&0xff00;
  crc=crc16_tab[(int)((crc>>8)^*data++)]^shifted;
 }
 return crc;
}

noinline unsigned char int_to_char(unsigned char value)
{
 unsigned char result;
 if(value >= 10) result=value+'A'-10;
 else result=value+'0';
 return result;
}

unsigned char intBuf_to_charBuf(unsigned char *input, char *output, unsigned int len)
{
 unsigned int i;
 for(i=0;i<len;i++) {
  output[2*i]=int_to_char(input[i] >> 4);
  output[2*i+1]=int_to_char(input[i] & 15);
 }
 return 0;
}

void cal_sha256(uint8_t * input, uint32_t bytes_num, uint8_t * output)
{

	if ((NULL != input) && (NULL != output)) {
		sha256_csum_wd(input, bytes_num, output, NULL);
	} else {
		secf("\r\tthe pointer is error,pls check it\n");
	}
}

unsigned int sprd_set_version(unsigned int,unsigned int);
void update_sec_version(void)
{
    int         i;
    uint32_t    nosecure_update_version = 0;

    if (nontrusted_count) {
        nosecure_update_version = nontrusted_updates[0];
        for (i = 1;i < nontrusted_count; i++)
        {
            if (nosecure_update_version != nontrusted_updates[i])
            {
                secf("\r\Nosec rollback version not unified\n");
                return;
            }
        }
        if (nosecure_update_version > nontrusted_version) {
            sprd_set_version(1,nosecure_update_version);
        }
        nontrusted_count = 0;
        nontrusted_version = 0;
    }
}

bool sprd_anti_rollback(uint32_t type, uint32_t version)
{
 unsigned char mode;
 secf("orca image type = %d\n", type);
 if (type == 1) {
  if (trusted_version == 0x1234) {
   sprd_get_version(0, &trusted_version);
   secf("read efuse version: 0X%x\n", trusted_version);
  }
  secf("Trusted_rollback_ver = 0X%x  cert_ver = 0X%x\n", trusted_version, version);
  if (version < trusted_version) {
   secf("ORCA SEC ANTI_ROLLBACK ERR ver = 0X%x\n", version);
   return false;
  }
  trusted_count++;
  return true;
 } else if (type == 0) {
  mode = verify_type;
  if (mode == 11) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(16, &nontrusted_version);
   secf("read boot version: %d\n", nontrusted_version);
  }
  else if (mode == 12) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(17, &nontrusted_version);
   secf("read nr_agdsp version: %d\n", nontrusted_version);
  }
  else if (mode == 13) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(18, &nontrusted_version);
   secf("read recovery version: %d\n", nontrusted_version);
  }
  else if (mode == 14) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(19, &nontrusted_version);
   secf("read nr_modem version: %d\n", nontrusted_version);
  }
  else if (mode == 15) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(20, &nontrusted_version);
   secf("read nr_v3phy version: %d\n", nontrusted_version);
  }
  else if (mode == 16) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(21, &nontrusted_version);
   secf("read nr_nrphy version: %d\n", nontrusted_version);
  }
  else if (mode == 17) {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   sy108_counter_read(22, &nontrusted_version);
   secf("read nr_nrdsp version: %d\n", nontrusted_version);
  }
  else {
   asm volatile("" ::: "memory");
   verify_type = 0;
   asm volatile("" ::: "memory");
   if (mode == 21) {
    nontrusted_version = sprd_ap_efuse_read(24);
    secf("Non_Trusted_rollback_ver = %d  cert_ver = %d\n", nontrusted_version, version);
    if (version < nontrusted_version) {
     secf("ORCA NOSEC ANTI_ROLLBACK ERR ver = %d\n", version);
     return false;
    } else if (version > nontrusted_version) {
     sprd_efuse_double_prog(24, 1, 0, version);
    }
    return true;
   } else if (mode == 22) {
    nontrusted_version = sprd_ap_efuse_read(25);
    if (version < nontrusted_version) {
     secf("ORCA NOSEC ANTI_ROLLBACK ERR ver = %d\n", version);
     return false;
    } else if (version > nontrusted_version) {
     sprd_efuse_double_prog(25, 1, 0, version);
    }
    return true;
   } else {
    secf("wrong image!\n");
    return false;
   }
  }
  secf("Non_Trusted_rollback_ver = %d  cert_ver = %d\n", nontrusted_version, version);
  if (version < nontrusted_version) {
   secf("ORCA NOSEC ANTI_ROLLBACK ERR ver = %d\n", version);
   return false;
  }
  nontrusted_updates[nontrusted_count] = version;
  nontrusted_count++;
  return true;
 } else {
  secf("orca image type mismatch!! \n");
  return false;
 }
}

bool sprd_verify_cert(uint8_t * hash_key_precert, uint8_t * hash_data, uint8_t * certptr)
{
	bool ret = false;
	uint8_t certtype = *certptr;
	uint8_t temphash_data[HASH_BYTE_LEN];
	sprd_crypto_err_t err = SPRD_CRYPTO_SUCCESS;
	sprd_rsa_pubkey_t pubkey;
	sprd_rsa_padding_t padding;
	int32_t result;
	uint8_t certhash[HASH_BYTE_LEN] = {0};

	padding.type = SPRD_RSASSA_PKCS1_PSS_MGF1;
	padding.pad.rsassa_pss.type = SPRD_CRYPTO_HASH_SHA256;
	padding.pad.rsassa_pss.mgf1_hash_type = SPRD_CRYPTO_HASH_SHA256;
	padding.pad.rsassa_pss.salt_len = 32;

	secf("<pss> cert type: %d\n",certtype);
	if ((certtype == CERTTYPE_CONTENT) || (certtype == CERTTYPE_KEY)) {
	   if (certtype == CERTTYPE_KEY) {

	            sprd_keycert *curcertptr = (sprd_keycert *) certptr;

	            cal_sha256((uint8_t *) & (curcertptr->pubkey), SPRD_RSAPUBKLEN, temphash_data);

	            if (sec_memcmp(hash_data, curcertptr->hash_data, HASH_BYTE_LEN)
	                || sec_memcmp(hash_key_precert, temphash_data, HASH_BYTE_LEN)
	                ) {
	                     secf("<pss> cmp hash of pubk diffent\r\n");
	                     return false;
	            }

	            pubkey.n = curcertptr->pubkey.mod;
	            pubkey.n_len = (curcertptr->pubkey.keybit_len) >> 3;
	            pubkey.e = (uint8_t *) & (curcertptr->pubkey.e);
	            pubkey.e_len = 4;

		    cal_sha256(curcertptr->hash_data, KEYCERT_HASH_LEN, certhash);
	            err = sprd_rsa_verify((const sprd_rsa_pubkey_t*)(&pubkey),
	                               certhash, HASH_BYTE_LEN,
	                               (const uint8_t*)(curcertptr->signature), pubkey.n_len,
	                               padding, &result);

	            if (err != SPRD_CRYPTO_SUCCESS) {
	                     secf("\n<pss> RSA verify err fail(%08x)\n", err);
	                     return false;
	            }

	            if (result != SPRD_VERIFY_SUCCESS) {
	                     secf("\n<pss> RSA verify err result(%08x)\n", result);
	                     return false;
	            }

	            secf("\n<pss> RSA verify Success\n");
	            ret = sprd_anti_rollback(curcertptr->type,curcertptr->version);
	   }

	   else if(certtype ==  CERTTYPE_CONTENT) {             //certtype is content

	            sprd_contentcert *curcertptr = (sprd_contentcert *) certptr;

	            cal_sha256((uint8_t *) & (curcertptr->pubkey), SPRD_RSAPUBKLEN, temphash_data);

	            if (sec_memcmp(hash_data, curcertptr->hash_data, HASH_BYTE_LEN)
	                || sec_memcmp(hash_key_precert, temphash_data, HASH_BYTE_LEN)
	                ) {
	                     secf("<pss> cmp hash key diffent\r\n");
	                     return false;
	            }
				//dumpHex("signature",curcertptr->signature,256);
				//dumpHex("pubkey",& (curcertptr->pubkey),260);
	            pubkey.n = curcertptr->pubkey.mod;
	            pubkey.n_len = (curcertptr->pubkey.keybit_len) >> 3;
	            pubkey.e = (uint8_t *) & (curcertptr->pubkey.e);
	            pubkey.e_len = 4;

		    cal_sha256(curcertptr->hash_data, CNTCERT_HASH_LEN, certhash);
	            err = sprd_rsa_verify((const sprd_rsa_pubkey_t*)(&pubkey),
	                               certhash, HASH_BYTE_LEN,
	                               (const uint8_t*)(curcertptr->signature), pubkey.n_len,
	                               padding, &result);

	            if (err != SPRD_CRYPTO_SUCCESS) {
	                     secf("\n<pss> RSA verify err fail(%08x)\n", err);
	                     return false;
	            }

	            if (result != SPRD_VERIFY_SUCCESS) {
	                     secf("\n<pss> RSA verify err result(%08x)\n", result);
	                     return false;
	            }

	            secf("\n<pss> RSA verify Success\n");
	            ret = sprd_anti_rollback(curcertptr->type,curcertptr->version);
	   }

	} else {
	   secf("invalid cert type %d!!", certtype);
	   ret = false;
	}

	return ret;
}

static int is_packed_modem_image(uint8_t *data)
{
	if (sec_memcmp(data, (uint8_t*)MODEM_MAGIC, sizeof(MODEM_MAGIC)))
		return 0;
	return 1;
}

static void get_modem_info(uint8_t *data, uint32_t *code_offset, uint32_t *code_len)
{
	uint32_t offset = 0, hdr_offset = 0, length = 0;
	uint8_t hdr_buf[MODEM_HDR_SIZE << 3] = {0};
	uint8_t read_len;
	uint8_t result = 0; // 0:OK, 1:not find, 2:some error occur
	data_block_header_t *hdr_ptr = NULL;

	read_len = sizeof(hdr_buf);
	sec_memcpy(hdr_buf, data, read_len);

    do {
      if (!hdr_offset) {
        if (memcmp(hdr_buf, MODEM_MAGIC, sizeof(MODEM_MAGIC))) {
          result = 2;
          secf("old image format!\n");
          break;
        }

        hdr_ptr = (data_block_header_t *)hdr_buf + 1;
        hdr_offset = MODEM_HDR_SIZE;
      } else {
        hdr_ptr = (data_block_header_t *)hdr_buf;
      }

      data_block_header_t* endp
          = (data_block_header_t*)(hdr_buf + sizeof hdr_buf);
      int found = 0;
      while (hdr_ptr < endp) {
        uint32_t type = (hdr_ptr->type_flags & 0xff);
        if (SCI_TYPE_MODEM_BIN == type) {
          found = 1;
          break;
        }

        /*  There is a bug (622472) in MODEM image generator.
         *  To recognize wrong SCI headers and correct SCI headers,
         *  we devise the workaround.
         *  When the MODEM image generator is fixed, remove #if 0.
         */
#if 0
        if (hdr_ptr->type_flags & MODEM_LAST_HDR) {
          result = 2;
          MODEM_LOGE("no modem image, error image header!!!\n");
          break;
        }
#endif
        hdr_ptr++;
      }
      if (!found) {
        result = 2;
        secf("no MODEM exe found in SCI header!");
      }

      if (result != 1) {
        break;
      }
    } while (1);

    if (!result) {
      offset = hdr_ptr->offset;
      if (hdr_ptr->type_flags & MODEM_SHA1_HDR) {
        offset += MODEM_SHA1_SIZE;
      }
      length = hdr_ptr->length;
    }

	*code_offset = offset;
	*code_len = length;
}

uint8_t *sprd_get_sechdr_addr(uint8_t * buf)
{
	if (NULL == buf) {
		secf("\r\t input of get_sechdr_Addr err\n");
	}
	sys_img_header *imghdr = (sys_img_header *) buf;

	uint8_t *sechdr = buf + imghdr->mImgSize + sizeof(sys_img_header);
	return sechdr;
}

uint8_t *sprd_get_cert_addr(uint8_t * buf)
{
	sprdsignedimageheader *sechdr_addr = (sprdsignedimageheader *) sprd_get_sechdr_addr(buf);
	uint8_t *cert_addr = buf + sechdr_addr->cert_offset;

	return cert_addr;
}

bool sprd_verify_img(uint8_t * hash_key_precert, uint8_t * imgbuf)
{
	sprdsignedimageheader *imghdr = (sprdsignedimageheader *) sprd_get_sechdr_addr(imgbuf);
	uint8_t *code_addr = imgbuf + imghdr->payload_offset;
	if (is_packed_modem_image(code_addr)) {
		secf("the image is new packed modem image\n");
		uint32_t modem_offset = 0;
		uint32_t modem_len = 0;
		get_modem_info(code_addr, &modem_offset, &modem_len);
		secf("modem offset: %d, length: %d\n", modem_offset, modem_len);
		code_addr += modem_offset;
	}
	uint8_t soft_hash_data[HASH_BYTE_LEN] = {0};
	cal_sha256(code_addr, imghdr->payload_size, soft_hash_data);
	uint8_t *curcertptr = imgbuf + imghdr->cert_offset;

	bool result = sprd_verify_cert(hash_key_precert, (uint8_t *) soft_hash_data, curcertptr);

	return result;
}

 void sprd_get_hash_key(uint8_t * load_buf,uint8_t *hash_key)
{
	sprd_keycert *certtype = (sprd_keycert *)sprd_get_cert_addr((uint8_t *)load_buf);
	sec_memcpy(hash_key, certtype->hash_key, HASH_BYTE_LEN);
}

void sprd_get_pubk(uint8_t *load_buf,uint8_t *pubk)
{
	sprd_keycert *certtype = (sprd_keycert*)sprd_get_cert_addr((uint8_t*)load_buf);
	sec_memcpy(pubk, (uint8_t*)&certtype->pubkey, sizeof(sprd_rsapubkey));
}

void sprd_secure_check(uint8_t *current, uint8_t *image)
{
 uint8_t next_hash[32];
 sec_memset(next_hash,0,32);
 sprd_get_hash_key(current,next_hash);
 uint8_t spl_public[264]={0};
 uint8_t spl_hash_crc[34]={0};
 char next_hash_text[64]={0};
 char spl_hash_text[68]={0};
 sprd_get_pubk((uint8_t *)0x28007000,spl_public);
 cal_sha256(spl_public,sizeof(spl_public),spl_hash_crc);
 unsigned short crc=oem_crc16(spl_hash_crc,32);
 spl_hash_crc[32]=crc&0xff;
 spl_hash_crc[33]=(crc>>8)&0xff;
 if(intBuf_to_charBuf(next_hash,next_hash_text,32))
  secf("error:intBuf_to_charBuf() fail1\n");
 if(intBuf_to_charBuf(spl_hash_crc,spl_hash_text,34))
  secf("error:intBuf_to_charBuf() fail2\n");
 uboot_hash[0]='h';uboot_hash[1]='a';uboot_hash[2]='s';
 uboot_hash[3]='h';uboot_hash[4]='=';
 sec_memcpy(uboot_hash+5,next_hash_text,64);
 sec_memcpy(uboot_hash+69,spl_hash_text,68);
 if(!sprd_verify_img(next_hash,image)) {
  secf("\r\t sprd_secure_check err\n");
  while(1);
 }
}

void sprd_dl_check(uint8_t *hash, uint8_t *image)
{
 uint8_t temporary_hash[32];
 sec_memset(temporary_hash,0,32);
 if(!sprd_verify_img(hash,image)) {
  secf("\r\t sprd_dl_check err\n");
  while(1);
 }
}

void orca_cpe_boot_verify(const char *name, void *image)
{
 secf("orca cpe img is: %s enter\r\n",name);
 if(name) {
  if(!strcmp("boot",name)||!strcmp("recovery",name)) {
   verify_type=21;
   sprd_secure_check((uint8_t *)(CONFIG_SYS_TEXT_BASE-512),image);
  } else if(!strcmp("nr_agdsp",name)) {
   verify_type=22;
   sprd_secure_check((uint8_t *)(CONFIG_SYS_TEXT_BASE-512),image);
  } else {
   secf("the cpe img do not need to verify\n");
  }
 }
}

void orca_cpe_dl_verify(const char *part, const char *key_owner, uint8_t *image, unsigned int unused)
{
 uint8_t public[264]={0},hash[32]={0};
 secf("img: %s store pubk hash\r\n",key_owner);
 if(!strcmp("boot",part))verify_type=11;
 else if(!strcmp("nr_agdsp",part))verify_type=12;
 else if(!strcmp("recovery",part))verify_type=13;
 else if(!strcmp("nr_modem",part))verify_type=14;
 else if(!strcmp("nr_v3phy",part))verify_type=15;
 else if(!strcmp("nr_nrphy",part))verify_type=16;
 else if(!strcmp("nr_nrdsp1",part)||!strcmp("nr_nrdsp2",part))verify_type=17;
 else verify_type=0;
 if(!key_owner)return;
 if(!strcmp("splloader",key_owner)||!strcmp("fdl1",key_owner))
  sprd_secure_check((uint8_t *)0x28007000,image);
 else if(!strcmp("fdl2",key_owner))
  sprd_secure_check((uint8_t *)(CONFIG_SYS_TEXT_BASE-512),image);
 else if(!strcmp("splloader0",key_owner)) {
  sprd_get_pubk((uint8_t *)0x28007000,public);
  cal_sha256(public,264,hash);
  sprd_dl_check(hash,image);
 } else if(!strcmp("uboot",key_owner))
  sprd_secure_check((uint8_t *)(CONFIG_SYS_TEXT_BASE-512),image);
}
