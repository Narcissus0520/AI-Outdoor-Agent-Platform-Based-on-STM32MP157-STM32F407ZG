# Repository Structure

本仓库采用 Monorepo 结构，但 MP157 Linux 程序和 STM32F407ZG 固件分目录独立管理、独立编译。

## `mp157/`

`mp157/` 用于 Linux 主控侧程序。

当前模块：

```text
mp157/outdoor-core-service/
```

该目录包含 Outdoor Core Runtime 的独立 CMake 工程，内部保留：

- `include/`
- `src/`
- `config/`
- `data/`
- `tests/`
- `CMakeLists.txt`

## `f407/`

`f407/` 用于 STM32F407ZG Sensor Hub 固件。

当前目录：

```text
f407/sensor-hub/
```

当前只保留目录结构，暂不实现真实 STM32F407ZG 固件代码。

## `common/`

`common/` 用于 MP157 与 F407 共享内容。

当前目录：

```text
common/protocol/
```

后续可放置协议说明、公共头文件、CRC 实现或可被两侧共享的轻量代码。当前阶段不强行抽离 Linux Runtime 内部实现。

## `tools/`

`tools/` 用于 PC 调试工具，例如后续的协议帧生成器、串口调试工具或数据回放辅助工具。

## `scripts/`

`scripts/` 用于仓库级统一构建脚本。

当前脚本：

```text
scripts/build_mp157.sh
```

该脚本进入 `mp157/outdoor-core-service/` 并执行 CMake 配置和编译。
