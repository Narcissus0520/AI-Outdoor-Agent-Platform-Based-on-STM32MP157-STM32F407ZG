# Troubleshooting Log

本文档用于持续记录 AI Outdoor Agent Platform 开发和联调中遇到的问题、排查路径、失败尝试、根因、修复方案与验证证据。它既是工程问题闭环记录，也是后续面试复盘的事实输入。

`docs/dev_log.md` 继续记录“某次开发完成了什么”，本文件专门回答“问题为什么发生、如何定位、哪些尝试无效、最终凭什么确认解决”。重要架构取舍仍需单独记录到 `docs/adr/`。

## 维护规则

- 问题一经确认需要分析，应先创建问题 ID，并将状态设为“分析中”；不能等问题解决后只补最终答案。
- 必须记录问题现象、影响范围、运行环境、接线与供电前提、复现步骤、预期结果和实际结果。
- 必须保留排查假设和每次尝试，包括无效方案、回退方案及其失败证据，禁止只写最终成功方案。
- “根因”必须有日志、状态位、抓包、测量结果、测试或代码路径支撑；尚未证实时应写“根因未完全确认”，不能把推测写成事实。
- 修复后必须记录验证命令、验证时长、关键指标和实际结果；未执行的测试必须明确标为“未执行”。
- 问题关闭后如再次出现，不覆盖旧结论，应追加新证据并重新打开，或创建关联问题 ID。
- 涉及重要技术选择时关联 ADR；涉及阶段性代码变更时同步关联开发日志、提交或修改文件。
- 每条记录应提炼“面试讲述要点”，但必须以工程证据为基础，不能使用脱离实际的包装性描述。

## 状态与证据等级

问题状态：

- `分析中`：正在收集证据和验证假设。
- `已定位`：已有证据支持根因，但修复或回归尚未完成。
- `已修复待验证`：修复已实现，尚未达到约定的验证门槛。
- `已关闭`：根因、修复和验证均已形成闭环。
- `已恢复，根因未完全确认`：当前功能恢复，但仍缺少足以确认根因的证据。
- `长期观察`：短时验证通过，但仍需耐久或故障注入验证。

证据等级：

- `A`：有可重复命令、自动化测试、串口抓包、回读校验或板端状态文件等直接证据。
- `B`：有开发日志、ADR、代码差异或人工记录支撑，但缺少可重复的原始产物。
- `C`：过程补录或经验判断，尚缺独立证据，只能作为后续排查线索。

## 问题索引表

