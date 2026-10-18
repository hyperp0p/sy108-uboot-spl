# SY108 平台说明

本文档说明 Soyealink SY108 5G通信壳（Unisoc UDX710 SoC）在 `oem` 基线分支中的构建目标与板级初始化结构。

## 构建配置

- **U-Boot（`src/u-boot`）**：使用 `udx710_4h10_recon_defconfig`，设备树为 `udx710_4h10_recon`。
- **SPL（`src/chipram`）**：使用 `udx710_3h10_config`，启用 `CONFIG_SPRD_SECBOOT`、`CONFIG_SECURE_EFUSE` 和 `PKCS1_PSS_FLAG`。

## 启动流程职责

- **SPL**：负责早期 SoC 初始化、DDR 训练以及后级镜像加载与安全启动校验。
- **U-Boot proper**：接收前级硬件状态，依次执行 `board_init_f()`、`board_init_r()`、板级 `board_init()`，随后进入启动模式判定与 `cboot()` 内核装载流程。

## 板级维护说明

- **UART 引脚矩阵**：UART matrix 在早期 pinmap 和后续 `uport` 配置阶段分别写入，修改串口初始化时需同时核对两处配置。
- **`global_data` 布局**：`oem` 分支 `global_data` 大小为 320 字节，偏移 56 处的 8 字节私有槽位保留为板级字段。
- **NAND / UBI**：ECC、OOB、坏块管理与卷名映射需与 SY108 板级 NAND 参数一致。
