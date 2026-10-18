#!/bin/sh
set -eu
ulimit -c 0
rm -f /out/offline-report.json /out/uboot.bin
export SOURCE_DATE_EPOCH=${SOURCE_DATE_EPOCH:-1661071572}
/bin/sh /opt/boot/build-firmware.sh spl
crypto=/build/chipram/nand_spl/secure/sprd
aarch64-linux-gnu-gcc -static -no-pie -O2 /opt/boot/spl-check.c \
 /build/chipram/nand_spl/secure/common/sec_common.o \
 "$crypto/sprd_verify.o" "$crypto/sec_string.o" \
 "$crypto/sprd_sha256_armv8.o" "$crypto/sprd_sha256_core_armv8.o" \
 "$crypto/sprd_rsa_sw.o" "$crypto/sprd_crypto_sw.o" "$crypto/pk1.o" \
 "$crypto/sprd_pkcs1_mgf1.o" "$crypto/sprd_pkcs1_pss.o" "$crypto/sprd_pkcs1_v1_5.o" \
 -Wl,--gc-sections -o /out/spl-check
/bin/sh /opt/boot/build-firmware.sh uboot
native=/src/u-boot/common/loader/sy108
aarch64-linux-gnu-gcc -static -O2 -std=c11 -I"$native" /opt/boot/boot-check.c \
 "$native/capsule.c" "$native/boot-policy.c" /build/u-boot/lib/gunzip.o \
 /build/u-boot/lib/zlib/zlib.o /build/u-boot/lib/crc32.o -Wl,--gc-sections -o /out/boot-check
python3 /opt/boot/envelope.py /out/uboot/u-boot-dtb.bin /inputs/sml.bin /out/uboot.bin > /tmp/envelope.json
digest=$(sha256sum /out/uboot.bin | cut -d' ' -f1)
python3 /opt/boot/offline.py /out "$digest"
python3 /opt/boot/package.py manifest /out
