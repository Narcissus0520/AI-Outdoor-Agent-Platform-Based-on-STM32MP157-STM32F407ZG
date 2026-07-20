# ADR-0018: Use CSV History Files and Size-based Log Rotation

## 状态

Accepted

## 背景

MP157 SD storage 已能保存 Runtime 状态、文本 dashboard 和追加式日志，但 `data/` 目录尚未用于传感器历史记录，日志也会无限增长。长期户外运行需要可直接导出分析的数据和有上限的日志占用，同时必须保持 C++17/标准库可交叉编译，不为 Stage 1 引入数据库或大型序列化依赖。

协作式 Runtime 现有 GNSS、MCU 和 Board IMU 状态是单线程更新的最新快照。历史记录应复用该确定性访问模型，不能额外创建后台线程或让有限文件回放因为记录组件而永不结束。

## 可选方案

### 方案 A：单个 JSON Lines 文件

优点：

- 不同传感器可以写入同一时间序列。
- 字段扩展灵活。

缺点：

- 当前没有 JSON 库，需要维护字符串转义和不同 payload 的手工序列化。
- 高频 IMU 每行重复字段名，SD 卡占用高于列式 CSV。
- Excel、Python 和常见数据分析工具虽然可读取，但不如独立 CSV 直接。

### 方案 B：每种传感器一个追加式 CSV

优点：

- 只使用标准库，格式轻量且便于 Excel/Python 导入。
- GNSS、MCU IMU、磁力计、气压计和 Board IMU 可以保留不同字段和原始单位。
- 每个文件只在对应状态计数或 uptime 变化时追加，避免 Runtime 高频循环重复写同一快照。

缺点：

- 多传感器跨文件对齐依赖 `host_time_utc` 和 MCU `uptime_ms`。
- CSV 不是原始字节抓包，不保存前导噪声、CRC 错误帧或解析失败载荷。
- CSV schema 变化需要显式版本迁移或新文件。

### 方案 C：SQLite

优点：

- 支持事务、索引、查询和多表关联。
- 更适合后续复杂轨迹检索。

缺点：

- 新增库、交叉编译和目标文件系统依赖。
- 当前数据规模和查询需求不足以证明复杂度合理。

### 日志策略比较

- 按日期分文件依赖板端系统时间在启动早期已正确同步；当前不能保证无 GNSS/NTP 时的日期可靠性。
- 按大小轮转只依赖文件大小，行为可在主机测试中确定性验证，更适合当前阶段。

## 最终决策

选择每种传感器一个追加式 CSV，并采用按大小日志轮转。

- `HistoryRecorder` 不注册为常驻 `IService`，因此不会改变有限服务的完成语义。GNSS/MCU serial service 在每个合法句子/帧应用后触发记录回调，Runtime 循环观察器补充 Board IMU 和 file/mock 路径。
- 输出 `gnss.csv`、`mcu_imu.csv`、`magnetometer.csv`、`barometer.csv` 和 `board_imu.csv`。
- GNSS 使用 `validSentenceCount`，MCU 传感器使用各自 `uptimeMs`，Board IMU 使用 `sampleCount` 去重。
- CSV 记录 UTC host 时间；MCU 文件同时保留 F407 uptime 和协议原始整数单位，避免提前丢失精度。
- 默认每 1000 ms flush；写入或 flush 失败会使 Runtime 失败并保留错误，不静默忽略历史数据损坏。
- `history_enabled` 默认关闭；`--history` 或 `--history-output` 可启用。启用 storage 时，相对 history 路径解析到 storage root 下。
- 日志默认 `storage_log_max_bytes = 1048576`、`storage_log_backup_count = 3`；超过上限时将现有文件依次轮转为 `.1`、`.2`、`.3`。
- `storage_log_max_bytes = 0` 可关闭轮转；backup count 为 0 时只截断活动日志，不保留历史文件。

## 决策理由

该方案解决了当前长期运行最直接的两个缺口，并保持内存固定、依赖简单、Windows 主机测试和 ARMv7 hard-float 交叉编译兼容。独立 CSV 适合作品集演示和离线数据分析；大小轮转不依赖板端时间正确性，并能用小阈值单元测试验证备份顺序和数量上限。

## 风险

- History Recorder 保存成功解析并应用的状态更新，不是 lossless raw-byte capture；需要保存噪声、CRC 错误和解析失败帧时仍应使用原始串口抓包。
- serial service 的逐帧回调在 Runtime 单线程内同步追加到文件缓冲；若真实 SD 写入或 flush 出现长延迟，会增加串口积压，小时级测试需观察该指标。
- 多个 CSV 文件不是事务性写入；突然断电可能使各文件最后一行的时间点不完全一致。
- 每秒 flush 是数据耐久与 SD 写放大的折中，尚未结合真实 SD 卡寿命和掉电行为测量。
- 日志轮转失败在本 ADR 初始实现时只输出到 stderr；该历史风险已由下方 2026-07-19 后续实现闭环。
- 当前没有总磁盘配额、CSV 分片、压缩或历史保留策略。

## 后续 TODO

- 在真实 MP157 SD 卡上执行小时级多传感器记录，统计记录频率、文件增长、flush 延迟和 CPU 占用。
- 做进程强制终止和整板断电测试，检查 CSV 最后一行和日志轮转文件的一致性。
- 若需要无损 100 Hz IMU 历史，设计有界事件队列或直接记录解码帧，并明确 overflow 策略。
- 后续根据数据分析需求评估按日期/会话分目录、压缩归档或 SQLite。

## 2026-07-19 后续实现：结构化 Logger 健康状态

初始实现只把轮转和写入失败输出到 stderr，后台运行或串口未持续采集时无法形成可审计证据。后续保持原有标准库和单线程模型，增加以下闭环：

- `LoggerStatus` 在一次文件输出会话内累计 `rotationFailureCount`、`writeFailureCount` 和 `lastError`；`clearFileOutput()` 只关闭输出，不擦除已发生的故障证据，新 `setFileOutput()` 才开始新的计数会话。
- 轮转失败后尝试以 append 模式重新打开活动日志；若恢复成功，触发本次轮转的日志仍会写入，但累计失败和最后错误不会因恢复而清除。
- `runtime_status.json` 的 `storage` 节点发布 `log_rotation_failure_count`、`log_write_failure_count` 和 `log_last_error`；任一 Logger 错误也进入聚合 `storage.last_error`。
- 板端健康预检和长测结束审计要求两类计数为零、Logger 错误为空；正常文件存在和备份数量不再是唯一日志健康证据。
- 主机测试用非空 `.1` 目录确定性阻止备份删除，验证轮转失败计数递增、错误保留且活动日志恢复后继续写入；独立状态发布测试验证非零计数和错误文本 JSON 转义。

真实 SD 卡写满、拔卡或只读重挂载导致的 write/flush 失败尚未执行受控故障注入，仍属于小时级/掉电验收的剩余项。
