#!/bin/sh
# Compile and package the maintained sources entirely inside Docker.
set -eu
trap 'status=$?; if test "$status" -ne 0; then for log in /out/build-spl.log /out/build-uboot.log; do if test -f "$log"; then tail -n 35 "$log" >&2; fi; done; fi' 0
part=${1:-all}
case "$part" in all|spl|uboot);;*) echo 'Usage: IMAGE [all|spl|uboot]' >&2; exit 64;; esac
jobs=${JOBS:-$(getconf _NPROCESSORS_ONLN)}
case "$jobs" in *[!0-9]*|''|0)exit 64;;esac
if test "$jobs" -gt 8; then jobs=8; fi
flags='-mno-fix-cortex-a53-835769 -B/opt/binutils230/aarch64-linux-gnu/bin/'
splsrc=/src/chipram
splout=/build/chipram
ubsrc=/src/u-boot
ubout=/build/u-boot
dtc=/opt/dtc140/bin/dtc
mkdir -p /out
{
 uname -m
 aarch64-linux-gnu-gcc --version | head -n 1
 aarch64-unisoc-linux-gcc --version | head -n 1
 aarch64-linux-gnu-gcc -dumpmachine
 aarch64-unisoc-linux-ld --version | head -n 1
 "$dtc" -v
} > /out/toolchain.txt
case "$part" in all|spl)
 make -C "$splsrc" O="$splout" CROSS_COMPILE=aarch64-linux-gnu- udx710_3h10_config > /out/build-spl.log 2>&1
 make -C "$splsrc" O="$splout" CROSS_COMPILE=aarch64-linux-gnu- CONFIG_SPRD_SECBOOT=y CONFIG_SECURE_EFUSE=y PKCS1_PSS_FLAG=y OPTFLAGS="-Os -std=gnu89 $flags" spl -j"$jobs" >> /out/build-spl.log 2>&1
 python3 /opt/boot/package.py spl "$splout" /out
 ;;esac
case "$part" in all|uboot)
 make -C "$ubsrc" O="$ubout" ARCH=arm DTC="$dtc" CROSS_COMPILE=aarch64-unisoc-linux- udx710_4h10_recon_defconfig > /out/build-uboot.log 2>&1
 make -C "$ubsrc" O="$ubout" ARCH=arm DTC="$dtc" CROSS_COMPILE=aarch64-unisoc-linux- DEVICE_TREE=udx710_4h10_recon KCFLAGS="$flags" -j"$jobs" >> /out/build-uboot.log 2>&1
 python3 /opt/boot/package.py uboot "$ubout" /out
 ;;esac
python3 /opt/boot/package.py manifest /out
printf 'Build complete: /out\n'
