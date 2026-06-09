# Dev Log

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
