# ADR-0008: F407 Cube Source and CMake Build Boundary

## 状态

Accepted

## 背景

仓库已经导入 STM32CubeMX 为 `STM32F407ZGTx` 生成的 HAL 基础工程，同时已有一套项目自行维护的纯 C Sensor Hub 应用骨架。长期目标是由仓库直接完成固件编译，而不是依赖 IDE 手工维护工程文件。

需要明确 Cube 生成文件、项目业务代码和构建系统的所有权边界，避免后续重新生成 Cube 工程时覆盖协议与应用代码。

## 可选方案

### 方案 A：继续只使用 MDK-ARM 工程

优点：

- 可直接使用 CubeMX 当前生成的 Keil 工程。
- 初期上板路径短。

缺点：

- 构建依赖 Keil 环境，不利于命令行和 CI。
- 工程文件中的源文件清单容易与仓库结构漂移。

### 方案 B：让 CubeMX 生成 Makefile 工程

优点：

- 可使用 GNU Arm Embedded Toolchain。
- 比手工建立编译参数更快。

缺点：

- 生成 Makefile 与仓库其他 CMake 工程不统一。
- 重新生成时需要持续审查构建文件变化。

### 方案 C：保留 Cube 源码基线，仓库维护独立 CMake 构建

优点：

- 可统一使用 CMake、命令行和 CI。
- Cube 负责硬件初始化，项目负责业务代码与构建入口，边界清晰。
- 可以显式控制编译选项、链接脚本和产物。

缺点：

- 首次需要补齐 GNU 启动文件、链接脚本和 toolchain 文件。
- Cube 外设变化后需同步维护 CMake 源文件清单。

## 最终决策

采用方案 C。

CubeMX 生成内容固定放在 `f407/sensor-hub/firmware/stm32cube/`。项目自有应用、BSP、协议和传感器代码放在同级的 `app/`、`bsp/`、`protocol/` 和 `sensors/`。仓库后续在 `firmware/` 维护 GNU ARM CMake 构建入口。

MDK-ARM 工程暂时保留，作为 Cube 生成基线和早期对照，但不作为长期唯一构建入口。

## 决策理由

该方案兼顾 CubeMX 外设配置效率、项目代码可维护性和作品集需要展示的独立固件工程能力，也便于后续加入自动构建和产物检查。

## 风险

- CubeMX 重新生成后，HAL 源文件集合可能变化。
- GNU 和 Arm Compiler 的启动文件、链接语义不同，不能直接复用当前 Keil 汇编启动文件。
- 链接脚本中的 Flash、SRAM、栈和堆布局必须与 STM32F407ZG 实际资源一致。

## 后续 TODO

- [x] 新增适配 GCC 的 STM32F407xx startup 和中断向量实现。
- [x] 新增 STM32F407ZG GNU linker script。
- [x] 新增 `arm-none-eabi-gcc` CMake toolchain 文件。
- [x] 新增固件 `CMakeLists.txt` 并生成 ELF、HEX、BIN 和 map。
- 接入项目自有 C 固件代码，再完成 UART 与上板验证。
