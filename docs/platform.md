# SY108 平台与启动流程说明

本文档说明 Soyealink SY108 5G通信壳（Unisoc UDX710 SoC）在 `sy108-uboot-spl` 中的构建配置、板级硬件约束以及 `main` 分支的原生启动流程。


## 构建配置与职责划分

- **SPL（`src/chipram`）**：使用 `udx710_3h10_config`，启用 `CONFIG_SPRD_SECBOOT`、`CONFIG_SECURE_EFUSE` 与 `PKCS1_PSS_FLAG`。负责早期 SoC 初始化、DDR 训练，以及加载并校验后级 SML 与 U-Boot 镜像。
- **U-Boot（`src/u-boot`）**：使用 `udx710_4h10_recon_defconfig`，设备树为 `udx710_4h10_recon`，启用 `CONFIG_NO_RELOCATION`（生产路径重定位偏移为 0）与 `CONFIG_SY108_NATIVE_BOOT`。负责板级外设与 NAND/UBI 初始化、启动模式判定、Capsule 镜像校验解压及移交 Linux 内核。

U-Boot 主流程依次经过 `board_init_f()`（分配复制 `global_data` 与附加 DTB，随后清零 BSS）、`board_init_r()`、板级 `board_init()`，最终进入 `cboot()` 执行启动选择或维护模式。

## 原生启动流程（`CONFIG_SY108_NATIVE_BOOT`）

原生启动代码位于 `src/u-boot/common/loader/sy108/`，直接解析 OpenWrt 主系统与独立 Recovery 的纯数据 Capsule 镜像，与内核及用户态共用 `openwrt-v1` 启动状态协议。

### 1. 早期 ADI 快照与看门狗保护（`sy108_boot_early` / `sy108_boot_late`）

1. **BSS 清零前快照（`sy108_boot_early`）**：若当前非下载模式（`BOOTLOADER_MODE_DOWNLOAD`），在调用依赖 BSS 的常规驱动前，通过 `adi.c`（含 FIFO 排空、BUSY 超时与地址回显校验）直接读取并保存 PMIC RTC 阶段寄存器（`0x250`）、POR 复位源、RTC 复位状态和看门狗挂起标志至 `gd->arch`（`main` 分支 `global_data` 结构体大小为 352 字节，保留偏移 56 处的原厂私有字段）。
2. **启动保护计时**：立即启动约 60 秒的 PMIC 硬件看门狗，并将 RTC 阶段更新为 `0xb45a`（`UDX_RTC_ENTRY_GUARD`，使用架构计数器等待跨时钟域 ACK 并回读核对），同时配置暖复位保留原因。
3. **重定位后模式解析（`sy108_boot_late`）**：在 BSS 清零及板级 ADI/PMIC 初始化完成后，`check_reboot_mode()` 读取 `gd->arch` 中保存的早期快照完成重启原因判定。

### 2. 启动目标判定（`udx_boot_select`）

`sy108_native_boot()` 读取 `misc` UBI 卷（1 MiB 环境区，含 CRC32 与 `udx_boot_state` 字段），结合早期 RTC 阶段值、重启模式与看门狗挂起状态决定本次启动目标：

- **`ready` 状态**：冷启动且无看门狗超时或残留未完成阶段时启动 `boot`（OpenWrt 主系统，就绪态 RTC 为 `0xbe5a`）；若检测到上次启动未完成（残留 `0xb05a` 等中间阶段）或看门狗触发，则自动回退至 `recovery`。
- **`trial` 状态（升级后首次试启动）**：仅当显式请求正常重启（`mode == 2`）且 RTC 阶段为升级移交值 `0xb65a` 时允许启动一次新主系统；在用户态本地服务确认（转为 `ready`）前，任何断电冷启动或启动失败均回退至 `recovery`。
- **`pending` / `recovery` / 校验失败**：固定进入 `recovery`。
- 主系统镜像加载或校验失败时，设置 RTC 为 `0xbc5a`（`UDX_RTC_RECOVERY_READY`）并触发 `CMD_RECOVERY_MODE` 重启进入 Recovery。