| ID | 日期 | 模块 | 问题摘要 | 状态 | 证据等级 | 详细记录 |
| --- | --- | --- | --- | --- | --- | --- |
| TRB-20260718-001 | 2026-07-18 | F407 UART4 RX | 主循环轮询期间丢失 MP157 连续下行命令 | 已关闭 | A | [记录](#trb-20260718-001-uart4-轮询接收丢失下行命令) |
| TRB-20260719-001 | 2026-07-19 | F407 I2C2 | 共享 I2C2 运行期锁死，三路传感器同时停发 | 长期观察 | A | [记录](#trb-20260719-001-共享-i2c2-运行期锁死) |
| TRB-20260719-002 | 2026-07-19；2026-07-21 复发 | F407 Bootloader | COM6 下载出现 `0xF9`、芯片 ID 位翻转和回读不一致 | 已恢复，根因未完全确认 | A | [记录](#trb-20260719-002-com6-bootloader-下载异常-0xf9) |
| TRB-20260719-003 | 2026-07-19 | ICM42688 INT1 | 事件合并与阻塞发送导致 IMU 实测频率低于 100 Hz | 已关闭 | A | [记录](#trb-20260719-003-imu-频率低于-100-hz) |
| TRB-20260719-004 | 2026-07-19 | ICM42688 供电/初始化 | 未整板复电时 ICM42688 不 ACK，Mock 输出可能造成误判 | 已恢复，根因未完全确认 | A | [记录](#trb-20260719-004-icm42688-不-ack-与-mock-验收误判风险) |
| TRB-20260719-005 | 2026-07-19 | ICM42688 FIFO/I2C | FIFO 读取误判、失败重放和临时参数实验 | 长期观察 | A/C | [记录](#trb-20260719-005-fifo-读取误判与恢复硬化) |
| TRB-20260719-006 | 2026-07-19 | IMU 时间轴 | FIFO 批次、fallback 和恢复切换造成时间戳回退或大间隔 | 已关闭 | A | [记录](#trb-20260719-006-imu-时间戳回退与大间隔) |
| TRB-20260719-007 | 2026-07-19 | MP157 Runtime | 顺序服务调度阻塞持续采集与常驻仪表盘并行运行 | 已修复待板端长测 | A | [记录](#trb-20260719-007-mp157-runtime-顺序调度阻塞) |
| TRB-20260719-008 | 2026-07-19 | MP157 History Test | 新增历史记录测试引用不存在的 `ImuSample::accelZG` 导致 MSVC 编译失败 | 已关闭 | A | [记录](#trb-20260719-008-history-recorder-测试字段名错误) |
| TRB-20260719-009 | 2026-07-19 | MP157 Board IMU | 新 Runtime 板端联合测试前 `/dev/icm20608` 设备节点缺失 | 已关闭 | A | [记录](#trb-20260719-009-mp157-devicm20608-设备节点缺失) |
| TRB-20260719-010 | 2026-07-19 | Board Test Harness | PowerShell 串口启动命令的嵌套引号导致命令未发送 | 已关闭 | A | [记录](#trb-20260719-010-powershell-串口启动命令解析失败) |
| TRB-20260719-011 | 2026-07-19 | MP157 History Performance | framebuffer 联合运行时 MCU IMU CSV 仅约 57 Hz | 已关闭 | A | [记录](#trb-20260719-011-framebuffer-联合运行时-mcu-imu-history-约-57-hz) |
| TRB-20260719-012 | 2026-07-19 | Board Test Harness | 串口预检把命令回显中的结束标记误判为板端执行完成 | 已关闭 | A | [记录](#trb-20260719-012-串口预检结束标记被命令回显提前命中) |
| TRB-20260719-013 | 2026-07-19 | Board Test Harness | 超长串口启动命令被截断并使 BusyBox shell 进入未完成 `if` 续行状态 | 已关闭 | A | [记录](#trb-20260719-013-超长串口启动命令被截断) |
| TRB-20260719-014 | 2026-07-19 | Board Test Harness | 异常恢复脚本未识别已有 Runtime，污染独占长测 | 已关闭 | A | [记录](#trb-20260719-014-已有-runtime-保护未生效) |
| TRB-20260719-015 | 2026-07-19 | F407 COM6 Verifier | 尝试无复位打开 COM6 实际重置 F407 并使 ICM42688 回退 Mock | 已关闭 | A | [记录](#trb-20260719-015-com6-skipreset-捕获-0-字节) |
| TRB-20260719-016 | 2026-07-19 | MP157 Boot | `systemd-modules-load` 在真实软重启中加载 ICM20608 失败 | 已关闭 | A | [记录](#trb-20260719-016-systemd-modules-load-启动失败) |
| TRB-20260719-017 | 2026-07-19 | Board Test Harness | `systemctl` 自动分页吞掉后续串口诊断命令 | 已关闭 | A | [记录](#trb-20260719-017-systemctl-串口分页阻塞) |
| TRB-20260719-018 | 2026-07-19 | GNSS Fix Verifier | 全局 `last_error` 检查会把正常 NMEA 跳过误判为系统故障 | 已修复待室外正向验证 | A | [记录](#trb-20260719-018-gnss-last_error-误判) |
| TRB-20260719-019 | 2026-07-19 | GNSS Fix Verifier | GNSS 验收复用含故意 CRC 错帧的 MCU 负向 fixture | 已关闭 | A | [记录](#trb-20260719-019-gnss-验收复用负向-mcu-fixture) |
| TRB-20260719-020 | 2026-07-19 | Board Test Harness | 长测仅在结束时检查真实传感器健康状态，可能浪费一小时并产生无效数据 | 已关闭 | A | [记录](#trb-20260719-020-正式长测缺少真实传感器健康预检) |
| TRB-20260719-021 | 2026-07-19 | Board Endurance Audit | 启动健康但运行中出现短暂 FIFO fallback，最终快照会掩盖降级 | 已修复待板端长测验证 | A/B | [记录](#trb-20260719-021-最终快照掩盖运行期-fifo-fallback) |
| TRB-20260719-022 | 2026-07-19 | Build/Test Harness | 多配置 CTest 调用错误和 MSVC abort 弹窗导致自动化验证无返回 | 已关闭 | A | [记录](#trb-20260719-022-ctest-配置与-msvc-断言弹窗阻塞) |
| TRB-20260719-023 | 2026-07-19 | Windows Test Harness | Git Bash 已安装但未加入 PowerShell PATH，首轮 shell 语法检查未执行 | 已关闭 | A | [记录](#trb-20260719-023-git-bash-未加入-powershell-path) |
| TRB-20260719-024 | 2026-07-19 | F407 FIFO/I2C2 | 逐秒 heartbeat 全健康，但累计诊断显示大量 FIFO empty/skipped、I2C 恢复和最终失败 | 分析中 | A | [记录](#trb-20260719-024-健康-heartbeat-掩盖累计-fifoi2c-异常) |
| TRB-20260719-025 | 2026-07-19 | Compass Verification | 罗盘算法单测通过但 Runtime/dashboard 集成验证连续失败 | 已关闭 | A | [记录](#trb-20260719-025-罗盘集成验证缺少输入且过早采样) |
| TRB-20260719-026 | 2026-07-19 | Board Test Harness | `/tmp` 清理命令因 PowerShell/远端 shell 嵌套引号解析失败 | 已关闭 | A | [记录](#trb-20260719-026-板端临时文件清理命令嵌套引号失败) |
| TRB-20260719-027 | 2026-07-19 | MP157 Logger Health | 日志轮转/写入失败只输出 stderr，无法进入无人值守验收状态 | 已关闭 | A/B | [记录](#trb-20260719-027-logger-失败只写-stderr-无法审计) |
| TRB-20260719-028 | 2026-07-19 | F407/MP157 UART4 Diagnostics | RX 环形缓冲溢出只有 heartbeat 布尔位，无法累计审计丢失规模 | 已关闭 | A | [记录](#trb-20260719-028-uart4-rx-丢弃只有布尔位无法累计审计) |
| TRB-20260719-029 | 2026-07-19 | Board Test Harness | 手工预检把 Runtime 路径误传为第一个时长参数 | 已关闭 | A | [记录](#trb-20260719-029-预检位置参数顺序误用) |
| TRB-20260719-030 | 2026-07-19 | Compass Calibration Tool | 首轮拟合测试挂起、强磁离群点使椭球非正定且 Release 断言假绿 | 已关闭 | A | [记录](#trb-20260719-030-罗盘校准器测试挂起与离群点拟合失败) |
| TRB-20260719-031 | 2026-07-19 | Repository Hygiene | 罗盘工具 GCC `build-gcc` 产物未被现有忽略规则覆盖 | 已关闭 | A | [记录](#trb-20260719-031-罗盘工具-build-gcc-产物未被忽略) |
| TRB-20260720-032 | 2026-07-20 | F407 Diagnostic Build | 新建隔离构建目录时 toolchain/Ninja 参数未可靠解析 | 已关闭 | A | [记录](#trb-20260720-032-f407-隔离构建参数解析失败) |
| TRB-20260721-033 | 2026-07-21 | MP157 Build Baseline | 复用已有 `build` 目录时 CMake 生成器冲突 | 已关闭 | A | [记录](#trb-20260721-033-复用已有-build-目录时-cmake-生成器冲突) |
| TRB-20260721-034 | 2026-07-21 | F407 Host Tests | Release 禁用 `assert` 导致测试副作用丢失、假绿与数值异常 | 已关闭 | A | [记录](#trb-20260721-034-release-禁用-assert-导致测试副作用丢失与假绿) |
| TRB-20260721-035 | 2026-07-21 | MP157 ARM Build | PowerShell 将交叉编译器变量作为字面量传给 CMake | 已关闭 | A | [记录](#trb-20260721-035-powershell-将交叉编译器变量作为字面量传给-cmake) |
| TRB-20260721-036 | 2026-07-21 | MP157 Unix IPC Test | ARM 测试目标使用 `std::thread` 但未显式链接 pthread | 已关闭 | A | [记录](#trb-20260721-036-arm-测试目标未显式链接-pthread) |
| TRB-20260721-037 | 2026-07-21 | Runtime Supervision Tests | PowerShell 把预期失败子进程 stderr 提升为终止错误 | 已关闭 | A | [记录](#trb-20260721-037-powershell-把预期失败子进程-stderr-提升为终止错误) |

## 问题详细复盘

### TRB-20260718-001: UART4 轮询接收丢失下行命令

| 字段 | 记录 |
| --- | --- |
| 问题现象 | MP157 连续发送三个合法 `command_ping`，F407 均未返回 `command_ack`。 |
| 影响范围 | F407 至 MP157 的上行传感器帧正常，但 MP157 至 F407 的下行控制链路不可靠。 |
| 环境与前提 | F407 UART4 PC10/PC11 对接 MP157 USART3 PD9/PD8，115200 8N1，共地；F407 当时使用主循环零超时轮询 RX，并执行阻塞式传感器帧发送和 USART1 镜像。 |
| 根因 | 主循环被阻塞式发送占用时无法及时轮询 UART4，连续命令字节到达后被丢失。 |
| 最终方案 | 改为 HAL 单字节中断接收，将字节写入固定 64 字节环形缓冲；命令 decoder 仍保留在主循环，避免在 ISR 中解析协议。 |
| 验证结果 | 修复后连续三个 ping 均返回 status 0、nonce `0x50494E47` 的 ACK；MP157 Runtime 状态文件也确认 `command_ack_seen=true`。 |
| 关联资料 | [开发日志](dev_log.md#2026-07-18---f407mp157-bidirectional-board-validation)、[ADR-0013](adr/0013-use-uart4-interrupt-rx-ring-buffer.md) |

排查与方案比较：

| 顺序 | 假设或尝试 | 结果 | 结论 |
| --- | --- | --- | --- |
| 1 | 检查上行链路和命令帧格式 | F407 上行帧正常，发送的 ping 合法，但无 ACK | 物理 TX 主链路和基本协议不是主要故障域 |
| 2 | 保留主循环零超时轮询 | 阻塞发送期间持续丢失连续字节 | 轮询不适合当前主循环最坏延迟 |
| 3 | 改用 RX 中断和固定环形缓冲 | 三次连续命令全部收到 ACK | 修复有效，不需要在当前带宽下引入 DMA |

面试讲述要点：通过“上行正常、下行连续失败”缩小故障域，再结合主循环阻塞时间定位轮询接收的时序缺陷；最终使用 ISR 只搬运数据、主循环解析协议的分层方式控制中断复杂度。

### TRB-20260719-001: 共享 I2C2 运行期锁死

| 字段 | 记录 |
| --- | --- |
| 问题现象 | 30 秒持续抓包约 27 秒后三颗真实传感器同时停止上报，heartbeat 从 `0x0029` 变为 `0x0056`。 |
| 影响范围 | ICM42688、MMC5603、BMP390 共用的 F407 I2C2 数据路径。 |
| 环境与前提 | PB10 为 I2C2_SCL，PB11 为 I2C2_SDA；三颗器件共享总线；原实现只在启动阶段释放总线，运行期 HAL I2C 事务失败后直接返回错误。 |
| 根因结论 | 已确认故障域位于共享 I2C2 或控制器状态：三路传感器同时停发且 heartbeat 仍可报告错误。运行期失败后外设可能停留在 BUSY，或从机可能保持 SDA 低；触发该状态的具体器件或电气原因尚未完全定位。 |
| 最终方案 | 首次事务失败后执行 HAL 去初始化、I2C2 外设硬复位、PB10/PB11 开漏 GPIO 总线释放、最多 18 个 SCL 脉冲、显式 STOP，恢复复用后对普通寄存器事务重试一次。 |
| 验证结果 | 修复后 60 秒抓取 7532 帧，三路数据持续输出，最终 heartbeat `0x0029`；随后两次独立复位 30 秒回归均通过。最终 FIFO 固件的 60 秒验收也保持三路输出。 |
| 剩余风险 | 最终长测仍累计出现 I2C recovery 100、最终失败 1；需要小时级耐久、SDA 卡住和传感器断线/复接故障注入。 |
| 关联资料 | [开发日志](dev_log.md#2026-07-19---f407-i2c2-sensor-interaction-recovery)、[ADR-0014](adr/0014-recover-i2c2-after-runtime-errors.md) |

排查与方案比较：

| 顺序 | 假设或尝试 | 结果 | 结论 |
| --- | --- | --- | --- |
| 1 | 分别怀疑单个传感器 provider | 三路真实数据在同一时段停发 | 单个 provider 不是最符合现象的公共故障点 |
| 2 | 怀疑 UART 输出链路 | heartbeat 仍能输出并改变错误位 | UART 不是三路采样同时停止的根因 |
| 3 | 仅等待人工复位 | 可恢复但无法满足长期运行 | 拒绝作为最终方案 |
| 4 | 仅重新调用 HAL I2C 初始化 | 无法保证释放 SDA 或清除所有外设 BUSY 状态 | 恢复动作不完整 |
| 5 | 硬复位外设、GPIO 释放总线、STOP、单次重试 | 60 秒和独立复位回归通过 | 作为当前受控恢复策略 |

面试讲述要点：通过“多个消费者同时失败”定位共享资源故障域；恢复策略放在 BSP 层覆盖全部 provider，并用单次重试限制永久硬件故障时的阻塞上界。

### TRB-20260719-002: COM6 Bootloader 下载异常 `0xF9`

| 字段 | 记录 |
| --- | --- |
| 状态 | 已恢复，根因未完全确认；2026-07-21 端口从 COM6 重枚举为 COM3 后，400 kHz 基线重新烧录和逐字节回读通过 |
| 问题现象 | COM6 ROM Bootloader 在固件写入阶段返回异常字节 `0xF9`，该次下载没有完成。 |
| 影响范围 | F407 固件烧录过程；不能将失败下载后的镜像用于板端验证。 |
| 环境与前提 | STM32 ROM UART Bootloader，COM6，一键下载电路，烧录脚本包含擦除、写入、回读和 GO。 |
| 根因结论 | 当前只确认是一次下载事务的瞬态异常，没有足够证据区分串口时序、Bootloader 会话状态或一键下载控制信号，因此根因未完全确认。 |
| 处置 | 明确将该次标记为失败；重新进入全新的 Bootloader 会话，重新擦除、写入并逐字节回读。 |
| 验证结果 | 后续镜像写入和逐字节回读通过，Bootloader version `0x31`、chip ID `0x0413`；应用态抓包验证另行通过。 |
| 后续动作 | 如果再次发生，保存原始串口收发、命令阶段、DTR/RTS 时序和重试次数，再决定是否修改脚本。 |
| 2026-07-21 复发 | 为部署 ICM-only 100 kHz A/B 镜像，首次全新会话正常读取 Bootloader `0x31`、chip ID `0x0413`，但在首个 `Write Memory address` 阶段收到 `0xF9`，该会话作废。按既有处置重新进入第二个全新会话后，脚本读到异常 chip ID `0x0493`，写入阶段完成但逐字节回读在 `0x0800013D` 首次不一致。两次均未计为烧录成功；第二次已执行全擦/写入，因此当前 F407 应用 Flash 明确标为不可信，不能启动 100 kHz A/B。 |
| 本轮只读检查 | Windows 仍将设备枚举为 `USB-SERIAL CH340 (COM6)`、状态 OK；未发现 Python、PuTTY、串口终端等进程占用。`0x0413 -> 0x0493` 的单比特变化和独立回读不一致支持“COM6 传输/物理连接不稳定”方向，但没有原始字节波形，根因仍未完全确认。 |
| 当前处置 | 连续两次失败后停止自动重试；先关闭所有 COM6 工具并物理拔插对应 USB/USB-UART，等待重新枚举，再从新的 Bootloader 会话重新全擦、写入和逐字节回读。不得使用 `-ReadoutUnprotectFirst`，因为当前没有读保护 NACK 证据，额外解保护不能解决传输位错误。 |
| 2026-07-21 恢复结果 | 用户物理拔插 COM6 对应 USB/USB-UART 并保持终端关闭后，Windows 重新枚举同一 CH340 设备为 OK。新的 Bootloader 会话读取 version `0x31`、chip ID `0x0413`，对 15072 B、SHA256 `56b31d76...c3e56` 的 ICM-only 100 kHz 镜像完成全擦、写入、逐字节回读和 GO，脚本 exit 0。当前 Flash 重新可信；该动作仅为应用热复位，不算传感器 A/B 结果。物理重连能恢复传输，但仍不足以区分 USB/CH340 会话状态、连接接触或控制线时序，根因继续标为未完全确认。 |
| 100 kHz A/B 后的端口重枚举与恢复 | 完整复电并完成 100 kHz 8 秒 A/B 后，准备回退 400 kHz 基线时，烧录脚本在 `SerialPort.Open()` 前返回 `The port 'COM6' does not exist`；没有进入 Bootloader、擦除或写入，板上仍是完整的 100 kHz 镜像。只读枚举当时仅见 COM9，历史 CH340 COM6 实例为 `Present=False/Status=Unknown`。用户随后确认 F407 USB-UART 实际已变为 COM3；PnP 显示 `USB-SERIAL CH340 (COM3)` 为 OK，未发现实际串口终端占用。通过 COM3 的新会话取得 Bootloader `0x31`、chip ID `0x0413`，将 15072 B、SHA256 `c07e235a...2f80a` 的 400 kHz ICM-only 基线完成全擦、写入、逐字节回读和 GO。该事件说明 COM 号是枚举属性，脚本参数必须以当前 PnP 结果为准；仍不足以确认早期位错误的物理根因。 |
| 关联资料 | [开发日志](dev_log.md#2026-07-19---icm42688-int1-trigger-and-sensor-filtering) |

面试讲述要点：烧录成功不能用“脚本最终返回成功”笼统代替；失败会话不计入验证，必须以重新写入、逐字节回读和应用态运行三层证据确认镜像可信。

### TRB-20260719-003: IMU 频率低于 100 Hz

| 字段 | 记录 |
| --- | --- |
| 问题现象 | ICM42688 配置 100 Hz ODR，但 60 秒真实抓包的 IMU 输出约为 `92.45 Hz`。 |
| 影响范围 | IMU 样本保留率和后续姿态处理的时间连续性。 |
| 环境与前提 | PB12 INT1 data-ready 只累计事件，主循环执行共享 I2C2 事务，同时 UART4 和 USART1 使用阻塞发送。 |
| 根因 | 主循环忙于 I2C 或双路串口发送时，多个 data-ready 事件被合并；读取“最新样本寄存器”无法恢复期间积压的历史样本。 |
| 最终方案 | ICM42688 改为 byte-count FIFO、64 字节 watermark 和可变包批量解析；UART4/USART1 改为固定内存中断 TX 队列，主循环不再等待串口发送。 |
| 验证结果 | 最终整板复电 60 秒抓取的 IMU 为 `101.43 Hz`，最大时间戳间隔 10 ms，CRC、协议、载荷和 TX overflow 检查均通过。 |
| 关联资料 | [开发日志](dev_log.md#2026-07-19---icm42688-fifo-and-interrupt-driven-uart-tx-queues)、[ADR-0016](adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md) |

排查与方案比较：

| 方案 | 优点 | 问题或结果 |
| --- | --- | --- |
| 继续读取最新 14 字节寄存器 | 简单、内存小 | 主循环延迟期间的样本无法恢复 |
| FIFO 固定 16 字节记录假设 | 长度计算简单 | 硬件可能产生 8 字节单传感器包或消息头，容易错位 |
| FIFO byte-count + 可变包解析 | 可保存积压样本并处理真实包型 | 需要批次解析、异常 flush 和时间轴重建；最终采用 |
| UART DMA | CPU 开销更低 | 当前带宽没有证明必须承担 DMA 复杂度，暂不采用 |

面试讲述要点：中断计数不等于样本缓存；当数据寄存器只保留最新值时，软件无法补回被合并事件。FIFO 把“事件通知”与“样本保留”解耦，中断 TX 队列则降低主循环最坏延迟。

### TRB-20260719-004: ICM42688 不 ACK 与 Mock 验收误判风险

| 字段 | 记录 |
| --- | --- |
| 问题现象 | 复电前诊断固件运行时 ICM42688 初始化不再 ACK，heartbeat 为 `0x102E`/`0x502E`；系统仍输出约 98–99 Hz IMU 数据。 |
| 影响范围 | 如果只观察频率和帧格式，Mock fallback 可能被误认为真实 FIFO 数据，造成错误验收。 |
| 环境与前提 | 固件支持 ICM42688 初始化失败后的 Mock IMU fallback；当时只做了 F407 应用复位或重新烧录，未完成 F407 与传感器整板断电。 |
| 根因结论 | 已确认当时的 IMU 帧来自 Mock fallback，而非真实 ICM42688；整板断电上电后真实器件恢复 ACK。器件为何在未复电状态持续不 ACK，尚缺电源、复位脚和总线波形证据，根因未完全确认。 |
| 最终处置 | 将 heartbeat ready/fallback/error 位、USART1-only 初始化诊断和真实数据变化纳入验收；Mock 输出明确不计为 FIFO 通过；完成整板断电上电后重新验收。 |
| 验证结果 | 复电后的最终 60 秒抓取为 heartbeat `0x01A9`，三路真实传感器持续变化，IMU `101.43 Hz`；MP157 主链路复验也通过。 |
| 2026-07-19 再次出现 | 刷入 MMC 轮询修复镜像并回读成功后，首轮预检得到 `4142 (0x102E)`、I2C recovery/failure 均为 52、最后失败为备用地址 `0x68` 的 `REG_BANK_SEL(0x76)` 写入 NACK、init step 1；MMC5603/BMP390 仍 seen，正式测试被预检拒绝。部署最新 MP157 Runtime 后再次执行 8 秒预检，证据目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-C8NBPu` 仍为 `0x102E`；此时 F407 uptime `2605008 ms`，累计 recovery `5211`、最终 failure `5208`、HAL error `4`，最后事务仍为设备 `0x68`、寄存器 `0x76`、operation `2`、HAL status `1`、init step 1。两次预检都证明热复位/持续软件重试未恢复 ICM42688。 |
| 2026-07-19 Logger-health 版本复验 | 部署 258560 B、SHA256 `330b7bc4...a645` 的 MP157 Runtime 后，预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-FImCIn` 仍为 `4142 (0x102E)`；累计 recovery `6636`、最终 failure `6632`、HAL error `4`，最后事务仍为设备 `0x68`、寄存器 `0x76`、operation `2`、HAL status `1`、init step 1。新增 Logger 三项健康门槛及其余门槛全部通过，只有真实 F407 sensor status 失败。 |
| 2026-07-19 diagnostics schema 1 版本复验 | 先部署 258560 B、SHA256 `47c4bcc9...b739` 的 MP157 Runtime，再通过 COM6 烧录并逐字节回读 19376 B、SHA256 `365fb9c3...129c` 的 F407 镜像。COM6 烧录属于热复位，不是传感器完整复电。初始预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-ZVXjOR` 已确认 schema 1、UART4 RX drop 0、Logger 三项健康和其余软件门槛，但仍以 `4142 (0x102E)` 拒绝正式测试；累计 recovery/failure 均为 56，最后事务仍为设备 `0x68`、寄存器 `0x76` 写失败、HAL status 1/error 4、init step 1。完成 RX 压力注入并做 F407 应用复位后，最新预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XOXgzn` 再次通过 schema 1/drop 0，仍只失败于同一 ICM42688 真实传感器门槛。 |
| 2026-07-19 完整复电结果 | 用户确认 F407 与传感器 3.3 V 完整断电再上电后，预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-rjgpa6` 取得 `0x01A9`、真实 IMU、schema 1/drop 0 并全部通过，证明本次不 ACK 状态确由完整复电恢复。随后异常转为独立的运行期 FIFO/I2C 问题 `0x03A9`，不再是 init step 1/Mock fallback。 |
| 后续动作 | 不 ACK 项当前恢复但器件锁存原因仍未完全确认；再次出现时测量 ICM42688 VDD/VDDIO、SCL/SDA、复位时序与 WHO_AM_I 事务。运行期 FIFO/I2C 异常继续在 TRB-024 独立分析，不能把两类故障混为一个根因。 |
| 关联资料 | [开发日志](dev_log.md#2026-07-19---icm42688-fifo-and-interrupt-driven-uart-tx-queues) |

排查关键点：

| 观察 | 错误解释 | 正确结论 |
| --- | --- | --- |
| IMU 输出约 99 Hz | FIFO 已经达到目标频率 | fallback 调度器也可输出约 100 Hz，频率不能证明数据源 |
| CRC 和时间戳正常 | 真实传感器链路正常 | 只能证明协议输出正常 |
| heartbeat `0x102E`/`0x502E` | 可忽略的诊断位 | 指示 ICM 初始化/当前链路异常，不能通过真实传感器验收 |

面试讲述要点：验收指标必须证明数据来源，而不只是证明输出格式。通过 ready/fallback 位、初始化步骤、真实数值变化和整板复电构成多维证据，避免 Mock 掩盖硬件故障。

### TRB-20260719-005: FIFO 读取误判与恢复硬化

| 字段 | 记录 |
| --- | --- |
| 问题现象 | FIFO 联调期间出现累计 recovery、overflow、malformed，以及“读取后 count 未下降”的 drain-stall 判断；部分失败路径可能对 `FIFO_DATA` 重放读取。 |
| 影响范围 | 可能造成误报、FIFO 包边界错位、重复消费数据或不必要的 I2C 恢复。 |
| 关键分析 | FIFO 在主机读取期间仍由传感器并发写入，因此读取前后 count 不保证下降；`FIFO_DATA` 是有副作用的数据流寄存器，失败后无法安全假设读取指针未移动。 |
| 最终方案 | 删除 drain-stall 启发式判断；FIFO_DATA 失败时只恢复 I2C2，不重放原 burst，并 flush FIFO 重新对齐；普通寄存器事务仍允许恢复后重试一次。启用 `FIFO_WM_GT_TH`、最后初始化 ICM，并以最短 30 ms 周期消费事件。 |
| 验证结果 | 主机测试覆盖并发增长、不可重放失败和异常 flush，CTest 5/5；最终 60 秒实测无时间戳回退，CRC/协议/载荷错误和 TX overflow 为 0。 |
| 剩余风险 | 60 秒诊断仍累计 I2C recovery 100、最终失败 1、FIFO overflow/malformed 各 10；已自愈但需要小时级与物理故障注入。 |
| 关联资料 | [最终开发日志](dev_log.md#2026-07-19---icm42688-fifo-final-board-validation-and-recovery-hardening)、[ADR-0016](adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md) |

已回退实验补录：

| 尝试 | 当时目的 | 观察与结论 | 最终状态 | 证据等级 |
| --- | --- | --- | --- | --- |
| I2C 从 100 kHz 临时降至 50 kHz，并将超时放宽到 80 ms | 判断总线速度或超时是否造成 FIFO 读取失败 | 未形成可以证明问题消失的稳定结果，且降低 FIFO drain 吞吐；最终恢复 100 kHz，并按 256 字节理论传输时间采用 30 ms 超时 | 已回退 | C |
| FIFO count 与 FIFO_DATA 读取间增加 1 ms 延迟 | 等待 FIFO 计数和数据稳定 | 未观察到可复用的稳定收益，额外扩大主循环延迟窗口 | 已回退 | C |
| 将 FIFO_DATA 改为 16 字节分段读取并尝试断点续读 | 缩短单次 I2C 事务并在失败后继续 | 畸形包风险和状态复杂度增加；有副作用的数据流读取无法安全判断失败前已消费的字节数 | 已回退 | C |
| 读取前后比较 FIFO count，未下降即判为 stall | 检测 FIFO 未被消费 | FIFO 可并发增长，产生误报并触发不必要恢复 | 已删除 | A |

以上前三项来自本次联调过程补录，仓库没有保存对应临时固件、逐次抓包和原始计数，因此只能作为 C 级过程线索，不能在面试中表述为具有完整实验数据的确定性结论。

面试讲述要点：区分普通寄存器的幂等读写与 FIFO 数据端口的破坏性读取；恢复策略不能机械地“所有失败都重试”，必须考虑事务副作用和无法观察的部分完成状态。

### TRB-20260719-006: IMU 时间戳回退与大间隔

| 字段 | 记录 |
| --- | --- |
| 问题现象 | FIFO 批次回推时间、真实/Mock fallback 切换或 I2C 恢复后，发布时间戳可能相对上一帧回退或出现过大空洞。 |
| 影响范围 | MP157 数据排序、后续积分/姿态融合和验证脚本的时间连续性判断。 |
| 根因 | 各批次若独立使用“当前时刻减去批次长度”回推首样本，会与上一批已发布时间重叠；fallback 和恢复路径使用不同时间基准会进一步放大不连续。 |
| 最终方案 | 新增真实/Mock 共用的时间轴归一化模块；批次整体锚定到上一已发布样本之后，保留批内 10 ms 相对间隔，所有数据源使用同一发布调度时间轴。 |
| 验证结果 | 主机测试覆盖批次衔接和 fallback 切换；最终 60 秒实测时间戳无回退，最大间隔 10 ms。 |
| 关联资料 | [最终开发日志](dev_log.md#2026-07-19---icm42688-fifo-final-board-validation-and-recovery-hardening)、[ADR-0016](adr/0016-use-icm42688-fifo-and-interrupt-uart-tx-queues.md) |

面试讲述要点：硬件采样时间、主机读取时间和消息发布时间不是同一个概念。当前没有读取硬件 FIFO 时间戳，因此明确采用连续的发布时序模型，并用单元测试和长抓包验证其单调性。

### TRB-20260719-007: MP157 Runtime 顺序调度阻塞

| 字段 | 记录 |
| --- | --- |
| 状态 | 已修复待板端长测 |
| 问题现象 | `RuntimeManager::run()` 会完整执行一个服务后才进入下一个服务；当 `DashboardService` 使用 `dashboard_refresh_count = 0` 常驻刷新时，排在其后的服务永远没有运行机会，排在其前面的串口服务也只能先完成固定时长采集，无法与仪表盘持续并行。 |
| 影响范围 | MP157 GNSS、MCU、板载 IMU、状态发布和常驻 framebuffer 仪表盘无法组成长期同时运行的 Runtime。 |
| 复现与证据 | `runtime_manager.cpp` 使用 `for` 循环逐个调用阻塞式 `service->run()`；`dashboard_service.cpp` 在 refresh count 为 0 时进入无限刷新循环。代码路径可确定性复现，不依赖硬件。 |
| 根因结论 | 当前 `IService::run()` 是“运行至完成”的阻塞接口，Runtime Manager 没有短周期调度点，也没有运行期状态发布回调。直接为每个服务创建线程还会使三个共享状态对象出现未同步读写，因此不能只做线程包装。 |
| 最终方案 | 将服务生命周期演进为单线程协作式 `poll()`：每次只处理一个非阻塞步骤并返回 running/completed/failed；Runtime Manager 轮询所有活动服务并周期发布状态。串口和 launcher 使用非阻塞 I/O，有限采集保持原有结束语义，计数/时长为 0 时用于常驻运行。增加 SIGINT/SIGTERM、`runtime_run_seconds` 和 active/completed service 计数。 |
| 验证结果 | Runtime Manager 单元测试覆盖服务交错、完成、失败、停止条件、循环回调和启动回滚；MP157 CTest 8/8、`verify_runtime.ps1` 和 ARMv7 hard-float 交叉构建通过。真实板端 60 秒、20 秒联合运行已通过。首次 PID `2005` 因竞争进程污染作废；第二次 PID `3127` 又因实验性 COM6 打开重置 F407、健康位变为 `0x102E` 而作废，二者均经 SIGTERM 1 秒内受控停止并保留负向证据。 |
| 剩余风险 | 当前没有有效的 3600 秒测试在运行；需要完整断电上电恢复真实 ICM42688 `0x01A9` 后，从全新目录重新启动。当前状态保持“已修复待板端长测”。evdev launcher 本次未启用。 |
| 关联资料 | [ADR-0017](adr/0017-use-cooperative-runtime-service-scheduler.md)、[Stage 1 计划](stage1_plan.md#stage-114-mp157-cooperative-runtime-scheduler)、[Bring-up 计划](stage1_bringup_plan.md) |

排查与方案比较：

| 顺序 | 假设或方案 | 依据 | 结论 | 证据等级 |
| --- | --- | --- | --- | --- |
| 1 | 保持顺序模型，仅调整服务排列顺序 | 无限服务无论放在哪里都会阻塞另一侧服务 | 无法满足持续采集与持续显示 | A |
| 2 | 每个服务直接使用一个线程 | 改动表面较小，但 GNSS/MCU/Board IMU 状态会被采集线程和 dashboard/status publisher 并发访问 | 在没有状态同步模型前不可直接采用 | A |
| 3 | 单线程协作式轮询 | 当前串口已使用 non-blocking fd，采样和 dashboard 都可以拆成短步骤；无需新增线程同步和第三方依赖 | 选为当前实现方向 | A |

面试讲述要点：问题表面是 dashboard 无限循环，实质是生命周期接口把“服务存活”与“单次执行”混在一起。方案选择时比较了调序、线程和协作式轮询；最终用单线程事件循环避免共享状态数据竞争，再用失败路径单测、持续模式回归和 ARM 构建分层验证，同时明确保留真实整机小时级验证边界。

### TRB-20260719-008: History Recorder 测试字段名错误

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | 新增 `history_recorder_tests.cpp` 后，MSVC 报错 `C2039: "accelZG": 不是 "outdoor::protocol::ImuSample" 的成员`，构建在该测试目标处失败。 |
| 影响范围 | 仅新增 History Recorder 单元测试无法编译；`outdoor_core` 库和 Runtime 主程序已完成编译链接。 |
| 复现步骤 | 在 `mp157/outdoor-core-service` 执行 `cmake --build build --config Debug`。 |
| 根因结论 | 测试夹具混用了转换后的 g 单位命名与协议原始结构命名。`ImuSample` 在 `common/protocol/imu_payload.h` 中定义的字段为 `accelZMg`，不存在 `accelZG`。 |
| 最终方案 | 将测试赋值字段从 `accelZG` 改为 `accelZMg`，不修改生产接口。 |
| 验证结果 | MP157 Debug 全量构建成功，CTest 8/8 通过；`verify_runtime.ps1` 通过；包含该测试目标的 ARMv7 hard-float 交叉构建成功。 |
| 剩余风险 | 无该字段错误相关剩余风险；真实 SD 卡耐久边界属于 History Recorder 功能验收，不属于本次测试夹具编译问题。 |
| 证据等级 | A：编译器原始错误与结构体定义可直接对应。 |

面试讲述要点：这类错误不应包装成复杂问题。关键是根据编译器类型/成员定位到测试夹具的单位命名混用，保持生产接口不动，并用完整回归确认修复没有掩盖其他问题。

### TRB-20260719-009: MP157 `/dev/icm20608` 设备节点缺失

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | 上传新 ARM Runtime 并执行板端联合测试预检时，`ls` 报告 `cannot access '/dev/icm20608': No such file or directory`。 |
| 影响范围 | 新 Runtime 暂时不能启用 `--board-imu` 完成 Board IMU 与双串口、framebuffer、SD history 的联合冒烟测试；不影响二进制上传和 SHA256 校验。 |
| 环境与前提 | STM32MP157 root console COM9，板端目录 `/tmp/ai_outdoor_runtime_validation_20260718`；本地/板端新可执行文件 SHA256 均为 `26184ec8c3755a7bd97dcc989e64b21bed9b90ed83ced23e289e6ef3000cab39`。历史验证表明该字符设备由 `icm20608.ko` 加载后生成。 |
| 根因结论 | 设备节点缺失时 `lsmod` 未显示 ICM20608 模块；执行 `modprobe icm20608` 返回 0 后，内核立即输出 `ICM20608 ID = 0XAE` 并创建字符设备 `240:0`。根因是本次启动未加载驱动模块，不是 SPI、设备树或接线故障。 |
| 恢复操作 | 执行 `modprobe icm20608`；随后使用新 Runtime 完成 60 秒和 20 秒联合运行，Board IMU 分别记录 479 和 160 条状态数据且 `last_error` 为空。 |
| 最终闭环 | 初版 modules-load.d 在手工服务重启时有效，但真实 boot 因板厂模块链接顺序失败；最终改用依赖 `link-modules.service` 的 `outdoor-agent-icm20608.service`。第二次软重启确认服务 active/enabled、probe ID `0xAE` 和设备节点自动出现。 |
| 剩余风险 | 当前已闭环 MP157 软件重启；整机物理断电仍属于独立的 SD/硬件恢复验收，不影响本问题的驱动加载根因。 |
| 证据等级 | A：真实板端命令输出。 |

面试讲述要点：设备节点缺失先按 Linux 驱动生命周期分层排查，避免直接归因为传感器或接线故障；“模块未加载”“probe 失败”和“节点权限/udev”需要不同证据。

### TRB-20260719-010: PowerShell 串口启动命令解析失败

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | 准备通过 COM9 后台启动 60 秒 Runtime 时，PowerShell 在本机报 `Unexpected token`，命令未执行。 |
| 影响范围 | 板端 Runtime 未启动，未修改板端运行状态或测试目录；只延迟联合验证。 |
| 根因结论 | PowerShell 双引号字符串同时包含 Linux `$test_root`、`$!`、嵌套引号和重定向；转义层次不正确，导致本机解析器在串口写入前拒绝脚本。 |
| 无效尝试 | 1. 在双引号字符串中用反斜杠和反引号混合转义 Linux 变量；PowerShell 不按 Bash 规则处理 `\"`。2. 对照测试查询时再次在 `.Write("...")` 内嵌套带单/双引号的 `grep -E`，本机报 `Missing ')' in method call`；板端对照进程已独立启动，不受该查询失败影响。3. 2026-07-19 小时级测试首轮状态查询又在 `.Write("...")` 中内嵌 JSON `grep` 正则，本机再次报 `Missing ')' in method call`，COM9 尚未打开，板端长测不受影响。 |
| 最终方案 | 使用 PowerShell 单引号 here-string 原样保存 Linux 命令，只在末尾追加回车，避免跨两层 shell 手工转义。 |
| 验证结果 | 修正后板端返回 PID `1900`、`1915`、`1929` 和各自唯一 storage root；对应进程均按 60/20 秒配置退出，最终 JSON 为 `state=stopped`。后续日志轮转测试也用同一方式成功启动 PID `1938`。本次复发后改用单引号 here-string，成功读取 PID `2005` 的首分钟状态、CSV 行数和日志轮转文件。 |
| 本轮复现 | 2026-07-20 在完整复电后的 ICM-only 8 秒预检启动前，首次 helper 又用 PowerShell 双引号字符串包裹带远端 `"$root/..."` 的命令，本机报 `Unexpected token`。错误发生在 PowerShell parser，串口对象尚未创建，COM9 和板端均未执行预检。改用 PowerShell 单引号字符串保存远端命令、仅让 BusyBox `sh` 展开 `$?/$root` 后，成功执行唯一一次有效预检并取得目录 `outdoor-agent-health-preflight-gsVcum`。同轮文档校验又把含 Markdown 反引号的 `rg` 模式放进 PowerShell 双引号字符串，触发 `The string is missing the terminator`；该命令同样未接触串口，改为单引号模式后复验。 |
| 剩余风险 | 人工编写新的 `.Write("...")` 仍可能复发；后续串口自动化必须复用 here-string，较长流程应上传板端脚本，最终再封装统一命令执行函数。 |
| 证据等级 | A：PowerShell 原始 ParserError，且 COM9 写入逻辑未开始。 |

面试讲述要点：多层 shell 自动化要明确“哪一层负责变量展开”。这次错误发生在 host parser，不能误判为板端命令、串口或 Runtime 失败。

### TRB-20260719-011: Framebuffer 联合运行时 MCU IMU History 约 57 Hz

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | 新 Runtime 在真实 MP157 上同时启用 GNSS/MCU 双串口、Board IMU、framebuffer dashboard 和 SD history 运行 60 秒后，`mcu_imu.csv` 为 3438 行（含表头），即约 57.3 条状态记录/秒；此前 F407 原始帧链路验收约 100 Hz。 |
| 影响范围 | 当前 CSV 不能作为无损 100 Hz IMU 原始数据集；Runtime 最终状态、四类 MCU seen、ping ACK、GNSS、Board IMU、dashboard 和受控停止均正常。 |
| 同次数据 | magnetometer 1088 条/60 秒约 18.1 Hz，barometer 582 条/60 秒约 9.7 Hz，Board IMU 479 条/60 秒约 8.0 Hz，GNSS 178 条/60 秒约 3.0 Hz。 |
| 已知设计边界 | History Recorder 观察 `McuStatus` 最新快照；`McuSerialService::poll()` 可在一次 read 中解析多个帧，同类早期帧可能在 history callback 前被后续帧覆盖。单线程 framebuffer 整帧渲染也会延长调度周期。 |
| 对照证据 | 旧实现 `--no-dashboard` 20 秒得到 MCU IMU 1300 条，约 65.0 Hz，说明 framebuffer 不是唯一原因；同期 Board IMU 约 9.65 Hz。COM6/USART1 20 秒原始镜像为 IMU 104.2 Hz、磁力计 18.6 Hz、气压计 9.6 Hz，CRC/协议/payload 错误均为 0，排除 F407 当前降频。 |
| 根因结论 | `McuSerialService::poll()` 一次 read 可解析多个合法帧，但 History Recorder 原来只在整轮调度结束后观察一次 `McuStatus`；同一批中的早期 IMU 快照被后续帧覆盖。framebuffer 整帧渲染会进一步拉长轮询周期，所以旧联合测试约 57 Hz、无 dashboard 约 65 Hz。 |
| 最终方案 | 为 GNSS/MCU serial service 增加通用 status-updated callback，在每个合法句子/协议帧应用后立即调用同一个 History Recorder；Runtime 循环回调继续覆盖 Board IMU 和 file/mock 路径。保持单线程，不新增锁、线程或第三方依赖。 |
| 验证结果 | 修复后本机 CTest 8/8、Runtime 脚本和 ARM 构建通过；新 ARM 二进制 SHA256 本地/板端均为 `8b4997053d32ccc364f7b5c06d1bb5a0e591fa22e1bef3bee0a6f36feb0b7f94`。真实 MP157 framebuffer 联合运行 20 秒得到 MCU IMU 1961 条数据约 98.1 Hz、磁力计约 20.1 Hz、气压计 10.0 Hz，最终 state stopped、四类 MCU seen、ping ACK status 0。 |
| 剩余风险 | History 保存成功解析并应用的传感器帧，不是包含噪声/CRC 错误的原始字节抓包；小时级频率、SD 写延迟和掉电行为仍待验证。 |
| 证据等级 | A：真实 SD CSV 行数、60 秒受控运行配置和最终状态文件。 |

面试讲述要点：先区分“上游采样率”“串口解码率”和“快照持久化率”。当前证据只能证明 history snapshot 约 57 Hz，不能据此声称 F407 只输出 57 Hz。

### TRB-20260719-012: 串口预检结束标记被命令回显提前命中

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19 17:18 CST；MP157 板端时钟仍错误显示为 2020-02-09，时间以主机为准 |
| 模块与版本 | PowerShell COM9 板端测试脚本；MP157 Linux 5.4.31 |
| 环境与前提 | COM9 115200 8N1，Linux console 开启输入回显；通过单条 shell 命令执行长测前置检查 |
| 问题现象 | 预检输出只包含命令回显、日期和 `uname`，没有 SD、设备节点和结束标记对应的完整执行结果；主机脚本约 0.8 秒即返回，明显早于 12 秒采集上限。 |
| 影响范围 | 本次预检证据不完整，不能据此判定板端设备缺失或命令执行失败；尚未启动 1 小时长测。 |
| 复现步骤 | 向开启 echo 的 Linux console 发送包含 `echo __PRECHECK_END__` 的命令，并在主机接收缓冲首次包含 `__PRECHECK_END__` 时退出。 |
| 预期结果 | 等待板端执行到命令末尾后打印的结束标记，再返回完整预检结果。 |
| 实际结果 | 结束标记先作为整条输入命令的一部分被 console 回显，主机 `Contains()` 判定提前成立。 |
| 根因结论 | 主机只判断标记是否出现，没有区分“输入回显中的标记”和“命令实际输出的标记”；板端输出中的日期与 `uname` 证明命令已经开始执行，但不能证明后续预检完成。 |
| 最终方案 | 不在输入命令中放置完整结束 token，而是使用 `printf '__BOARD_%s__\n' PRECHECK_DONE` 在板端执行时拼接出 `__BOARD_PRECHECK_DONE__`；接收端仍保留 12 秒超时上限。该方式不依赖 console echo 是否开启，也不受长命令自动换行影响。 |
| 验证结果 | 使用拼接 token 重跑预检，0.8 秒内完整返回日期、`uname`、SD 挂载容量、`/dev/icm20608`、`/dev/ttySTM1`、`/dev/ttySTM2`、`/dev/fb0` 和最终 `__BOARD_PRECHECK_DONE__`；未发现已有 Runtime 进程。后续临时 Runtime 扫描再次把固定 `__SCAN_DONE__` 误命中命令回显；改为 `printf '__SCAN_RC_%s__' "$rc"` 后，接收端只匹配带数字的结果 token，复验 `RUNTIME_SCAN_FOUND=0`。 |
| 本轮复现 | 2026-07-20 启动 ICM-only 热复位烟测时，接收端再次直接搜索完整 `__CODEX_ICM_PROFILE_END__`；console 回显使主机 1.3 秒即返回，而 8 秒预检仍在板端执行。没有重复启动 Runtime；改用两个 `printf` 在执行阶段拼接 `__CODEX_ICM_RESULT_END__` 后，成功读取同一证据目录的完整报告和 JSON。 |
| 剩余风险 | 当前代码仍是临时 PowerShell 片段；如继续扩大板端自动化范围，应把“随机 token + 超时 + 原始输出保存”封装为可复用脚本并增加自身测试。 |
| 证据与关联资料 | 本次 COM9 原始输出显示整条命令回显以及 `Sun 09 Feb 2020...`、`Linux ATK-DLMP157...` 后主机提前返回。 |

面试讲述要点：自动化测试工具本身也需要验证。通过“返回时间异常短、标记出现在输入回显中、板端输出不完整”把故障域限定在主机终止判定，而不是误判为 MP157、SD 卡或外设故障。

### TRB-20260719-013: 超长串口启动命令被截断

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19 17:20 CST；MP157 板端时钟不可信 |
| 模块与版本 | PowerShell COM9 板端测试脚本；BusyBox `sh`；待测 ARM Runtime SHA256 `8b4997053d32ccc364f7b5c06d1bb5a0e591fa22e1bef3bee0a6f36feb0b7f94` |
| 环境与前提 | 通过 Linux serial console 一次发送包含 `mktemp`、约 30 个 Runtime 参数、后台启动和启动后 `if` 检查的超长命令 |
| 问题现象 | console 回显停在 `printf 'HOUR_STARTED` 附近并出现 `>` 续行提示符；后续短命令也被当作未完成复合命令的一部分，1 小时测试没有获得启动确认。 |
| 影响范围 | 长测未启动；没有生成 `/tmp/outdoor_agent_hour_root`，也没有残留 `outdoor_core_runtime` 进程，因此没有污染正式验证结果。 |
| 复现步骤 | 使用单次 `SerialPort.Write()` 发送长启动命令，关闭串口后重新连接，shell 仍处于 `if ... fi` 未完成状态。 |
| 预期结果 | shell 完整解析命令，后台启动 Runtime，打印 PID 和 SD 测试目录。 |
| 实际结果 | 控制台进入 PS2 续行；普通 Ctrl-C 在当前 console 设置下只回显 `^C`，没有退出复合命令；发送 EOF 后 shell 报 `syntax error: unexpected end of file (expecting "fi")` 并恢复主提示符。 |
| 根因结论 | 启动逻辑通过交互串口承载过长的复合 shell 命令，接收边界/行编辑行为不可靠；`unexpected end of file (expecting "fi")` 直接证明板端收到的复合命令不完整。 |
| 最终方案 | 不再通过 console 发送长复合命令；将长测启动逻辑写入仓库中的 POSIX shell 脚本，经 XMODEM 完整上传后只在 console 执行短命令，并对上传文件做 SHA256 校验。异常状态使用 EOF 恢复，随后检查进程和 root 记录文件。 |
| 验证结果 | EOF 后 shell 恢复正常提示符；`ps | grep '[o]utdoor_core_runtime'` 无输出，`/tmp/outdoor_agent_hour_root` 返回 `NO_ROOT`，证明失败会话没有启动 Runtime。随后新增 `run_board_long_validation.sh`，本地/板端 SHA256 均为 `cb1656fe6157e97d4f3e04b6ac3a5925a1ecea46feacc477075c9816068a067e`；短命令启动返回 `LONG_TEST_STARTED pid=2005 root=/run/media/mmcblk1p1/outdoor-agent-hour-20260719-GzcOEG duration_seconds=3600`。 |
| 剩余风险 | XMODEM 上传也可能产生不完整文件，因此正式执行前必须校验本地/板端 SHA256；长测脚本需保持 POSIX `sh` 兼容。 |
| 证据与关联资料 | COM9 原始错误 `-sh: syntax error: unexpected end of file (expecting "fi")`；恢复后进程/root 文件检查。 |

面试讲述要点：不要把“自动化命令发出”当成“测试已启动”。通过 shell PS2、EOF 语法错误和无进程/无元数据三类证据确认失败边界，再把不可重复的交互长命令收敛为版本化、可校验的脚本。

### TRB-20260719-014: 已有 Runtime 保护未生效

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19 17:20 CST；以主机时间为准 |
| 模块与版本 | `run_board_crash_recovery_validation.sh` SHA256 `752036c09cbe4e0c43d36b6c6d69f0e3fad9291efb3cceb07059f7c81f1754d3`；BusyBox `ps` |
| 环境与前提 | PID `2005` 的 3600 秒 Runtime 正在运行；对异常恢复脚本执行 `sh -n` 后，故意调用脚本以验证“已有 Runtime 时返回 2”的保护分支。 |
| 问题现象 | `SYNTAX_RC=0` 后没有立即输出 `another outdoor_core_runtime process is active` 和预期返回码，主机 6 秒采集窗口结束，表明 `ps | grep -q '[o]utdoor_core_runtime'` 很可能没有匹配已有进程，脚本可能继续进入第二个 Runtime 流程。 |
| 影响范围 | 若第二个 Runtime 已启动，会与 PID `2005` 竞争 `/dev/ttySTM1`、`/dev/ttySTM2` 和 `/dev/fb0`，当前小时长测不再满足独占前提，必须作废并重新计时。 |
| 预期结果 | 检测到任意已有 Runtime 后在创建目录和打开设备前退出 2。 |
| 实际结果 | 创建 `/run/media/mmcblk1p1/outdoor-agent-crash-recovery-T43Y4W`，先后启动 PID `2201` 和 `2302`；恢复阶段 GNSS 行数没有增长，MCU 出现 CRC mismatch、`command_ack_seen=false`、`status_flags=4142`，证明两个 Runtime 竞争串口。BusyBox `ps` 只显示进程名 `outdoor_core_ru`。 |
| 根因结论 | BusyBox `ps` 的 CMD 列截断为 `outdoor_core_ru`，因此完整名称 grep 返回未匹配；保护逻辑错误地把“grep 无结果”当作无 Runtime。`/proc/2005/cmdline` 则保留完整可执行文件和参数。 |
| 最终方案 | 长测和异常恢复脚本都遍历 `/proc/[0-9]*/cmdline`，只读取 argv[0] 并按 basename `outdoor_core_runtime*` 匹配；异常恢复验收同时新增 `command_ack_seen=true` 和 `status_flags=425`，避免只检查 ACK status 默认值 0 造成误报。 |
| 验证结果 | 更新后的长测脚本 SHA256 `586f9e5c...60a05`、异常恢复脚本 SHA256 `fc0e09b1...2737`；在 PID `2005` 存活时，两脚本分别返回明确错误并以 3、2 退出，未创建目录。随后 PID `2005` 经 SIGTERM 1 秒内停止；新独占长测 PID `3127` 从全新目录启动。 |
| 处置与证据边界 | 原 PID `2005` 的数据保留但明确作废，不计入小时级结论；竞争测试目录保留为负向证据。 |
| 剩余风险 | 新 PID `3127` 仍需完整 3600 秒验证；`/proc` 枚举只保证当前 Linux 环境，若将来使用容器或不同 PID namespace 需重新验证。 |
| 证据等级 | A：COM9 原始进程扫描、失败报告、JSON 错误字段、修复后退出码及新 PID/root。 |

面试讲述要点：测试前置保护也必须在目标 BusyBox 环境验证，不能依赖桌面 Linux 的 `ps` 输出假设；一旦独占性不再可证，应主动作废耐久数据，而不是继续累计时长后宣称通过。

### TRB-20260719-015: COM6 `SkipReset` 捕获 0 字节

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19，主机时间；板端 RTC 不可信 |
| 模块与版本 | `scripts/verify_f407_uart.ps1` 新增 `-SkipReset` 的首次板端验证；COM6 115200 8N1 |
| 环境与前提 | 干净 MP157 长测 PID `3127` 正在通过 F407 UART4 接收数据；为取得不重启 F407 的累计诊断基线，脚本在打开 COM6 前设置 `DtrEnable=true`、`RtsEnable=true` 并跳过既有 reset pulse。 |
| 问题现象 | 5 秒捕获 `bytes_read=0`、所有帧计数为 0，验证按预期返回失败；未取得诊断基线。 |
| 影响范围 | F407 被重置后 ICM42688 不 ACK，固件退回 Mock IMU；PID `3127` 的小时长测不再是真实 ICM42688 验证，已作废并停止。 |
| 预期结果 | 不复位 F407，在 COM6 接收 USART1 镜像并读取累计 diagnostics。 |
| 实际结果 | 组合一 `DTR=true/RTS=true` 捕获 0 字节；随后 MP157 UART4 显示 F407 uptime 已重置、`status_flags=0x102E`。组合二使用烧录脚本结束时的 `DTR=true/RTS=false` 能捕获 20396 字节/5 秒，但 uptime 仅 1–5 秒，同样证明再次重置；diagnostics 为 I2C recovery 11、failure 8、设备 `0x68`、寄存器 `0x76`、init error step 1，heartbeat 持续 `0x102E`。 |
| 根因结论 | 在当前一键下载硬件和 .NET `SerialPort` 上，打开 COM6 会应用 DTR/RTS 并扰动 F407 复位/启动控制；仅在脚本中跳过显式 `Invoke-AppReset` 不能实现物理“无复位打开”。复位后 ICM42688 未 ACK，触发既有 Mock fallback。 |
| 最终处置 | 撤回未验证的 `-SkipReset` 参数，恢复原验证脚本行为；PID `3127` 经 SIGTERM 1 秒内停止，最终状态保留 `0x102E` 作为作废证据。后续小时测试期间不再打开 COM6。 |
| 验证结果 | MP157 UART4 在两次 COM6 尝试后仍收到约 95 Hz Mock 帧，证明不是整机失联；COM6 第二次捕获 CRC/version/payload 均为 0，但 ready flags 不健康并明确失败。后续未打开 COM6 的 8 秒健康探针 PID `2791` 正常停止但仍为 `4142 (0x102E)`。再次恢复目标后执行两次独立 8 秒健康预检均得到 `425 (0x01A9)`，四类 MCU 数据、ping ACK、GNSS NMEA、Board IMU 和错误字段全部通过；随后 60 秒联合冒烟审计通过，证明真实传感器状态已恢复。恢复发生在两轮任务之间，未取得人工供电动作的直接记录，因此只将 `0x01A9` 状态视为恢复证据，不反推具体操作。 |
| 剩余风险 | 打开 COM6 仍会扰动控制线；小时长测期间必须保持 COM6 关闭。当前恢复已满足继续测试的门槛，但仍需小时级观察传感器是否再次降级。 |
| 证据等级 | A：两次完整 PowerShell 输出、MP157 JSON、F407 diagnostics 和 PID `3127` 受控停止结果。 |

面试讲述要点：新增“跳过软件 reset”参数不等于硬件上真的无扰动；串口适配器的 DTR/RTS 是物理控制信号，必须通过另一条独立链路和真值表验证，而不是仅凭代码分支命名判断。

### TRB-20260719-016: `systemd-modules-load` 启动失败

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19 MP157 软重启，主机时间为准 |
| 模块与版本 | OpenSTLinux 5.4.31；`/etc/modules-load.d/outdoor-agent.conf` 内容为 `icm20608` |
| 环境与前提 | 配置文件此前已通过 `modprobe -r icm20608` 后手工重启 `systemd-modules-load.service` 验证，模块能 probe ID `0xAE` 并创建设备节点。此次执行完整 `sync; reboot` 验证真实 boot 顺序。 |
| 问题现象 | boot 约 8.7 秒显示 `systemd-modules-load.service: Main process exited, status=1/FAILURE` 和 `Failed to start Load Kernel Modules`；随后约 9.1 秒才启动板厂 `Link Linux Kernel Modules`。 |
| 影响范围 | `/etc/modules-load.d` 配置不能被视为已完成开机自动加载；需要确认模块查找路径、板厂链接服务和 SPI probe 时序，不能勾选软重启验收。 |
| 预期结果 | boot 中 `systemd-modules-load` 成功，`lsmod` 包含 `icm20608`，`/dev/icm20608` 自动出现。 |
| 根因结论 | `/usr/bin/link-modules.sh` 在 `link-modules.service` 中先 sleep 1 秒，再创建 `/lib/modules/$(uname -r) -> /boot/$(uname -r)`；标准 `systemd-modules-load` 在该链接出现前执行，因此早期 `modprobe icm20608` 失败。重启完成后 `modprobe -n -v` 又能解析到 `/boot/.../icm20608.ko`，与时序根因一致。 |
| 最终方案 | 删除 `/etc/modules-load.d/outdoor-agent.conf`，新增并 enable `outdoor-agent-icm20608.service`：`Requires/After=link-modules.service`，oneshot 依次执行 `/sbin/modprobe icm20608` 和 `/usr/bin/test -c /dev/icm20608`，`RemainAfterExit=yes`。 |
| 验证结果 | unit 本地/板端 SHA256 均为 `1124e176...123c3`；运行态 start 的两个 ExecStart 均为 0。第二次 `sync; reboot` 后，标准 modules-load 为 `Result=success/ExecMainStatus=0`，自定义服务为 `active (exited)/enabled/Result=success`；内核 13.466 秒输出 ID `0xAE`，`lsmod` 和字符设备 `240:0` 均存在。 |
| 剩余风险 | unit 依赖板厂专有 `link-modules.service`；迁移到其他镜像时需改用其模块安装布局或发行版原生 depmod 流程。 |
| 证据等级 | A：两次完整 boot 日志、unit/result 状态、模块/设备节点和 dmesg probe 证据。 |

面试讲述要点：手工重启服务成功不能替代真实 boot 验证。通过启动时序发现板厂镜像的模块路径准备晚于标准 modules-load，需要把依赖关系写进 systemd unit，而不是把偶然成功当成开机可靠。

### TRB-20260719-017: `systemctl` 串口分页阻塞

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | 在 COM9 交互 TTY 上串联执行 `systemctl cat/status/show` 时，systemd 自动调用 pager；输出停在 `standard input`，后续命令被 less 当成搜索/文件名输入。 |
| 影响范围 | 只延迟诊断并造成终端显示混乱，没有修改服务状态；ICM20608 unit 安装和重启结果不受影响。 |
| 根因结论 | `systemctl` 检测到 TTY 后使用默认 pager，单条串口命令中没有禁用分页；不能假设非交互脚本环境的直接输出行为。 |
| 无效尝试 | 直接连续发送多个 `q` 时处于 less 的 `Examine:` 子提示，字符被当作输入而不是退出命令；随后发送普通 shell 文本也被 pager 吞掉。 |
| 最终方案 | 先发送 ESC 退出子提示、`q` 退出 pager，必要时 Ctrl-C 回到 shell；后续所有板端 systemctl 诊断统一使用 `SYSTEMD_PAGER=cat systemctl --no-pager ...`。 |
| 验证结果 | 恢复 shell 后带 `SYSTEMD_PAGER=cat` 的两条 `systemctl show` 连续完整返回，标准 modules-load 和自定义服务的 Result/ExecMainStatus 均为 success/0。 |
| 剩余风险 | 人工新增命令时仍可能遗漏环境变量；后续可在统一串口执行函数中默认注入 `SYSTEMD_PAGER=cat`。 |
| 证据等级 | A：COM9 pager 控制序列、恢复提示符和无分页重试输出。 |

面试讲述要点：自动化诊断不仅要处理命令退出码，还要控制交互环境。TTY pager 会改变输入消费者，必须先恢复终端状态，再用显式无分页配置取得可重复输出。

### TRB-20260719-018: GNSS `last_error` 误判

| 字段 | 记录 |
| --- | --- |
| 状态 | 已修复待室外正向验证 |
| 问题现象 | 新增 GNSS fix 脚本在室内 5 秒负向测试中正确识别 `fix_valid=false`，但全局扫描 JSON 时同时报告 `status non_empty_last_error=true`。 |
| 环境与证据 | 真实 UBLOX-M10 `/dev/ttySTM2` 38400 8N1；5 秒生成 45 行 GNSS CSV，Runtime 受控退出，fix/RMC/GGA 三个定位门槛均失败。GNSS service 会把无效 RMC 或未解析句子的原因写入 `gnss.last_error`，后续合法句子才清空。 |
| 影响范围 | 如果室外已经获得有效 fix，但停止前最后一个句子恰好被跳过，全局 `last_error` 仍可能让验收误报失败；反之不能因此放松 Runtime/storage/MCU 错误门槛。 |
| 根因结论 | `gnss.last_error` 表示最后一次句子解析状态，不等同于 Runtime 致命错误；状态 JSON 中多个 section 使用同名字段，全局 grep 丢失了错误所属模块。 |
| 最终方案 | 使用 section-aware AWK，仅将 `runtime`、`storage` 和 `mcu` 的非空 `last_error` 视为核心失败；GNSS 成功仍必须同时满足 status `fix_valid=true`、CSV 中有效 RMC 非零坐标、有效 GGA 卫星数/质量和完整末行。 |
| 验证结果 | 使用合法 MCU fixture 重跑室内 5 秒负向测试：Runtime/storage/MCU 三个 section 均报告 `last_error_empty=true`；脚本仅因 fix status、RMC、GGA 四个定位门槛失败，以 1 退出并保存 45 行完整 GNSS CSV。section-aware 修复已验证。 |
| 剩余风险 | 室外真实 fix 尚未取得；只有正向测试才能关闭该问题并确认 GGA/RMC 组合门槛。 |
| 证据等级 | A：真实板端报告、状态 JSON 语义和 `GnssSerialService::consumeLine()` 代码路径。 |

面试讲述要点：同名错误字段必须结合模块语义判断。解析器的“最近一次跳过”不是进程故障，验收既不能误报，也不能为了通过而删除核心错误检查，因此采用 section-aware 门槛并保留多句型定位证据。

### TRB-20260719-019: GNSS 验收复用负向 MCU fixture

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 问题现象 | section-aware 修复后再次执行 5 秒 GNSS 负向测试，Runtime/storage error 均为空，但 MCU section 报 `MCU frame CRC mismatch: expected 0x0, calculated 0xc152`。 |
| 关键证据 | 最终 JSON 显示 heartbeat/IMU 均已 seen、最后帧为 `sensor_imu`，随后保留 CRC mismatch；`data/mcu_mock_frames.txt` 末尾明确标注并包含一条“used by verification to prove CRC rejection”的错误 heartbeat。 |
| 根因结论 | GNSS 脚本为满足 Runtime 必须存在的 MCU service，复用了通用回归 fixture；该 fixture 同时承担 CRC 拒绝负向测试，因此必然污染 MCU final error，不适合正向 GNSS 验收。 |
| 方案取舍 | 不删除原错误帧，否则会降低既有 Runtime 回归覆盖；也不忽略 MCU error，否则可能掩盖真正故障。新增只含 heartbeat/mock sensor/mock IMU 合法帧的 `mcu_mock_frames_valid.txt`，GNSS 脚本改用该专用 fixture。 |
| 验证结果 | 新 fixture 本地/板端 SHA256 均为 `991d4eaa...f1af4`；GNSS 脚本 SHA256 均为 `2f7e09b9...5e713` 且 `sh -n` 返回 0。5 秒真实 M10 负向测试中 MCU section error 为空，脚本只因 fix/status/RMC/GGA 四项定位证据缺失返回 1，符合预期。 |
| 剩余风险 | fixture 复用问题已闭环；室外正向 fix 是 Stage 1 独立验收项。 |
| 证据等级 | A：真实 JSON、日志、fixture 原文和确定性代码路径。 |

面试讲述要点：测试数据也有适用范围。负向 fixture 不能直接当作另一个功能的健康依赖；正确做法是保留原错误覆盖，再提供语义单一的合法 fixture，使验收失败能准确归因。

### TRB-20260719-020: 正式长测缺少真实传感器健康预检

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19，恢复目标后的完成度审计 |
| 模块与版本 | `run_board_long_validation.sh`、`audit_board_long_validation.sh`、`run_board_crash_recovery_validation.sh`；ARM Runtime SHA256 `8b499705...b7f94` |
| 环境与前提 | F407 在 COM6 控制线复位后保持 `0x102E` Mock fallback；真实健康值为 `425 (0x01A9)`。 |
| 问题现象 | 长测启动脚本只检查设备节点和进程独占，`status_flags=425` 仅由结束审计检查；异常恢复脚本也会先执行 SIGKILL，再在恢复末尾发现健康位不符。 |
| 影响范围 | F407 降级时仍可能启动一小时任务，直到结束才判失败；虽然审计不会误报通过，但会浪费时间、增加 SD 写入并产生需人工作废的数据。 |
| 复现与证据 | 对三个脚本进行代码路径审计：启动脚本在 `nohup` 前没有短时 Runtime 状态检查；结束审计明确检查 `status_flags=425`；已有 PID `3127` 的作废经历证明启动后发生/保留 Mock 会使整段耐久证据失效。 |
| 根因结论 | 验收门槛放置过晚。设备节点存在只证明 Linux 接口存在，不能证明 F407 三路真实传感器、双向命令链路和板载 IMU在本次测试起点健康。 |
| 最终方案 | 新增 `run_board_health_preflight.sh`：独占执行 8 秒真实双串口/Board IMU Runtime，保存独立 status、日志和报告；要求受控停止、GNSS seen、四类 MCU seen、ping ACK status 0、F407 `status_flags=425`、Board IMU seen 以及 Runtime/storage/MCU/Board IMU error 为空。小时长测和异常恢复脚本只有预检成功才创建正式证据目录。 |
| 验证结果 | 本地/板端 shell 语法和部署 SHA256 门槛通过；真实板多次以 `0x01A9` 正向放行。2026-07-19 再次烧录后 ICM42688 不 ACK，预检保存 `status_flags=4142 (0x102E)`、init step 1 和 52 次最终 I2C failure，并仅因真实状态门槛失败；`run_board_long_validation.sh` 返回 4，未创建/启动正式 30 秒目录，直接证明负向拒绝分支。 |
| 剩余风险 | 8 秒门槛只能确认启动基线，不能替代小时内健康时间线和最终审计。 |
| 证据等级 | A：真实板正向放行、`0x102E` 负向拒绝、独立预检目录和脚本退出码均有直接证据。 |

面试讲述要点：结束审计严格并不代表测试启动成本可忽略。把最关键的健康不变量前移为独立短时门槛，既避免无效耐久测试，也保留预检与正式数据两套可追溯证据。

### TRB-20260719-021: 最终快照掩盖运行期 FIFO fallback

| 字段 | 记录 |
| --- | --- |
| 状态 | 已修复待板端长测验证 |
| 首次发现时间 | 2026-07-19；3600 秒正式长测 PID `4868` 启动后约两分钟 |
| 模块与版本 | F407 FIFO/TX queue 固件；ARM Runtime SHA256 `8b499705...b7f94`；长测目录 `/run/media/mmcblk1p1/outdoor-agent-hour-final-o7GBsX` |
| 环境与前提 | 两次独立 8 秒预检和一次 60 秒联合冒烟均为 `425 (0x01A9)`；COM6 始终保持关闭。 |
| 问题现象 | PID `4868` 仍运行、五类 CSV 持续增长且所有 Runtime `last_error` 为空，但现场读取 `status_flags=558 (0x022E)`；随后 10 秒连续观察首样本为 `942 (0x03AE)`，其余恢复为 `425`。 |
| 影响范围 | ICM42688 短暂进入 FIFO error + Mock fallback；当前只保存最终 JSON 的审计可能在恢复后误认为整段时间始终健康。 |
| 状态位解码 | `0x022E/0x03AE` 均含 fallback、IMU error 和 FIFO error；MMC5603/BMP390 仍 ready。`0x03AE` 还保留 INT1/FIFO active，随后自动回到完整健康 `0x01A9`。未出现 TX/RX overflow、init error 或 I2C 最终失败位。 |
| 处置 | 不把自恢复后的最终 `0x01A9` 当作全程通过。PID `4868` 经 SIGTERM 1 秒停止，目录写入 `health_incident.txt` 并标为 `ABORTED_MISSING_HEALTH_TIMELINE`，不计入小时级结论。 |
| 根因结论 | 验收漏洞的根因已确认：只检查最终快照，没有保存状态时间序列。后续 UART4 diagnostics 已确认底层同时存在 FIFO empty-header 解析放大和共享 I2C2 事务风暴；具体两轮证据、修复与未闭环项见 TRB-024。 |
| 最终方案 | 新增 `monitor_board_runtime_health.sh` 逐秒保存健康时间线；长测自动启动 monitor 并记录 PID/完成标记。审计增加样本覆盖、致命位、核心 error、99% 健康占比、最长 5 秒连续降级和最终 `0x01A9` 门槛。按 ADR-0020 将同一 44 字节 diagnostics 帧同时发送到 UART4/MP157，审计累计计数的单调性和运行期增量。 |
| 验证结果 | 20 秒旧时间线正确拒绝降级；UART4 diagnostics 新版预检确认 seen/字段解码，30 秒目录 `diag30-9B4FM2` 在逐秒 flags 全健康时仍以 recovery 204、failure 2、overflow 10、malformed 8、skipped 9281 返回失败，证明累计门槛能捕获亚秒级异常。 |
| 剩余风险 | 1 秒 flags 会漏掉亚秒级瞬态，因此累计 diagnostics 仍是必需门槛；TRB-024 的 provider 修复待完整复电后板端验证，小时结论仍未形成。 |
| 证据等级 | A：状态序列、累计 diagnostics、CSV、受控停止、作废文件和多次失败审计均为真实板直接证据。 |

面试讲述要点：最终状态健康不能证明运行过程健康。发现可自恢复瞬态后，既不简单忽略，也不把任何瞬态都等同永久失败，而是记录持续时间、比例和致命位，把恢复能力变成可量化的验收指标。

### TRB-20260719-022: CTest 配置与 MSVC 断言弹窗阻塞

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；新增 diagnostics parser 后的本机验证 |
| 模块与版本 | Windows CMake Visual Studio 多配置生成器；CTest；MSVC Debug CRT；`mcu_protocol_tests` |
| 问题现象 | 第一次 F407 `ctest --test-dir build` 的 5 个测试全部显示 `Not Run`；更正后并行验证在 124 秒超时。拆分执行时 MP157 CTest 长时间不返回，进程窗口标题为 `Microsoft Visual C++ Runtime Library`。 |
| 影响范围 | 自动化无法给出可信退出码；F407 首次结果不是测试失败而是未执行，MP157 diagnostics 测试的真实断言被 GUI 弹窗遮蔽。 |
| 根因结论 | F407/MP157 使用 Visual Studio 多配置构建，CTest 必须指定 `-C Debug`。修正后，新增 diagnostics fixture 实际只有 43 字节，少了协议第 44 字节 reserved，触发 MSVC `assert`；Debug CRT 默认用 modal abort 对话框，非交互工具一直等待。并行总超时又留下失去父进程的 CTest/测试进程。 |
| 无效尝试 | 只设置 `_CrtSetReportMode(...stderr)` 后断言文本已转 stderr，但 `abort()` 仍弹独立窗口，第二次 30 秒测试继续超时。 |
| 最终方案 | 精确终止已确认失去父进程的 CTest/测试 PID；测试入口同时设置 `_CrtSetReportMode` 和 `_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT)`，使断言在无 GUI 环境直接返回；补齐 reserved 字节；所有 Visual Studio CTest 命令显式使用 `-C Debug`，三类构建拆分取得独立返回码。 |
| 验证结果 | 具体失败明确输出为 `payload.size() == ...kSensorHubDiagnosticsPayloadSize` 第 79 行；修复后 `mcu_protocol_tests` 直接运行返回 0，MP157 CTest 8/8 和 F407 CTest 5/5 均在 1 秒内通过，Runtime 验证脚本和 ARMv7 构建通过。 |
| 本轮复现 | 撤回 FIFO 实验后的首轮 F407 验证再次遗漏 `-C Debug`，7 个测试均显示 `Not Run`、总耗时 0.02 秒；立即按既有方案改为 `ctest --test-dir build -C Debug --output-on-failure`，7/7 在 0.48 秒内实际执行通过。该结果继续归入本条，不另建重复问题。 |
| 剩余风险 | 其他使用裸 `assert` 的 MSVC 测试若以后失败，仍可能触发 modal；可后续统一测试断言封装，但本轮不扩大无关重构。 |
| 证据等级 | A：CTest 原始输出、进程命令行/窗口文本、明确断言行和修复后完整返回码。 |

面试讲述要点：自动化“超时”可能同时包含命令用法错误、测试真实失败和交互弹窗三层问题。先区分 Not Run 与 Failed，再读取进程/窗口证据，最后让测试在 CI 式无 GUI 环境可观察失败，而不是盲目增加超时。

### TRB-20260719-023: Git Bash 未加入 PowerShell PATH

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；新增长测脚本最终静态检查 |
| 模块与版本 | Windows PowerShell；Git for Windows `C:\Program Files\Git\bin\bash.exe` |
| 问题现象 | 直接执行 `bash -n ...` 返回 `bash : The term 'bash' is not recognized`；同一复合命令后续仍继续运行，若只看最终退出码会误以为 shell 检查通过。 |
| 影响范围 | 首次命令实际没有执行六份 POSIX shell 脚本的语法检查；代码和板端状态未受影响。 |
| 根因结论 | Git Bash 已安装，但其目录不在当前 PowerShell `PATH`；此外 PowerShell 默认不会因中间 native/command-not-found 错误自动终止整个命令序列。 |
| 无效尝试 | 直接使用未解析的 `bash` 命令，并在同一调用中继续执行 `git diff --check` 和哈希读取；调用最终返回 0，不能作为 shell 检查证据。 |
| 最终方案 | 先用 `Test-Path` 确认 Git Bash 的绝对路径，再调用 `C:\Program Files\Git\bin\bash.exe -n ...`；分别保存 Bash、`git diff --check` 和 PowerShell parser 的返回值，任一非零时显式失败。 |
| 验证结果 | 首次闭环时六份 shell 脚本语法检查 `exit=0`，`git diff --check exit=0`，仓库 `scripts/*.ps1` parser errors 为 0。2026-07-19 最终全仓回归中该环境条件再次出现；绝对路径存在性检查确认 Git/MSYS2 Bash 均已安装，改用 `C:\Program Files\Git\bin\bash.exe` 后，当前仓库 8 份 `.sh` 全部通过 `bash -n`。 |
| 剩余风险 | 后续新增脚本必须通过动态文件清单或显式清单纳入检查；Windows 自动化不应假设 Git Bash 已进入 PATH。 |
| 证据等级 | A：原始 command-not-found 输出、绝对路径存在性检查和带独立返回值的复验输出。 |

面试讲述要点：一条复合验证命令的最终退出码不一定代表所有子步骤都执行成功。对关键门槛分别保存返回值，并显式解析工具路径，能避免“验证脚本自己漏检”的二次风险。

### TRB-20260719-024: 健康 heartbeat 掩盖累计 FIFO/I2C 异常

| 字段 | 记录 |
| --- | --- |
| 状态 | 分析中 |
| 首次发现时间 | 2026-07-19；UART4 diagnostics 镜像首次 30 秒联合测试 |
| 模块与版本 | 首轮 F407 19396 B diagnostics 镜像 SHA256 `660fedee...5723`；MMC/provider 修复基线 19356 B、SHA256 `d220cbc7...ad56`；当前 schema 1 镜像 19376 B、SHA256 `365fb9c3...129c`；首轮证据使用的 MP157 Runtime SHA256 `5e8e197c...56f7`，当前为 `47c4bcc9...b739`；证据目录 `/run/media/mmcblk1p1/outdoor-agent-diag30-9B4FM2` |
| 环境与前提 | 新版 8 秒预检通过，F407 `0x01A9`、四类数据、diagnostics、ping ACK、GNSS 和 Board IMU 均 seen；COM6 在测试期间保持关闭。 |
| 问题现象 | 29 个逐秒样本中 28 个可评估样本全部为 `0x01A9`，但 30 秒累计增量为 I2C recovery 204、最终 failure 2、FIFO overflow 10、malformed 8、empty 31、skipped 9281；审计唯一失败项为 diagnostics 增量。 |
| 影响范围 | 传感器输出频率和最终状态看似正常，但底层持续发生恢复、FIFO flush 和数据跳过；若只看 heartbeat/CSV，会把真实数据质量风险误判为稳定。 |
| 根因证据 | 每秒上下文反复落在 ICM42688 `0x69` 的 `FIFO_COUNTH(0x2E)`、`FIFO_DATA(0x30)` 和当时仍读取的 `INT_STATUS(0x2D)`，也包括 MMC5603 `0x30`；HAL status 为 ERROR/TIMEOUT。TDK 定义 `HEADER_MSG=1` 表示 FIFO 已空，而旧 parser 遇到后继续逐字节扫描，直接放大 skipped。旧 byte-count 模式还允许读取非整包快照，且 100 kHz 下 256 字节上限接近 30 ms timeout。 |
| 无效结论 | “所有逐秒 heartbeat 都是 `0x01A9`，因此测试健康”被累计 diagnostics 直接否定；“提高健康采样频率即可解决”也不能替代单调累计计数。 |
| 第一轮修复与失败证据 | ICM42688 改为 record-count/4-record watermark，empty header 终止 parser，去掉运行期 `INT_STATUS`，I2C2 提升到 400 kHz，并将单次失败与 fallback 解耦。相同 30 秒 `diag30b-SosYV3` 中 skipped 降至 290，但 recovery 179、failure 2、overflow 28，且 2 个逐秒样本为 `937 (0x03A9)`；说明 parser 修正有效，但 I2C 事务风暴根因仍在。 |
| 根因补充 | 新版最近失败横跨 ICM42688 `0x69`、MMC5603 `0x30` 和 BMP390 `0x76`。代码审查发现 MMC5603 每 50 ms 启动一次 6.6 ms 单次测量后，在最长 20 ms 内无任何 delay 紧循环读取 `STATUS1(0x18)`；400 kHz 使单位时间轮询更多，符合 overflow 反增和跨设备竞争现象。 |
| 第二轮方案与主机验证 | 保留 record-count、empty-marker 和 400 kHz；按 MEMSIC `BW=00` 的 6.6 ms 时序，Take Measurement 后等待 7 ms，再最多读取 3 次状态、每次间隔 1 ms；新增 MMC provider 测试，确认正常路径只读一次状态。F407 CTest 7/7、ARM 镜像 19376 B、SHA256 `365fb9c3...129c`、刷写和回读均通过。方案比较见 ADR-0021。 |
| 完整复电复验失败证据 | 完整复电后的首个正式预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-rjgpa6` 以 `0x01A9` 全部通过，但其 F407 uptime `104078 ms` 时累计 recovery 234、failure 0、overflow 56、malformed 19、empty 7、skipped 713，说明最终健康已经掩盖运行期错误。约 73 秒后，长测入口自带的第二次预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-Hq1vCJ` 以 `937 (0x03A9)` 拒绝启动；同期累计值变为 recovery 424、failure 2、overflow 114、malformed 34、empty 23、skipped 1244，即增量分别为 `+190/+2/+58/+15/+16/+531`。最近失败固定为 ICM42688 `0x69`、`FIFO_COUNTH(0x2E)`、2 字节 read、HAL status 1/error 4，init step 0、UART4 RX drop 0。该 A 级证据否定“MMC 有界轮询已解决运行期根因”的结论。 |
| 100 kHz 对照失败 | 仅把 I2C2 从 400 kHz 降到 100 kHz，主机 7/7 和 ARM 构建通过；19376 B 镜像 SHA256 `cf5ce837...8148` 刷写回读后，预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-oQ03bR` 在 F407 uptime `22064 ms` 已变为 `0x03A9`，recovery 59、failure 1、overflow 3、malformed 7、empty 2、skipped 84，最近失败仍为 `0x69/FIFO_COUNTH(0x2E)` read。降速没有改善，已恢复源码和 `.ioc` 为 400 kHz。 |
| 固定窗口试验失败 | 根据 TDK 的 `FIFO_RESUME_PARTIAL_RD` 语义，先后试验不读 count、固定读取 8 records/128 B 和 4 records/64 B。128 B 镜像 19300 B、SHA256 `2f857e2b...d2c`：首个预检 `SZcM24` 虽通过，但 uptime `33040 ms` 已有 recovery 49、malformed 51、empty 88；约 50 秒后的 `E4V8AJ` 变为 `0x03A9`，recovery/malformed 增至 139/134。64 B 镜像 19300 B、SHA256 `b6a623b8...fbdf` 的 `cvgvps` 预检虽通过，但 uptime `27032 ms` 已有 recovery 45、malformed 51、empty 154。两种固定过读都比精确 record-count 读取产生更多 malformed/empty，已全部撤回。 |
| 临时原始包头证据 | 只在 parse 失败时临时把 64 B 读取中偏移 0/16/32/48 的首字节打包到既有诊断字段。19328 B 临时镜像 SHA256 `cc6051a3...142` 的预检 `OCKFth` 在 uptime `24142 ms` 得到 packed value `0xFFFFFF06`，即四个候选包头为 `06 FF FF FF`，而当前 16 字节 accel+gyro 包应为 `0x68`、empty marker 应为 `0x80`。这证明一次 HAL 成功返回的数据流已经错位或损坏，但尚不能区分 partial-read 中断后的 FIFO 指针状态与物理总线数据完整性。临时代码随后移除。 |
| 实验撤回与正式镜像恢复 | 撤回 100 kHz、fixed partial read、临时包头复用和相应测试，恢复 400 kHz、先读 record count 再精确读取完整 16 B records 的实现。F407 主机 CTest 7/7、ARM clean build 通过；当前 BIN 19384 B、SHA256 `f0addc29...b9d1e`，COM6 ROM Bootloader 烧录和逐字节回读通过。该刷写会热复位 F407，不能代替新的完整复电验证。 |
| 首次物理隔离复验无效 | 用户报告已只保留 ICM42688 并完整复电后，正式固件 8 秒预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-9D6RBq` 得到 `status_flags=21046 (0x5236)`、ICM init step 1、recovery 3377、最终 failure 3264、overflow/malformed/empty/skipped 84/14/14/458。最近失败为 `0x30/STATUS1(0x18)`，状态同时为 `barometer_seen=true`、`magnetometer_seen=false`。这些直接数据证明本次运行仍存在非 ICM 事务/帧，无法确认实际电气隔离条件；同时也证明正式三传感器固件不适合作为缺从机器件下的 ICM-only 测试载荷。该预检按设计失败，不能归因给 ICM 本体。 |
| 可重复隔离方案 | 固件增加默认 `OFF` 的 `SENSOR_HUB_ICM42688_ONLY_DIAGNOSTIC` CMake 选项；启用后不初始化或轮询 MMC5603/BMP390，并在 heartbeat 设置 bit 15 `0x8000`，防止诊断镜像误作生产验收。预检增加第五参数 profile，默认 `full` 保持原门槛；`icm42688_only` 要求状态 `0x8181`、磁力计/气压计 absent，以及 I2C/FIFO/init/drop 累计值全部为零。 |
| 隔离镜像与部署 | ICM-only Release BIN 15072 B、text 15052 B、data 20 B、BSS 4252 B，SHA256 `c07e235a...2f80a`；正式默认构建仍为 19384 B、SHA256 `f0addc29...b9d1e`。F407 CTest 7/7、两种 ARM 构建通过。诊断镜像经 COM6 ROM Bootloader 烧录和逐字节回读成功；MP157 profile 脚本 SHA256 `73632276...12dbf` 经 XMODEM、板端哈希、权限和 `sh -n` 验证部署。COM6 刷写属于热复位，尚未产生新的 ICM-only 复电结论。 |
| 热复位 profile 烟测 | 预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-eGaj29` 证明 profile 生效：磁力计/气压计均 absent，FIFO overflow/malformed/empty/drain/skipped 全零，diagnostics schema 1/drop 0；heartbeat 为 `53254 (0xD006)`，其中 `0x8000` 明确来自隔离镜像。ICM 仍在热复位后的 init step 1，recovery/failure 均为 898，最后失败为备用地址 `0x68/REG_BANK_SEL(0x76)` 写 NACK；profile 以 5 项失败拒绝。该结果验证了隔离边界和门槛脚本，也再次证明必须完整复电，不能作为 ICM 稳定性结论。 |
| ICM-only 完整复电结果 | 用户确认诊断镜像与 ICM42688 已完整断电重上电后，COM9 先确认无残留 Runtime，再执行唯一一次 8 秒 profile。证据目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-gsVcum`：磁力计/气压计均 absent，heartbeat 精确为 `33153 (0x8181)`、真实 IMU seen、init step 0、diagnostics schema 1、UART4 RX drop 0，证明完整复电恢复了 ICM 初始化且软件隔离边界生效。但 F407 uptime `229523 ms` 时已累计 recovery 660、最终 transaction failure 105、FIFO overflow/malformed/empty/drain/skipped `707/126/39/0/2933`；最后失败为 ICM `0x69/SIGNAL_PATH_RESET(0x4B)` 单字节 write、HAL status `3 (TIMEOUT)`、error `32 (0x20, HAL_I2C_ERROR_TIMEOUT)`。profile 以 7 项零累计门槛失败拒绝；因此 30/60 秒未启动。 |
| 验证前提补正 | 上述结果形成后，用户补充确认该轮完整复电和预检期间 COM6 实际未关闭，现已关闭。COM6 控制线可能触发 F407 应用热复位，因此违反“COM6 全程关闭”的隔离验收前提；`gsVcum` 的原始异常数据继续保留，但不得作为干净完整复电结论。diagnostics uptime `229523 ms` 说明采集快照前约 229 秒内没有应用复位，但无法证明上电阶段或预检前控制线没有切换。必须在 COM6 关闭状态下重新让 F407 与 ICM42688 完整掉电上电，再从零执行 8 秒 profile。 |
| COM6 关闭后的干净复验 | 用户保持 COM6 关闭，再次让 F407 与 ICM42688 完整断电重上电。COM9 确认无残留 Runtime 后执行唯一一次 8 秒 profile，证据目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-XMWvGC`。heartbeat 仍精确为 `33153 (0x8181)`、真实 IMU seen、磁力计/气压计 absent、init step 0、schema 1、UART4 RX drop 0；但 diagnostics uptime `58298 ms` 时已累计 recovery/failure `232/45`，FIFO overflow/malformed/empty/drain/skipped `186/21/20/0/671`，profile 再次以 7 项失败拒绝，30/60 秒未启动。最后失败为 ICM `0x69/FIFO_DATA(0x30)`、112 B read、HAL status `1 (HAL_ERROR)`、error `4 (0x04, HAL_I2C_ERROR_AF/ACK failure)`。 |
| COM6 A/B 结论 | 干净轮次 recovery/failure/overflow/skipped 约为 `3.98/0.77/3.19/11.51` 次每秒；受污染轮次约为 `2.88/0.46/3.08/12.78` 次每秒。两轮持续时间不同，不能做严格性能比较，但错误量级一致且干净轮次没有消失，足以否定“COM6 未关闭是本次 I2C/FIFO 累计异常的必要条件”。COM6 仍必须在后续正式验证中关闭。 |
| 单器件 100 kHz A/B | 在没有万用表的条件下，仅把 I2C2 从 400 kHz 改为 100 kHz，保持 ICM-only 镜像、接线、profile 和约 58 秒启动窗口不变。完整复电后的证据目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-wB2xhs`：heartbeat `53766 (0xD206)`、init step 1，recovery/failure `759/164`，FIFO overflow/malformed/empty/drain/skipped `58/22/12/0/309`；最后失败为备用地址 `0x68/REG_BANK_SEL(0x76)` 单字节 write、HAL error `0x04 (ACK failure)`。profile 以 9 项失败拒绝，30/60 秒未启动。 |
| 100/400 kHz 对比结论 | 400 kHz `XMWvGC` 与 100 kHz `wB2xhs` 的 diagnostics uptime 分别为 `58.298/57.920 s`，窗口高度接近。recovery 速率从 `3.980/s` 增至 `13.104/s`（`3.29×`），最终 failure 从 `0.772/s` 增至 `2.831/s`（`3.67×`），且状态从 `0x8181/init step 0` 退化为 `0xD206/init step 1`。overflow/skipped 降低来自真实 FIFO 工作中断和 fallback，不能视为改善。该 A/B 否定“单纯 400 kHz 过快”假设，100 kHz 方案已判失败并完成回滚。 |
| 400 kHz 基线恢复 | 回滚命令先因 COM6 不存在而在开口前失败，没有擦写 Flash。用户确认 F407 USB-UART 重枚举为 COM3 后，只读检查确认 COM3/COM9 均为 CH340 且状态 OK，无实际串口终端占用；通过 COM3 取得 Bootloader `0x31`、chip ID `0x0413`，对 15072 B、SHA256 `c07e235a...2f80a` 的 400 kHz ICM-only 基线完成全擦、写入、逐字节回读和 GO。当前板上镜像可信，但此次烧录是应用热复位，不构成新的传感器稳定性结论。 |
| 当前分析方向 | 编译期隔离、COM6 关闭复验与同窗口 100/400 kHz A/B，已经排除其他两颗从机访问、COM6 打开和单纯 400 kHz 过快作为异常成立的必要条件。根因边界收敛到 ICM42688 单器件的供电/共地、PB10/PB11 接触、有效上拉/边沿、模块本体，以及 FIFO 长读与恢复交互；没有万用表或波形工具时，继续修改读法只会增加软件变量。400 kHz 隔离基线已恢复，下一步等待电气测量工具。 |
| 剩余风险 | 当前缺少 VCC/AD0、SCL/SDA 空闲电平、有效上拉阻值、边沿和 ACK failure/TIMEOUT 波形。100 kHz A/B 失败只能否定单一降速方案，不能区分布线、上拉、供电、ICM 模块本体或驱动交互。Windows COM 号已从 COM6 变为 COM3，后续脚本必须先核对枚举；30/60 秒及小时长测继续禁止启动。 |
| 一手资料 | [TDK ICM-42688-P Datasheet DS-000347](https://invensense.tdk.com/wp-content/uploads/2020/04/ds-000347_icm-42688-p-datasheet.pdf)；用于核对 record count、partial-read、watermark 和 FIFO packet/header 语义。 |
| 证据等级 | A/B：复电前后板端 timeline/diagnostics、所有 A/B 镜像、预检拒绝、烧录和回读为 A；物理层或具体从机根因目前仅为 B 级方向，待单器件隔离和波形证据。 |

面试讲述要点：可恢复错误最容易被“最终健康”掩盖，测试前提污染也最容易被事后忽略。发现 COM6 未关闭后，保留 `gsVcum` 原始数据但撤销其干净资格；随后用 COM6 关闭的 `XMWvGC` 重做相同试验，仍在约 58 秒累计 232 次 recovery 和 45 次最终 failure。这个 A/B 既没有删除不利证据，也没有把相关性误写成根因，并把下一步明确收敛到单器件电气测量和事务波形。

### TRB-20260719-025: 罗盘集成验证缺少输入且过早采样

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；新增 CompassEstimator 后首次完整 Runtime 验证 |
| 模块与版本 | `CompassEstimator`、`McuMockService`、`DashboardService`、`verify_runtime.ps1`；MP157 CTest 9/9 基线 |
| 环境与前提 | 独立算法测试已覆盖四方位、磁偏角、硬铁修正、30° 倾斜、样本过期、磁场越界和奇异矩阵；完整验证使用默认 `mcu_mock_frames.txt` 和单次 dashboard 刷新。 |
| 问题现象 | 第一次完整验证报告“runtime status output did not report a valid compass estimate”。增加专用 IMU/磁场 fixture 后，最终 JSON 已为 `valid=true`，但 dashboard 连续两次仍为 `tilt_compensated=false`。 |
| 影响范围 | 算法本身没有被证据否定，但 Runtime 状态和 UI 的端到端验收不成立；如果只保留 CTest 结论，会漏掉输入夹具语义和协作调度时序问题。 |
| 根因结论 | 第一层根因是默认 MCU fixture 只有 heartbeat/mock sensor/IMU 和故意错误 CRC，没有 magnetometer，算法正确拒绝缺输入。第二层根因是 dashboard 的有限刷新在 mock 文件的注释/空行和传感器帧消费完成前结束；即使最终 Runtime 快照有效，UI 文件仍是早期快照。 |
| 无效尝试 | 直接要求默认 fixture 输出有效航向；随后只把 dashboard 刷新次数从 1 增加到 2、再增加到 4。由于服务按轮次各处理一个有界步骤，前导注释/空行使这两个次数仍早于磁力计帧。 |
| 最终方案 | 保留默认负向 fixture，不伪造它已有磁力计；新增语义单一的 `mcu_mock_frames_compass.txt`。MCU frame 应用成功后立即执行派生罗盘更新，再调用 history callback，使同轮后续 dashboard 可见一致状态；聚焦验证使用 32 次零间隔刷新覆盖完整 fixture，并以结构化 JSON 检查 valid/quality/heading 范围。 |
| 验证结果 | MP157 CMake/MSVC 构建通过，CTest 9/9；`verify_runtime.ps1` 最终通过。聚焦 JSON 得到 `valid=true`、`tilt_compensated=true`、`quality=uncalibrated`、航向位于 `[0,360)`；最终 dashboard 同样包含 `tilt_compensated: true`。ARMv7 hard-float 交叉构建通过；同一二进制部署到 MP157 后，板端 fixture 复验得到 heading `30.303°`、field `57.711 uT` 且 JSON/dashboard 一致。 |
| 剩余风险 | 本条只闭环软件集成和测试时序；真实安装矩阵、硬铁/软铁系数、当地磁偏角和现场精度仍按 ADR-0022 待板端/室外采集。 |
| 证据等级 | A：三次失败输出、聚焦 fixture、最终 JSON/dashboard、CTest、Runtime verifier 和 ARM 构建均为直接证据。 |

面试讲述要点：算法单测通过不等于数据链路已经接通；端到端失败也不应立刻归因算法。先检查夹具是否提供所有必需输入，再沿协作调度的轮次分析派生状态和 UI 快照时序，最后让验证同时覆盖负向缺输入与正向完整输入。

### TRB-20260719-026: 板端临时文件清理命令嵌套引号失败

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；MP157 罗盘聚焦验证完成后的 `/tmp` 清理 |
| 模块与版本 | Windows PowerShell inline `SerialPort` helper；MP157 COM9 shell |
| 问题现象 | 主机立即报告 `Missing ')' in method call`、`Unexpected token` 和 `Try statement is missing its Catch or Finally block`，没有得到任何串口输出。 |
| 影响范围 | PowerShell 在语法解析阶段即终止，板端没有执行删除；仅两个本轮生成的 `/tmp/compass_*.{json,txt}` 暂时保留，项目文件和正式证据未受影响。 |
| 根因结论 | 在双引号 PowerShell 字符串中同时手工转义远端 shell 的 `$?`、`$r` 和双引号，导致宿主语言字符串提前结束。问题发生在 PowerShell parser，不是 COM9、BusyBox `rm` 或文件权限问题。 |
| 无效尝试 | 把远端命令、结果 token 和 PowerShell 控制流压进一个手工转义的 `SerialPort.Write(...)` 表达式。 |
| 最终方案 | 复用已通过正向运行/读取验证的 `Invoke-Mp157Command` 封装：远端命令作为独立参数，封装统一追加不会被 echo 误命中的数字结果 token；精确删除两个已知 `/tmp` 文件，再用 `test ! -e ... -a ! -e ...` 验证不存在。 |
| 验证结果 | 返回 `__CODEX_COMPASS_CLEAN_0__`，两个临时文件均确认不存在。 |
| 本轮复现 | 2026-07-20 首次 COM9 状态探针再次把外层 PowerShell here-string 与内层 here-string 使用同一终止边界，主机报 `Unexpected token '}'` 和 `The string is missing the terminator`；串口尚未打开，板端未执行命令。改为直接构造不含嵌套引号的固定标记命令后，COM9 正常返回进程状态。 |
| 剩余风险 | 后续应把串口 console helper 固化为仓库脚本并增加引用/echo 自测；不要继续复制一次性多层转义片段。 |
| 证据等级 | A：主机 parser 原始错误、未出现串口输出、修复后远端 token 和存在性检查均为直接证据。 |

面试讲述要点：跨 PowerShell、串口和远端 shell 的自动化有三层解析边界。看到宿主 parser 错误时应先确认远端根本没有执行，再复用经过验证的参数化封装，而不是在同一转义表达式上继续试错。

### TRB-20260719-027: Logger 失败只写 stderr 无法审计

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；重新审计 `project_design` 与 ADR-0018 的未闭环风险 |
| 模块与版本 | MP157 `Logger`、`RuntimeStatus`、`StatusPublisher`、板端预检/长测审计；最终 ARM Runtime 258560 B，SHA256 `330b7bc4ee95d58e83ce7590f122e67cffc8e112a8b741801102eb95cb52a645` |
| 环境与前提 | Runtime 以单线程协作式调度运行，storage 日志采用标准库 `ofstream` 与 `filesystem` 按大小轮转；正式运行可能由串口脚本后台启动，stderr 不一定持续采集。 |
| 问题现象 | 轮转的 remove/rename/reopen 失败或活动日志 write/flush 失败只打印 `[LOGGER] ...` 到 stderr；`runtime_status.json` 没有累计计数和最后错误。 |
| 影响范围 | 无人值守运行中若 stderr 未保留，日志介质故障可能只留下缺失或不完整文件；预检与长测只能检查最终文件、备份数和通用 storage 初始化错误，无法区分运行期曾失败后恢复的情况。 |
| 根因结论 | Logger 的文件操作路径没有持久健康状态，错误处理终止于控制台输出；`RuntimeStatus` 和验收脚本也没有对应字段。该问题是观测性/验收链路缺口，不是已证明的 SD 介质故障。 |
| 不足方案 | 1. 只依赖 stderr：后台运行或串口未持续抓取时证据易丢失。2. 只检查活动日志和 `.1` 到 `.3` 是否存在：不能证明每次轮转/write/flush 都成功，也会被后续恢复掩盖。3. 立即让 Runtime 因一次可恢复轮转失败退出：会牺牲其余传感器采集，且没有必要；最终选择继续写入并让健康门槛决定验收失败。 |
| 最终方案 | 增加会话级 `LoggerStatus`，累计轮转失败、写/flush 失败和最后错误；轮转异常后尝试重新打开活动文件，恢复成功也不清除故障事实。状态发布新增三个字段，并将最后错误合并到 `storage.last_error`。预检与长测审计要求两类计数为零且错误为空。 |
| 验证结果 | 非空 `.1` 目录确定性阻止备份删除，`logger_tests` 验证 rotation failure 计数为 1、write failure 为 0、错误保留且活动日志继续写入；`status_publisher_tests` 验证非零计数和错误转义。MP157 CTest 10/10、Runtime verifier、ARM 构建和 COM9 部署通过。板端 fixture 发布三项健康值，预检目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-FImCIn` 明确通过三个 Logger 门槛。 |
| 剩余风险 | 当前只对轮转文件操作做了可移植故障注入；真实 SD 写满、拔卡或只读重挂载造成的 write/flush 失败尚未执行。代码路径和非零 JSON 序列化已覆盖，但介质级失败仍需独立、可恢复的板端测试。 |
| 证据等级 | A/B：主机故障注入、CTest、Runtime verifier、ARM 构建、部署哈希、板端状态与预检为 A；真实 SD write/flush 故障注入未执行，相关介质行为保持 B/待验证。 |

面试讲述要点：文件“最后存在”不等于整个运行期写入健康。对于可恢复 I/O 错误，既不应静默继续，也不一定要立即杀死全部采集；更合适的做法是保留单调故障证据、尽力恢复数据通路，并由结构化健康门槛明确判定本次验收失败。

### TRB-20260719-028: UART4 RX 丢弃只有布尔位无法累计审计

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；审计 UART4 RX 长期可靠性 TODO 时 |
| 模块与版本 | F407 UART4 64 字节 RX 环形缓冲、Sensor Hub diagnostics、MP157 parser/status/预检/时间线/审计；当前 F407 BIN 19376 B、SHA256 `365fb9c3...129c`，MP157 Runtime 258560 B、SHA256 `47c4bcc9...b739` |
| 环境与前提 | F407 UART4 PC10/PC11 与 MP157 USART3 PD9/PD8 双向连接，115200 8N1；ISR 在 RX 环满时已经拒绝新字节并累计内部计数，heartbeat bit `0x2000` 只能报告“本次启动曾溢出”。 |
| 问题现象 | MP157 能从 heartbeat 看到 RX overflow sticky 位，却不知道丢了多少字节、何时增长，也无法在逐秒时间线上检查计数是否单调或计算一次长测的增量。 |
| 影响范围 | 一次丢字节和持续接收过载在验收结果中没有区分；最终 heartbeat 只能判定发生过异常，不能为容量分析、回归比较和面试复盘提供量化证据。 |
| 根因结论 | 计数已经存在于 F407 BSP，但协议层没有携带它。旧 44 字节 diagnostics 最后一字节仅为 reserved，直接复用而不声明版本会让旧 MP157 对扩展语义没有可验证边界；只增加 heartbeat 位也不能表达规模。 |
| 方案比较 | 1. 只保留 heartbeat：实现最小，但无累计规模和时间序列。2. 静默改写 reserved：载荷长度不变，但只能容纳 8 位且没有版本协商。3. 新增独立帧类型：边界清晰，但增加调度、解析和长期协议面。4. 在旧载荷尾部追加版本化扩展：legacy 仍可按 44 字节解析，当前字段集中在同一诊断快照；最终选择方案 4。 |
| 最终方案 | 定义 legacy payload 44、current payload 48、extension version 1；byte 43 为 schema version，byte 44..47 为小端 32 位 UART4 RX 累计丢弃计数。MP157 对 44 字节映射 schema 0/drop 0，对 48 字节只接受 version 1，未知版本拒绝。JSON、文本状态、预检、23 列时间线、结束审计和 COM6 verifier 全部接入新字段。部署顺序为先部署兼容新旧的 MP157，再刷 F407 schema 1，避免升级窗口内解析中断。 |
| 验证结果 | F407 CTest 7/7，frame-builder 测试验证 schema 1、`0x01020304` 小端编码和 CRC；MP157 CTest 10/10，parser 覆盖 schema 1 count 9、legacy schema 0/drop 0 和未知版本拒绝，status publisher 覆盖 schema 1/drop 7。两侧 ARM 构建、COM9 部署、COM6 烧录回读通过。零值 monitor 探针得到 4 行、每行 23 列。确认 `/dev/ttySTM1` 无占用后连续下发 40960 字节，预检 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-jYMnDJ` 解码 heartbeat `12334 (0x302E)`、schema 1、drop 2231，并按设计以非零 drop 返回失败；使用既有 DTR/RTS 应用复位序列后，`/run/media/mmcblk1p1/outdoor-agent-health-preflight-XOXgzn` 恢复 schema 1/drop 0，证明启动周期清零。 |
| 剩余风险 | 本条要求的“非零累计、门槛拒绝、复位归零”已闭环。尚未测量不同流量、合法命令帧与持续过载下的消费延迟和容量边界；这些属于后续可靠性容量评估，不影响本问题关闭。应用复位不是 ICM42688 完整复电，不能借此关闭 TRB-004/024。 |
| 证据等级 | A：主机测试、ARM 构建、部署哈希、烧录回读、零值时间线、真实非零压力注入、失败预检和复位后零值预检均为直接证据。 |
| 关联资料 | [ADR-0023](adr/0023-version-sensor-hub-diagnostics-extension.md)、[MCU 协议](mcu_protocol.md)、[开发日志](dev_log.md#2026-07-19---versioned-uart4-rx-drop-diagnostics) |

面试讲述要点：sticky bit 适合回答“是否发生过”，累计计数才适合回答“发生了多少、何时增长、修复前后是否改善”。协议扩展不能只看当前两端代码，还要设计 legacy 兼容、未知版本拒绝和可安全执行的部署顺序；同时必须把真实非零故障注入与零值链路验证明确分开。

### TRB-20260719-029: 预检位置参数顺序误用

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；TRB-028 实板压力注入后的首轮预检 |
| 模块与版本 | `run_board_health_preflight.sh` 当前接口：`duration_seconds runtime_binary runtime_config storage_mount`；Windows COM9 串口命令 helper |
| 问题现象 | 首轮命令把 Runtime 路径作为第一个参数，脚本立即输出 `duration_seconds must be a positive integer` 并返回 rc 2。没有创建预检证据目录，也没有启动 Runtime。 |
| 影响范围 | 只延迟本次故障注入验证；没有改变 Runtime、F407 状态或已有证据。若忽略 rc 2，可能把“测试未执行”误写成门槛失败。 |
| 根因结论 | 操作者沿用了错误的参数顺序，没有先核对脚本头部的位置参数定义。脚本的数字校验正确阻止了误执行，返回值明确属于调用错误而非被测系统失败。 |
| 最终方案 | 重新读取脚本入口，按 `8 /opt/outdoor-agent/bin/outdoor_core_runtime /opt/outdoor-agent/config/runtime.conf /run/media/mmcblk1p1` 顺序调用，并要求结果 token 必须为预期的 rc 1；后续记录必须区分 rc 2“未执行”和 rc 1“门槛按设计拒绝”。 |
| 验证结果 | 修正后 Runtime 受控退出 rc 0，脚本完成全部门槛并以总体 rc 1 返回；目录 `/run/media/mmcblk1p1/outdoor-agent-health-preflight-jYMnDJ` 明确失败于 ICM 状态和 `uart4_rx_drop_count_zero` 两项，成功取得 drop 2231 的有效证据。 |
| 剩余风险 | 位置参数仍依赖调用者遵守顺序；仓库内长测脚本使用固定调用，不受本次手工误用影响。后续若公开给更多操作者，可再增加 `--help` 或命名参数包装。 |
| 证据等级 | A：rc 2 原始输出、无测试目录、脚本参数定义、修正命令、rc 1 报告和状态 JSON 均为直接证据。 |

面试讲述要点：测试工具的退出码也要分类。参数校验失败代表“测试没有运行”，不能与业务门槛失败混为一谈；先读入口契约、再用证据目录和具体失败项证明真正执行过，能避免自动化自己制造误结论。

### TRB-20260719-030: 罗盘校准器测试挂起与离群点拟合失败

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；新增 `tools/compass_calibrator` 后首次主机回归 |
| 模块与版本 | C++17 全椭球拟合、MSVC 17.14 Debug、GCC 16.1 Release、CMake/CTest；新工具独立构建目录 |
| 环境与前提 | 合成数据使用 Fibonacci 球面方向、已知硬铁偏置、正定非对角校正矩阵和确定性小噪声；后续另注入 5 个约 ±250 μT 的极端点。 |
| 问题现象 | 首轮 CTest 超过 20 秒无输出，残留 `compass_calibrator_tests` PID 21828 和 `ctest` PID 24720。改为 stderr 断言后显示方向方差比 `>0.8` 失败。增加强磁点后，拟合在 `result.fitted` 前失败。GCC Release 当时虽然 2/2 通过，却报告 `expected` 未使用，证明裸 `assert` 已因 `NDEBUG` 被全部编译掉。 |
| 影响范围 | Debug 自动化会被 modal 阻塞；强磁环境下少量极端点可使代数二次曲面非正定；Release 测试可能假绿。这三项都会让校准工具的工程证据不可信。 |
| 根因结论 | 1. MSVC Debug CRT 的裸 `assert` 默认打开交互对话框，非交互 CTest 无法返回。2. 测试错误地要求畸变后的原始方向仍接近均匀分布，而该指标只应排除近平面覆盖。3. 普通最小二乘对平方项的高杠杆离群点敏感，初始曲面已非正定，事后残差裁剪来不及运行。4. 标准 `assert` 在 Release 被编译删除，不能作为跨配置测试框架。 |
| 最终方案 | 用始终生效的 `CHECK` 替换全部裸断言，失败直接打印表达式/文件/行号并返回非零；方向方差门槛只证明明显三维覆盖，恢复精度由偏置、逐矩阵元素和残差分别断言；拟合前增加分量中位数中心与径向 MAD 粗筛，拟合后保留残差 MAD 精筛。所有测试同时在 MSVC Debug 和 GCC Release 运行。 |
| 验证结果 | 精确确认残留进程路径后终止两个 PID。最终 MSVC Debug CTest 3/3、GCC Release CTest 3/3，GCC `-Wall -Wextra -Wpedantic` 无警告。1600 点合成数据偏置误差 `<0.05 μT`、每个矩阵元素误差 `<0.01`、RMS `<0.2%`、最大残差 `<0.5%`；5 个极端点全部计入 rejected，拟合质量恢复；近平面和非法安装旋转均被拒绝；固定球面 CSV 跑通命令行拟合和候选配置写出。 |
| 失败/回退尝试 | 首先假设构建仍在运行并等待，实际是 assert modal；随后只把 MSVC assert 改为写 stderr，能定位 Debug 失败但没有解决 Release 假绿；增加事后残差裁剪也无法拯救已被强离群点破坏的初始二次曲面，因此补充拟合前粗筛。 |
| 其他排查记录 | 初次结构审计尝试读取仓库根 `CMakeLists.txt`，实际仓库没有根级 CMake；该读取失败没有修改文件，工具随后按现有 `tools/frame_decoder` 模式采用独立 CMake 工程。 |
| 剩余风险 | 合成测试不能代表当前整机磁环境；代数拟合对严重不均匀姿态仍可能有偏差。真实数据必须保存原始 CSV、比较多轮独立拟合，并通过八方向/倾角/动态验收。 |
| 证据等级 | A：挂起进程、断言原始输出、强离群点失败、GCC 警告、修复后双编译器测试和数值误差门槛均为直接证据。 |
| 关联资料 | [ADR-0024](adr/0024-fit-compass-calibration-offline.md)、[工具说明](../tools/compass_calibrator/README.md)、[开发日志](dev_log.md#2026-07-19---offline-full-matrix-compass-calibration-tool) |

面试讲述要点：数值算法测试不能只看“CTest 绿色”。Debug 可能被交互断言阻塞，Release 又可能把同一断言删除；同时，离群点能在残差阶段之前破坏模型。通过跨配置始终生效的检查、合成真值误差、负向覆盖和拟合前后两阶段稳健处理，才能形成可复查的算法工程证据。

### TRB-20260719-031: 罗盘工具 build-gcc 产物未被忽略

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-19；罗盘校准器双编译器回归后的最终 Git 范围审计 |
| 模块与版本 | 根 `.gitignore`；`tools/compass_calibrator/build-gcc`；GCC 16.1、Ninja、CMake 4.3.3 |
| 环境与前提 | MSVC 使用目录名 `build`，GCC Release 为避免生成器缓存冲突使用独立目录名 `build-gcc`。 |
| 问题现象 | `git status --short --untracked-files=all -- tools/compass_calibrator` 列出 `.ninja_*`、`CMakeCache.txt`、CMakeFiles、静态库和 `compass_candidate_smoke.conf` 等构建产物。 |
| 影响范围 | 不影响编译和测试结果，但若只看聚合目录状态，可能把生成文件误加入提交，污染作品集历史并增大仓库。 |
| 根因结论 | 根忽略规则 `build/` 只覆盖名为 `build` 的目录；既有 `/f407/sensor-hub/build-gcc/` 是模块专用规则，没有覆盖新工具路径。 |
| 最终方案 | 在根 `.gitignore` 增加 `/tools/compass_calibrator/build-gcc/`，保留独立 GCC 构建目录和跨编译器回归能力，不删除用户源码或修改构建命令。 |
| 验证结果 | 重新执行同一精确范围 `git status --short --untracked-files=all -- tools/compass_calibrator` 后，仅列出应提交的 CMake、README、include、src、测试源码和固定 CSV；不再出现 `build` 或 `build-gcc` 产物。`git diff --check` 同时通过。 |
| 本轮复现 | 新增 F407 `build-icm-only`/`build-icm-only-v2` 后，精确 `git status --untracked-files=all` 再次列出 Ninja/CMake/ELF/BIN/map 产物；新增 `/f407/sensor-hub/firmware/build-icm-only*/` 忽略规则后不再进入提交范围。该重复问题沿用本条，不另建仓库卫生问题。 |
| 失败/回退尝试 | 初始假设全局 `build/` 会覆盖所有以 build 开头的目录；精确枚举未跟踪文件证明该假设错误。未采用删除构建目录，因为忽略规则才是可重复且不破坏本地缓存的修复。 |
| 剩余风险 | 后续若新增 `out-*`、`build-clang` 等命名，需要同步采用统一目录约定或增加明确规则；提交前仍应精确枚举新模块。 |
| 证据等级 | A：修复前后使用同一 `git status --untracked-files=all` 命令直接对比。 |

面试讲述要点：构建成功不等于提交范围干净。多编译器工程经常使用多个构建目录，提交前应精确展开未跟踪文件，验证忽略规则覆盖的是实际生成器目录，而不是凭通配直觉判断。

### TRB-20260720-032: F407 隔离构建参数解析失败

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-20；首次创建 ICM42688-only 独立 ARM 构建目录 |
| 模块与版本 | Windows PowerShell、CMake 4.3.3、Ninja、GNU Arm 14.2；`firmware/build-icm-only` |
| 问题现象 | 单行相对参数调用报告忽略额外路径 `.cmake`、找不到 toolchain `cmake/arm-none-eabi-gcc`，并报告新目录没有 `CMAKE_MAKE_PROGRAM`；随后产物/哈希读取因 BIN 不存在继续报错。 |
| 影响范围 | 首次隔离镜像没有配置、编译或刷写；正式 `firmware/build` 和板端固件未受影响。 |
| 根因结论 | 新构建目录不能依赖既有 cache 中的生成器和工具路径；把 `-D`、toolchain 与 Ninja 混在一条未校验的原生命令中也没有建立“配置失败立即停止”的边界。相对 toolchain 参数在该调用链中未被可靠保留为单个有效文件路径。 |
| 无效尝试 | 1. 失败后同一命令仍读取不存在的 BIN。2. 为寻找 Ninja 对 `C:\` 做递归枚举，30 秒超时；其实 `Get-Command ninja` 和已有 cache 已足够。3. 尝试在同一复杂命令中验证并递归删除失败目录，被执行策略拒绝且没有删除任何文件。 |
| 最终方案 | 用 PowerShell 参数数组分别传递 `-S/-B/-G/-D`，解析 toolchain 和 Ninja 的绝对路径，并在每个 CMake 阶段立即检查退出码；改用新的 `build-icm-only-v2` 避免复用不完整 cache。 |
| 验证结果 | CMake 明确识别 GNU 14.2.1 和 Ninja，Release 47/47 编译链接通过；得到 15072 B BIN、SHA256 `c07e235a...2f80a`，随后 COM6 烧录和逐字节回读通过。 |
| 剩余风险 | `build-icm-only` 的不完整 cache 仍作为已忽略的本机构建产物存在，不进入版本库；后续应复用文档化参数数组或专用构建脚本，避免复制手工单行调用。 |
| 证据等级 | A：CMake 原始错误、超时、策略拒绝、成功配置输出、固件哈希和烧录回读均为直接证据。 |

面试讲述要点：新构建目录没有历史 cache，不能把生成器、toolchain 和构建工具视为隐含环境。对每个阶段立即检查退出码，并用绝对路径参数数组保存参数边界，可避免配置失败后继续对不存在产物做“验证”。

### TRB-20260721-033: 复用已有 build 目录时 CMake 生成器冲突

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-21，Stage 2.1 开发前主机基线配置 |
| 模块与版本 | `mp157/outdoor-core-service`；CMake；当前分支 `agent/stage2-unix-status-ipc` |
| 环境与前提 | Windows PowerShell；仓库内 `build` 目录已存在；本轮尝试显式使用 Ninja 生成器。未执行任何串口、复位、烧录或其他硬件交互。 |
| 问题现象 | `cmake -S . -B build -G Ninja` 返回：`generator : Ninja Does not match the generator used previously: Visual Studio 17 2022`。 |
| 影响范围 | 首轮 MP157 主机基线未配置成功；并行验证调用提前返回，尚不能据此判断其余模块结果。源码和已有构建缓存未被修改。 |
| 复现步骤 | 在已有 Visual Studio CMake cache 的 `mp157/outdoor-core-service/build` 上显式执行 Ninja 配置。 |
| 预期结果 | 完成独立 Release 配置、构建和 CTest。 |
| 实际结果 | CMake 在配置阶段拒绝混用生成器并返回非零。 |
| 根因结论 | 已有 `build/CMakeCache.txt` 明确记录 Visual Studio 17 2022；CMake 不允许在同一二进制目录切换到 Ninja。使用全新目录后配置立即通过，排除了源码和工具链故障。 |
| 最终方案 | 本轮基线与回归统一使用 `%TEMP%/ai-outdoor-stage2-baseline/<module>` 独立构建树，不删除或覆盖用户已有 build cache；每个配置、构建、CTest 阶段均检查退出码。 |
| 验证结果 | MP157 在独立 Ninja Release 构建树完成 47 个构建步骤，CTest 10/10 通过；Compass Calibrator CTest 3/3、Frame Decoder 构建和 F407 ARM 构建也完成。 |
| 剩余风险 | 本问题已闭环。MSVC 提示临时目录不适合长期增量构建，本轮只用于一次性隔离验证；长期本地开发仍应使用按生成器区分的持久构建目录。 |
| 证据与关联资料 | 本轮 CMake 原始错误输出；Stage 2.1 开发日志待完成后关联。 |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | 复用模块既有 `build` 并指定 Ninja | 希望避免重复生成目录 | CMake 报告既有生成器为 Visual Studio 17 2022 | 不复用 cache，改用独立临时构建树 | A |
| 2 | 在系统临时目录建立独立 Ninja Release 构建树 | 新目录不携带既有生成器状态 | MP157 配置、构建和 CTest 10/10 通过 | 根因与隔离方案确认，问题关闭 | A |

面试讲述要点：构建目录包含生成器状态，不能把它当作可在 MSVC 与 Ninja 之间共享的普通输出目录；基线验证应使用明确隔离的构建树，并在配置失败后停止后续步骤。

### TRB-20260721-034: Release 禁用 assert 导致测试副作用丢失与假绿

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-21，F407 主机 Release 独立基线 |
| 模块与版本 | `f407/sensor-hub/tests`；GCC 16.1；CMake Release；当前分支 `agent/stage2-unix-status-ipc` |
| 环境与前提 | Windows PowerShell；全新 Ninja 构建树；`CMAKE_BUILD_TYPE=Release`。未执行任何板端或串口交互。 |
| 问题现象 | 编译输出报告大量断言相关变量/函数未使用；CTest 6/7 中 `uart_tx_queue_tests` 以 `NUMERICAL` 异常失败，其他测试虽返回通过但存在断言被删除的假绿风险。 |
| 影响范围 | F407 PC 单元测试无法在 Release 下提供可信证据；如果初始化或被测调用写在 `assert(...)` 内，其副作用会随 `NDEBUG` 一起消失。固件生产源码尚无证据表明受影响。 |
| 复现步骤 | 在全新目录执行 `cmake -S f407/sensor-hub -B <dir> -G Ninja -DCMAKE_BUILD_TYPE=Release`、构建并运行 CTest。 |
| 预期结果 | 7/7 测试在 Release 下执行全部检查并通过，不依赖 Debug 专用断言。 |
| 实际结果 | `uart_tx_queue_tests` 数值异常；编译器对其余测试给出大量未使用警告，证明检查主体已被编译删除。 |
| 根因结论 | 7 个 F407 主机测试文件全部直接使用标准 `assert`，多个初始化、enqueue、FIFO 解析和 provider 调用位于断言表达式内。Release 定义 `NDEBUG` 后删除表达式，UART 队列保持零容量并在后续操作中触发数值异常；其余用例的未使用告警证明检查未执行。 |
| 最终方案 | 新增仅供测试使用的 `test_check.h`，以单次求值、打印表达式/文件/行号并 `EXIT_FAILURE` 退出的始终生效检查替代标准断言语义；7 个测试源统一引用该头文件，生产固件源码和 ARM 编译选项不变。 |
| 验证结果 | 全新 GCC 16.1 Ninja Release 重新构建时原未使用告警消失，CTest 7/7 通过；MSVC 19.44 Debug CTest 7/7 通过；`scripts/build_f407.ps1` ARM 固件构建通过。 |
| 剩余风险 | 本问题已闭环。测试仍使用历史 `assert(...)` 拼写，但在测试源内由专用头明确重绑定为 always-on check；后续新增测试应继续包含该头或直接使用 `TEST_CHECK`。 |
| 证据与关联资料 | Release 编译告警和 CTest `uart_tx_queue_tests (NUMERICAL)` 原始输出；开发日志待闭环后关联。 |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | 用全新 Release 构建树运行 F407 CTest | 排除旧 cache 和 Debug 配置干扰 | 大量未使用告警，UART 队列测试数值异常 | 检查测试中的 `assert` 与断言内副作用 | A |
| 2 | 扫描全部 F407 测试并引入 always-on test check | 7 个文件均使用标准 `assert`，且存在断言内副作用 | Release 无原告警且 7/7；MSVC Debug 7/7；ARM 构建通过 | 根因、修复和跨配置验证闭环 | A |

面试讲述要点：单元测试显示绿色不代表检查真实执行；跨 Debug/Release 验证、编译告警和失败类型可以共同揭示 `NDEBUG` 删除断言及副作用的问题。

### TRB-20260721-035: PowerShell 将交叉编译器变量作为字面量传给 CMake

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-21，Stage 2.1 首次 MP157 ARM 交叉配置 |
| 模块与版本 | `mp157/outdoor-core-service`；CMake 4.3.3；GNU ARM Linux 9.2.1；当前分支 `agent/stage2-unix-status-ipc` |
| 环境与前提 | Windows PowerShell；编译器文件已确认存在；使用全新临时 ARM 构建树。未执行串口、部署或板端命令。 |
| 问题现象 | CMake 报告 `CMAKE_CXX_COMPILER: $compiler is not a full path and was not found in the PATH`，配置阶段直接失败。 |
| 影响范围 | 首轮 MP157 ARM 编译未执行；尚不能用该轮判断 POSIX Unix socket 源码是否能被 9.2.1 工具链编译。源码和板端状态未改变。 |
| 复现步骤 | 在 PowerShell 原生命令中以内联 `-DCMAKE_CXX_COMPILER=$compiler` 形式传递变量。 |
| 预期结果 | CMake 收到交叉编译器绝对路径并生成 Linux/ARM Ninja 构建树。 |
| 实际结果 | CMake 收到字面量 `$compiler`。 |
| 根因结论 | PowerShell 对该原生命令参数的拼接没有形成预期的单个 `-Dkey=value` 字符串；与 TRB-20260720-032 的参数边界问题同类。显式参数数组复验确认不是源码或编译器路径缺失。 |
| 最终方案 | 构造包含完整 `-DCMAKE_CXX_COMPILER=<absolute-path>` 的参数数组，并在配置成功后才进入构建。 |
| 验证结果 | CMake 正确识别 GNU ARM Linux 9.2.1；修正测试线程链接后，`outdoor_core_runtime`、`outdoor_status_query` 和全部测试目标完成 ARM 全量编译链接。 |
| 剩余风险 | 本问题已关闭；长期可用独立 toolchain file 进一步减少命令行参数边界风险。真实 Unix socket 运行验收属于 Stage 2.1 板端待办，不影响本配置问题闭环。 |
| 证据与关联资料 | 本轮 CMake 原始错误；关联 TRB-20260720-032。 |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | 以内联 PowerShell 变量传递交叉编译器 | 编译器绝对路径已存在 | CMake 收到字面量 `$compiler` 并拒绝配置 | 改用显式参数数组 | A |
| 2 | 用显式参数数组传递完整 `-Dkey=value` | 避免原生命令参数二次拆分 | CMake 识别 GNU 9.2.1 并生成 Linux/ARM Ninja 构建树 | 配置调用问题关闭 | A |

面试讲述要点：交叉编译失败需要先区分“源码编不过”和“配置根本没收到工具路径”；检查 CMake 回显的实际值并在配置失败后停止，可以避免把调用错误误判为代码缺陷。

### TRB-20260721-036: ARM 测试目标未显式链接 pthread

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-21，Stage 2.1 MP157 ARM 首次有效编译 |
| 模块与版本 | `unix_status_service_tests`；GNU ARM Linux 9.2.1；CMake/Ninja Release |
| 环境与前提 | 使用显式参数数组完成 Linux/ARM 交叉配置；`outdoor_core_runtime`、`outdoor_status_query` 和 Unix socket 源码均已编译链接。未运行 ARM 测试，也未部署到板端。 |
| 问题现象 | 链接 `unix_status_service_tests` 时报告两处 `undefined reference to pthread_create`，Ninja 在 53/53 停止。 |
| 影响范围 | ARM 全目标构建返回失败；Runtime 与查询客户端产物已生成，但本轮不能记为完整交叉构建通过。 |
| 复现步骤 | 交叉构建包含 `std::thread` 的 Unix status service 集成测试，测试目标只链接 `outdoor_core`。 |
| 预期结果 | CMake 为测试目标声明并传递平台线程库，所有目标链接完成。 |
| 实际结果 | 测试对象引用 `pthread_create`，链接命令没有 `-pthread` 或 pthread 库。 |
| 根因结论 | 测试通过后台线程驱动非阻塞服务，但 CMake 没有用 `find_package(Threads)` / `Threads::Threads` 表达线程依赖；Windows 原生工具链隐式满足，Linux ARM 链接器则正确拒绝。 |
| 最终方案 | 使用 `find_package(Threads REQUIRED)`，只为 `unix_status_service_tests` 显式链接 `Threads::Threads`，不把测试专用线程依赖扩散到 Runtime 生产目标。 |
| 验证结果 | Windows GCC Release CTest 11/11 通过；GNU ARM Linux 9.2.1 的 Runtime、查询客户端和全部测试目标完成编译链接，原 `pthread_create` 未定义引用消失。 |
| 剩余风险 | 本链接问题已关闭。ARM 测试二进制在当前 Windows 主机只能完成交叉链接；实际 Unix socket 生命周期仍需 Linux 主机或 MP157 后续验收。 |
| 证据与关联资料 | ARM 链接器 `undefined reference to pthread_create` 原始输出。 |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | 用修正后的参数数组执行 ARM 全目标构建 | 验证 POSIX 实现与旧工具链兼容 | 生产目标链接成功，仅线程化测试缺少 pthread | 在测试目标声明 Threads 依赖 | A |
| 2 | 仅给集成测试链接 `Threads::Threads` 后重建 | 依赖只属于测试驱动线程 | Windows 11/11 CTest 与 ARM 全目标链接通过 | 根因、修复和回归闭环 | A |

面试讲述要点：跨平台测试依赖也必须进入构建模型；Windows 能链接不代表 Linux 目标会自动补 pthread，应把依赖精确放在使用线程的测试目标上，避免污染生产二进制。

### TRB-20260721-037: PowerShell 把预期失败子进程 stderr 提升为终止错误

| 字段 | 记录 |
| --- | --- |
| 状态 | 已关闭 |
| 首次发现时间 | 2026-07-21，Stage 2.2 Runtime supervision verifier 首轮负向测试 |
| 模块与版本 | `verify_runtime_supervision_tests.ps1`；Windows PowerShell 5.1 |
| 环境与前提 | 纯主机测试；用子 PowerShell 运行 verifier，并故意把 `Restart=on-failure` 改为 `Restart=no`。未访问 COM3/COM9 或板端。 |
| 问题现象 | verifier 按设计向 stderr 报告不安全重启策略，但父脚本在取得 `$LASTEXITCODE` 前以 `NativeCommandError` 终止。局部修复后，四个负向用例均符合预期，脚本作用域仍保留最后一个子进程的 `$LASTEXITCODE=1`，调用方又把成功自测误判为失败。 |
| 影响范围 | 负向自测不能区分“verifier 正确拒绝 fixture”和“测试驱动自身失败”；生产 unit、Runtime 和板端状态均未改变。 |
| 复现步骤 | 父脚本设置 `$ErrorActionPreference = 'Stop'`，执行预期返回非零且写 stderr 的子 `powershell.exe`，并尝试用退出码判断结果。 |
| 预期结果 | 捕获子进程 stdout/stderr 和退出码；预期失败用例返回非零时测试继续。 |
| 实际结果 | 重定向后的 native stderr 先被提升为终止错误，退出码断言未执行。 |
| 根因结论 | Windows PowerShell 在 `ErrorActionPreference=Stop` 下把 native 子进程 stderr 包装为错误记录；预期失败测试必须在局部允许该错误流，再恢复严格错误策略并检查退出码。此外，以 `&` 调用另一个 `.ps1` 不会自动把成功状态写回 `$LASTEXITCODE`，该变量可能保留脚本内部最后一个 native 子进程的值。 |
| 最终方案 | `Invoke-Verifier` 只在子进程调用期间把 `ErrorActionPreference` 临时设为 `Continue`，在 `finally` 恢复原值，并以当次 `$LASTEXITCODE` 判断正/负向结果；调用 `.ps1` 的上层脚本改为依赖 PowerShell 异常传播，不再读取可能陈旧的 `$LASTEXITCODE`。 |
| 验证结果 | 有效 unit/config 正向通过；`Restart=no`、`StartLimitBurst=0`、`DevicePolicy=auto`、socket disabled 四个负向 fixture 均被 verifier 拒绝；独立自测和完整 `verify_runtime.ps1` 均通过。 |
| 剩余风险 | 本问题已关闭；该测试依赖 Windows PowerShell 子进程语义，目标 Linux 上仍需使用 `systemd-analyze verify` 和真实 service 生命周期验收。 |
| 证据与关联资料 | 首轮 `NativeCommandError` 原始输出；修复后 `Runtime supervision verifier tests passed.`。 |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | 运行含一个正向和四个负向 fixture 的 verifier 自测 | 需要证明静态检查会拒绝被放宽的监管策略 | 首个预期失败 fixture 触发父脚本终止 | 检查 PowerShell native stderr 与严格错误策略交互 | A |
| 2 | 在子进程调用局部使用 `Continue`，随后恢复 `Stop` 并断言退出码 | 预期失败 stderr 是测试数据，不应绕过退出码判定 | 正向与四个负向用例全部符合预期；调用方仍读到最后负向用例遗留的 1 | 去除脚本间调用后的陈旧退出码判断 | A |
| 3 | `.ps1` 调用只依赖异常传播，native 子进程仍在局部检查退出码 | PowerShell 脚本失败会 throw，成功不保证重置全局 `$LASTEXITCODE` | 独立自测和完整 Runtime verifier 均通过 | 两层误判全部闭环，问题关闭 | A |

面试讲述要点：负向测试不仅要让被测程序失败，还要证明测试框架能可靠地区分“预期拒绝”和“驱动崩溃”；对子进程 stderr 和退出码的语义必须显式建模。

## 新问题记录模板

复制以下模板创建新条目，并同时在“问题索引表”增加一行：

```md
### TRB-YYYYMMDD-NNN: 问题标题

| 字段 | 记录 |
| --- | --- |
| 状态 | 分析中 / 已定位 / 已修复待验证 / 已关闭 / 已恢复，根因未完全确认 / 长期观察 |
| 首次发现时间 | YYYY-MM-DD HH:mm，注明时区或板端时间是否可信 |
| 模块与版本 | 分支、commit、固件尺寸/版本、配置 |
| 环境与前提 | 板卡、接线、供电、串口/I2C 参数、外设地址 |
| 问题现象 | 可观察事实，保留原始错误码或状态位 |
| 影响范围 | 功能、模块、数据或用户影响 |
| 复现步骤 | 最小可重复步骤 |
| 预期结果 | ... |
| 实际结果 | ... |
| 根因结论 | 有证据的结论；未确认时明确写未知项 |
| 最终方案 | 修改点与取舍 |
| 验证结果 | 命令、时长、次数、指标和实际结果 |
| 剩余风险 | 未覆盖场景和 TODO |
| 证据与关联资料 | 日志、抓包、测试、状态文件、ADR、开发日志、commit |

排查时间线：

| 时间/顺序 | 假设或动作 | 依据 | 实际结果 | 结论/下一步 | 证据等级 |
| --- | --- | --- | --- | --- | --- |
| 1 | ... | ... | ... | ... | A/B/C |

面试讲述要点：用“现象 -> 缩小故障域 -> 关键证据 -> 方案取舍 -> 验证 -> 剩余风险”的顺序概括，不省略失败尝试，不夸大未完成验证。
```
