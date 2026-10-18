# syntax=docker/dockerfile:1
# Always use amd64, including on ARM64 hosts. No host SDK or binary archives.
FROM --platform=linux/amd64 ubuntu:20.04@sha256:8feb4d8ca5354def3d8fce243717141ce31e2c428701f6682bd2fafe15388214 AS base
ENV DEBIAN_FRONTEND=noninteractive LC_ALL=C.UTF-8 TZ=UTC
RUN test "$(dpkg --print-architecture)" = amd64 \
 && apt-get update && apt-get install -y --no-install-recommends \
      build-essential gcc-9 g++-9 make bc bison flex perl patch git file curl \
      ca-certificates xz-utils libssl-dev zlib1g-dev libncurses5-dev texinfo \
      python3 \
 && rm -rf /var/lib/apt/lists/*

FROM base AS toolchain
COPY docker/dependencies.lock.json /opt/boot/dependencies.lock.json
COPY docker/scripts/fetch-dependencies.py docker/scripts/build-toolchain.sh /opt/boot/
RUN python3 /opt/boot/fetch-dependencies.py /opt/boot/dependencies.lock.json
RUN /bin/sh /opt/boot/build-toolchain.sh
FROM base AS builder
COPY --from=toolchain /opt/ /opt/
ENV PATH=/opt/binutils230/bin:/opt/linaro7/gcc-linaro-7.2.1-2017.11-x86_64_aarch64-linux-gnu/bin:/opt/dtc140/bin:${PATH}
ENV SOURCE_DATE_EPOCH=1661071572 GIT_CONFIG_GLOBAL=/dev/null GIT_CONFIG_SYSTEM=/dev/null HOME=/tmp
COPY src/u-boot/ /src/u-boot/
COPY src/chipram/ /src/chipram/
COPY docker/scripts/ /opt/boot/
RUN mkdir -p /build/u-boot /build/chipram /out \
 && chmod 1777 /build/u-boot /build/chipram /out \
 && chmod 755 /opt/boot/build-firmware.sh
WORKDIR /out
ENTRYPOINT ["/opt/boot/build-firmware.sh"]
CMD ["all"]
