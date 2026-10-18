#!/bin/sh
# Reuse the verified GCC binary distribution. Build only the two exact-version
# utilities for which a suitable standalone prebuilt package has not been found.
set -eu
src=/tmp/dependencies
sdk=/opt/linaro7/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu
mkdir -p /tmp/build-binutils
cd /tmp/build-binutils
"$src/binutils/configure" --target=aarch64-linux-gnu --prefix=/opt/binutils230 \
 --disable-gdb --disable-gdbserver --disable-sim --disable-werror \
 --disable-nls --disable-gprof --disable-gold --enable-plugins
make -j6 MAKEINFO=true all-binutils all-gas all-ld
make MAKEINFO=true install-binutils install-gas install-ld
cd "$src/dtc"
make -j6 CC=gcc-9 dtc
install -D -m 755 dtc /opt/dtc140/bin/dtc
cd "$sdk/bin"
for t in gcc g++ cpp ar as ld nm objcopy objdump ranlib readelf size strip; do
 ln -s "aarch64-linux-gnu-$t" "aarch64-unisoc-linux-$t"
done
ln -sf /opt/binutils230/bin/aarch64-linux-gnu-ld aarch64-unisoc-linux-ld
