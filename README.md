# SY108 U-Boot / SPL

面向 Soyealink SY108 5G通信壳（Unisoc UDX710 SoC）的 U-Boot 2015.07 与 SPL（`chipram`）源码仓库，提供基于 Docker 的可重现编译、SML 签名封装与离线仿真校验。


## 源码基线与分支说明

| 项目 | 说明 |
| --- | --- |
| 上游参考仓库 | [sprd-linux/ud710_bsp](https://github.com/sprd-linux/ud710_bsp)（提交 [`4aaa077fed89`](https://github.com/sprd-linux/ud710_bsp/commit/4aaa077fed8900894184c368fc764ab54bb50121)） |
| U-Boot 2015.07 | 对应上游 [`bootloader/u-boot15`](https://github.com/sprd-linux/ud710_bsp/tree/4aaa077fed8900894184c368fc764ab54bb50121/bootloader/u-boot15)，位于本仓库 `src/u-boot` |
| SPL / DDR 初始化 | 对应上游 [`bootloader/chipram`](https://github.com/sprd-linux/ud710_bsp/tree/4aaa077fed8900894184c368fc764ab54bb50121/bootloader/chipram)，位于本仓库 `src/chipram` |

本仓库在参考 BSP 基础上包含 SY108（4H10）板级硬件适配，维护两个分支：

- `oem`：原厂 U-Boot / SPL 源码复现基线。
- `main`：面向 OpenWrt 主系统与独立 Recovery 的原生启动实现。设备保留原厂 SPL、DDR 初始化与 SML，利用原厂 SML 签名数据封装新编译的 U-Boot proper。

## 编译（裸镜像）

宿主机仅需安装 Docker 与 Make。构建固定使用 `linux/amd64` 容器（ARM64 宿主机由 Docker 自动提供模拟执行），无需在宿主机安装交叉编译工具链或 Python 环境。

```sh
make LABEL=dev        # 编译 SPL + U-Boot
make LABEL=dev spl    # 仅编译 SPL
make LABEL=dev uboot  # 仅编译 U-Boot
```

首次构建 Docker 镜像时会联网拉取固定版本的工具链依赖；随后的固件编译容器全程在 `--network none` 离线环境下运行。也可单独构建容器镜像：

```sh
docker build --platform linux/amd64 -f docker/Dockerfile -t udx710-boot:build .
```

构建产物输出至 `build/<LABEL>/`（默认 `build/dev`）：

- `spl/`：`u-boot-spl`（ELF）、`u-boot-spl.bin`、`u-boot-spl-aligned16.bin`、`u-boot-spl.map`
- `uboot/`：`u-boot`（ELF）、`u-boot.bin`、`u-boot.dtb`、`u-boot-dtb.bin`、`u-boot-dtb-aligned16.bin`、`u-boot.map`
- `manifest.json`：构建产物文件名、字节大小与 SHA-256 清单
- `toolchain.txt`、`build-spl.log`、`build-uboot.log`：工具链版本信息与编译日志

## SML 签名封装与离线验证

在保留设备原厂 SPL 的前提下，可通过 `offline` 目标生成可直接刷入 `uboot` 分区的封装镜像并执行离线校验：

```sh
make offline LABEL=offline-sml \
  SPL_IMAGE=/absolute/path/to/original-spl.bin \
  SML_IMAGE=/absolute/path/to/original-sml.bin \
  MAIN_CAPSULE=/absolute/path/to/main/boot.bin \
  RECOVERY_CAPSULE=/absolute/path/to/recovery/boot.bin
```

四个输入参数均需传入绝对路径，构建脚本以只读方式将其挂载进无网络容器。

### 封装结构与原理

原厂 SPL 加载 `uboot` 分区（分区大小 `0x140000`，SPL 实际读取前 `0xC0000` 字节）时，仅对镜像内签名描述符（descriptor）指定偏移区间的载荷计算 SHA-256 并校验 RSA-PSS 证书。`docker/scripts/envelope.py` 按以下结构生成 `build/<LABEL>/uboot.bin`：

1. **DHTB 头部（512 B）**：加载地址固定为 `0x9F000000`，载荷长度与整体 SHA-256 更新为新拼接体。
2. **新 U-Boot + DTB（16 字节对齐）**：位于 `0x9F000000` 起始处，SPL 验签通过后直接跳转至此入口执行。
3. **原厂 SML 签名载荷**：紧随新 U-Boot 之后。
4. **修正偏移后的签名描述符（96 B）与原厂 SML 证书（564 B）**：将描述符中的载荷偏移与证书偏移指向附加的原厂 SML 数据，确保整体必需内容控制在 `0xC0000` 读取上限之内。

### 离线验证流程

`make offline` 在容器内依次执行以下检查并输出 `build/<LABEL>/offline-report.json`：

1. **SPL 安全启动验签校验**：将 `src/chipram` 编译出的原厂 SHA-256 / RSA-PSS 验签目标文件链接为 AArch64 校验程序（`spl-check`），在 `qemu-aarch64` 中对封装后的 `uboot.bin` 执行实际验签逻辑，并通过 Python `cryptography` 独立复核证书链。
2. **Capsule 解压与策略回归测试**：将新编译的 U-Boot `gunzip`、`zlib`、`crc32` 与 `src/u-boot/common/loader/sy108/` 链接为测试程序（`boot-check`），在 `qemu-aarch64` 中校验启动状态机策略，并对传入的主系统与 Recovery `boot.bin` 执行完整解压与 CRC 校验。
3. **Unicorn 指令级启动仿真**：通过 `docker/scripts/emulate.py` 仿真执行原厂 SML 上下文切换、`board_init_f()` 全局数据（GD）与 DTB 拷贝、BSS 清零、早期 ADI/RTC 状态快照、看门狗启动、Capsule 解析及最终跳转 Linux 内核（`0x80200000`）的完整指令路径。

## 工具链与依赖锁定

构建环境的工具链与 Python 依赖通过 [docker/dependencies.lock.json](docker/dependencies.lock.json) 和 [docker/requirements.txt](docker/requirements.txt) 锁定 SHA-256：

| 组件 | 版本与获取方式 |
| --- | --- |
| Linaro GCC | `7.2.1-2017.11`（`x86_64_aarch64-linux-gnu` 预编译工具链，`aarch64-unisoc-linux-` 为其符号链接别名） |
| GNU Binutils | `2.30`（容器内从固定源码归档编译） |
| DTC（Device Tree Compiler） | `1.4.0`（容器内从固定源码归档编译） |
| QEMU User & Python 依赖 | Ubuntu `qemu-user 1:4.2-3ubuntu6.30`，以及固定哈希的 `unicorn`、`capstone`、`cryptography`、`pyelftools` |

## 目录结构与硬件实测状态

- `src/u-boot`：U-Boot 2015.07 源码（SY108 原生启动逻辑位于 `src/u-boot/common/loader/sy108/`）
- `src/chipram`：SPL 与 DDR 初始化源码
- `docker/`：Dockerfile、构建与封装脚本、QEMU/Unicorn 离线校验器及依赖锁文件
- `docs/platform.md`：SY108 平台硬件约束与原生启动流程说明

### 实机验证状态

在保留原厂 SPL、SML 与备用分区 `ubootbak` 的 SY108 实机上，封装后的 U-Boot proper 写入主 `uboot` 分区后已完成以下验证：

| 验证项目 | 状态 |
| --- | --- |
| 断电冷启动与热重启 | 已实测通过 |
| OpenWrt 主系统与独立 Recovery 正常启动 | 已实测通过 |
| 主系统与 Recovery 双向切换 | 已实测通过 |
| 固件升级后试启动（`trial`）与自动确认 | 已实测通过 |
| 启动异常时 PMIC 看门狗超时自动回退 Recovery | 已实测通过 |
| UART 串口按键中断与独立下载模式（U2S / Fastboot） | 仅离线仿真覆盖，待实机验证 |

## 致谢

- [TheGammaSqueeze/UnisocBypass](https://github.com/TheGammaSqueeze/UnisocBypass) — 紫光展锐 DHTB 签名结构分析与保留原厂签名的 U-Boot 引导研究
- [LeoChen-CoreMind/spd_flasher](https://github.com/leochen-coremind/spd_flasher) — 基于 RP2350 的紫光展锐 `CVE-2022-38694` USB Host 自动刷机与恢复工具
- [TomKing062/CVE-2022-38694_unlock_bootloader](https://github.com/TomKing062/CVE-2022-38694_unlock_bootloader) & [ilyakurdyukov/spreadtrum_flash](https://github.com/ilyakurdyukov/spreadtrum_flash) (`spd_dump`) — 紫光展锐 BSL/FDL 刷写协议与 BootROM 漏洞利用工具
- [NCC Group](https://www.nccgroup.com/research-blog/there-s-another-hole-in-your-soc-unisoc-rom-vulnerabilities/) — 紫光展锐 BootROM 漏洞（`CVE-2022-38694`）原始安全研究
- [sprd-linux/ud710_bsp](https://github.com/sprd-linux/ud710_bsp) — UDX710 `u-boot15` 与 `chipram` 参考源码基线

---

*Powered by GPT 6.1 Sol & Gemini 4 Argon*