### 3. 内存布局与内核移交

选定启动目标（`boot` 卷上限 `133 LEB`，`recovery` 卷上限 `149 LEB`）后，U-Boot 按下表布局校验并装载镜像：

| 用途 | 物理地址 / 范围 | 说明 |
| --- | --- | --- |
| Capsule 读取缓存 | `0x82000000`（`UDX_IMAGE_CACHE`） | 存放从 UBI 卷读取的完整 DHTB + Android + XGZ1 Capsule |
| 内核解压目标 | `0x80200000`（`UDX_IMAGE_DESTINATION`） | `gunzip` 解压后 Linux `Image` 的运行起始地址（上限 `30 MiB`） |
| 设备树（DTB） | `0x85400000`（`UDX_FDT_ADDRESS`） | 展开并执行板级 `fdt_fixup_all()` 后的 DTB（空间 `1 MiB`） |
| Initrd / Ramdisk | `0x85500000`（`UDX_INITRD_ADDRESS`） | 拷贝 Capsule 中的 ramdisk 并写入 `/chosen` 节点 |

装载与移交步骤：

1. **格式与边界校验**：校验 DHTB 头部、Android 头部、SPRD DTB 头部及偏移 128 处的 `XGZ1` 元数据（含 gzip、解压后 Image、ramdisk、DTB 各自的长度与 CRC32）；在复制 DTB/initrd 前及执行板级 `fdt_fixup_all()` 后分别通过 `layout_ok()` 核对所有目标内存区间与 `/memory` 及 `/reserved-memory` 无越界或重叠。
2. **辅助核与 Bootargs 处理**：
   - 启动 OpenWrt 主系统时，按 `s_boot_image_table` 加载 `nr_agdsp` 分区并调用 `modem_entry()`，同时将 `/chosen/bootargs` 根文件系统参数重写为直挂 SquashFS（`ubi.mtd=ubipac ubi.block=0,rootfs root=/dev/ubiblock0_21 rootfstype=squashfs ro rootwait`）。
   - 启动独立 Recovery 时，跳过基带/DSP 加载，保留 initramfs 启动参数。
   - `nr_pmsys`（CM4 电源管理固件）统一交由 Linux 内核在 SIPC 就绪后加载。
3. **跳转内核**：关闭 USB 驱动，将 RTC 阶段更新为 `0xb05a`（`UDX_RTC_IMAGE_ENTRY`），唤醒从核、关闭中断、清理缓存并切换至 EL2，以 `x0 = DTB, x1..x3 = 0` 跳转至 `0x80200000`。

### 4. UART 中断与下载维护模式

- 配置启用 `CONFIG_BOOTDELAY=0` 与 `CONFIG_ZERO_BOOTDELAY_CHECK`：无人值守启动无额外等待延迟；若 UART 接收缓冲区已有输入，可中断自动启动进入命令行。
- 进入命令行、下载模式（`download`）或 Fastboot 时，`sy108_boot_maintenance()` 自动停止启动看门狗计时器；若从命令行再次调用原生启动，则在读取存储前重新启动 PMIC 看门狗并恢复 `0xb45a` 阶段保护。

## 板级维护注意事项

- **UART 引脚矩阵**：UART matrix 在早期 pinmap 与后续 `uport` 初始化阶段分别配置，修改串口相关逻辑时需同时核对两个阶段。
- **NAND / UBI 配置**：ECC、OOB、坏块管理与卷名映射需与 SY108 板级 NAND 参数保持一致。
- **离线仿真模型边界**：`docker/scripts/emulate.py` 在 Unicorn（EL1）中仿真执行真实 U-Boot ELF 指令流，其中 EL2/EL3 系统寄存器访问、PMIC ADI 寄存器响应（含超时/回显异常注入）、存储读取与缓存/MMU 指令由显式桩函数提供，用于回归验证启动逻辑、内存边界与寄存器交接状态。
