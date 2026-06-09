# Dev Log

## 2026-06-10 - Stage 0.3

### 本次完成

- 完成 Runtime Manager 与 Service 抽象。
- 新增 `runtime::IService`，定义 `start/run/stop` 生命周期。
- 新增 `runtime::RuntimeManager`，支持顺序启动、运行和停止服务。
- 新增 `services::GnssReplayService`，将 NMEA 文件回放封装为 Runtime 服务。
- `main.cpp` 改为负责配置加载、命令行覆盖和 Runtime 组装。
- 新增 Runtime 服务架构 ADR。

### 修改文件

- `CMakeLists.txt`
- `README.md`
- `docs/adr/0003-runtime-service-architecture.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/runtime/i_service.h`
- `include/runtime/runtime_manager.h`
- `include/services/gnss_replay_service.h`
- `scripts/verify_runtime.ps1`
- `src/main.cpp`
- `src/runtime/runtime_manager.cpp`
- `src/services/gnss_replay_service.cpp`

### 验证结果

- 已执行：

```bash
cmake -S . -B build
cmake --build build
```

- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_runtime.ps1
```

- 验证结果：CMake 构建通过，Runtime 验证脚本通过。

### 后续 TODO

- 推进 Stage 0.4：GNSS mock 服务与最小 NMEA Parser。
- 为 Runtime Manager 补充更标准的 CTest 或单元测试。

## 2026-06-10

### 本次完成

- 完成 Stage 0.2 日志与配置模块增强。
- 新增 `ConfigLoader` 和 `AppConfig`，支持加载 `config/runtime.conf`。
- 日志模块新增 `debug/info/warn/error` 最小日志级别过滤。
- 主程序支持 `--config`、`--input`、`--log-level` 参数，并兼容 Stage 0.1 的直接 NMEA 文件路径用法。
- 新增最小 PowerShell 验证脚本。
- 新增配置格式 ADR。

### 修改文件

- `.gitattributes`
- `CMakeLists.txt`
- `README.md`
- `config/runtime.conf`
- `docs/adr/0002-use-simple-key-value-config.md`
- `docs/dev_log.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `include/config/app_config.h`
- `include/config/config_loader.h`
- `include/log/logger.h`
- `scripts/verify_stage0_2.ps1`
- `src/config/config_loader.cpp`
- `src/log/logger.cpp`
- `src/main.cpp`

### 验证结果

- `cmake -S . -B build` 未执行成功：当前环境中 `cmake` 不在 PATH。
- 已执行：

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify_stage0_2.ps1
```

- 补充验证结果：脚本使用本机 `g++` 成功编译，并运行 `outdoor_core_runtime` 读取配置文件与样例 NMEA 数据。
- 脚本同时检查默认运行输出包含 NMEA 行，并检查 `--log-level warn` 会抑制 INFO 日志。

### 后续 TODO

- 安装或配置 CMake 后执行标准 CMake 构建验证。
- 推进 Stage 0.3 Runtime Manager 与 Service 抽象。

## 2026-06-09

### 本次完成

- 完成 Stage 0.1 Outdoor Core Runtime 工程骨架。
- 新增简单日志模块、输入源抽象和 NMEA 文件回放实现。
- 新增样例 NMEA 文件，并实现主循环逐行读取和日志打印。

### 修改文件

- `.gitignore`
- `CMakeLists.txt`
- `README.md`
- `docs/adr/0001-use-cmake.md`
- `docs/project_design.md`
- `docs/stage0_plan.md`
- `docs/dev_log.md`
- `include/input/i_input_source.h`
- `include/input/file_replay_input.h`
- `include/log/logger.h`
- `src/input/file_replay_input.cpp`
- `src/log/logger.cpp`
- `src/main.cpp`
- `data/nmea_sample.txt`

### 验证结果

- `cmake -S . -B build` 未通过：当前环境中 `cmake` 不在 PATH。
- `cmake --build build` 未执行：CMake 配置阶段无法运行。
- 已使用本机 `g++` 进行补充验证：

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/input/file_replay_input.cpp src/log/logger.cpp -o outdoor_core_runtime.exe
./outdoor_core_runtime.exe
```

- 补充验证结果：程序成功读取 `data/nmea_sample.txt` 并逐行打印 NMEA 日志。

### 后续 TODO

- 补充自动化测试或验证脚本。
- 继续推进 Stage 0.2 日志与配置模块增强。
