# SY108 U-Boot / SPL

面向 Soyealink SY108 5G通信壳（Unisoc UDX710 SoC）的 U-Boot 2015.07 与 SPL（`chipram`）源码仓库（`oem` 基线分支）。

## 源码基线

| 项目 | 地址 |
| --- | --- |
| 上游参考仓库 | [sprd-linux/ud710_bsp](https://github.com/sprd-linux/ud710_bsp)（提交 [`4aaa077fed89`](https://github.com/sprd-linux/ud710_bsp/commit/4aaa077fed8900894184c368fc764ab54bb50121)） |
| U-Boot 2015.07 | [`bootloader/u-boot15`](https://github.com/sprd-linux/ud710_bsp/tree/4aaa077fed8900894184c368fc764ab54bb50121/bootloader/u-boot15)（本仓库 `src/u-boot`） |
| SPL / DDR | [`bootloader/chipram`](https://github.com/sprd-linux/ud710_bsp/tree/4aaa077fed8900894184c368fc764ab54bb50121/bootloader/chipram)（本仓库 `src/chipram`） |

本分支维护 SY108（4H10）原厂启动、下载、安全校验及硬件初始化的源码基线。

## 构建

宿主机仅需安装 Docker 与 Make，固定使用 `linux/amd64` 容器编译：

```sh
make LABEL=dev        # 编译 SPL + U-Boot
make LABEL=dev spl    # 仅编译 SPL
make LABEL=dev uboot  # 仅编译 U-Boot
```

首次构建容器镜像时联网拉取依赖，固件编译容器全程在 `--network none` 下运行。也可单独构建镜像：

```sh
docker build --platform linux/amd64 -f docker/Dockerfile -t udx710-boot:build .
```

构建产物位于 `build/<LABEL>/`：

- `spl/`：SPL ELF、原始 bin、16 字节对齐 bin 与 map
- `uboot/`：U-Boot ELF、原始 bin、DTB、拼接 DTB 的 bin、16 字节对齐 bin 与 map
- `manifest.json`：构建产物文件名、大小与 SHA-256 清单
- `toolchain.txt`、`build-spl.log`、`build-uboot.log`：工具链版本与构建日志

## 工具链依赖

依赖下载地址、版本与 SHA-256 锁定在 [docker/dependencies.lock.json](docker/dependencies.lock.json)：

| 组件 | 版本与获取方式 |
| --- | --- |
| Linaro GCC | `7.2.1-2017.11`（预编译发行包，`aarch64-unisoc-linux-` 为 `aarch64-linux-gnu-` 的符号链接别名） |
| GNU Binutils | `2.30`（容器内源码编译） |
| DTC | `1.4.0`（容器内源码编译） |
