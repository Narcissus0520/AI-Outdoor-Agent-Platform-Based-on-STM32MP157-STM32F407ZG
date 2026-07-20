# Compass Calibrator

`compass_calibrator` 是 MMC5603 离线校准工具。它直接读取 MP157 `HistoryRecorder` 生成的 `magnetometer.csv`，拟合三轴硬铁偏置和完整 3×3 软铁校正矩阵，并输出可复制到 Runtime 配置中的候选值。

该工具不依赖 Eigen、Python、NumPy 或其他第三方库，使用 C++17 标准库实现，适合 Windows/Linux 主机离线处理。它不是板端 Runtime 服务，也不会修改原始 CSV 或项目配置。

## 构建与测试

```bash
cmake -S tools/compass_calibrator -B tools/compass_calibrator/build
cmake --build tools/compass_calibrator/build --config Debug
ctest --test-dir tools/compass_calibrator/build -C Debug --output-on-failure
```

当前 CTest 共 3 项：算法/边界单元测试、CLI help，以及固定球面 CSV 到候选配置文件的端到端 smoke。

## 输入格式

输入必须是 History Recorder 的磁力计 CSV，至少包含：

```text
magnetic_x_nt,magnetic_y_nt,magnetic_z_nt
```

默认至少要求 200 个样本和 8 个方向八分体全部覆盖。现场建议缓慢转动整机覆盖 roll、pitch、yaw 的完整姿态空间，并采集 1000 个以上样本；只在桌面水平旋转会形成近平面数据，工具会拒绝。

## 使用方法

```powershell
tools\compass_calibrator\build\Debug\compass_calibrator.exe `
  --input path\to\magnetometer.csv `
  --output path\to\compass_candidate.conf
```

如果已通过独立测量确定传感器坐标到机体坐标的旋转，可用 9 个按行排列、逗号分隔的值传入：

```powershell
--sensor-to-body "0,-1,0,1,0,0,0,0,1"
```

矩阵必须是行正交、行列式约为 `+1` 的 proper rotation。最终输出矩阵为：

```text
sensor_to_body_rotation * fitted_soft_iron_correction
```

磁力计随机姿态数据本身不能唯一确定安装旋转，因此工具不会从椭球拟合中猜测 sensor-to-body 方向；没有独立测量时保持默认单位阵。

## 算法与质量门槛

工具执行以下步骤：

1. 将 nT 输入转换为 μT，并以分量中位数和径向 MAD 剔除极端强磁离群点。
2. 对数据平移和尺度归一化，求解完整三维二次椭球最小二乘模型。
3. 从椭球中心得到硬铁偏置，通过对称 3×3 特征分解求正定平方根，得到包含交叉轴项的软铁校正。
4. 将校正矩阵行列式归一为 1，保留椭球三轴几何平均磁场尺度。
5. 按拟合残差再做一次稳健裁剪并重新拟合。
6. 检查样本数量、八分体覆盖、方向方差、椭球轴比、RMS 残差和最大残差。

默认质量门槛：

```text
minimum samples              200
minimum octants              8 / 8
minimum directional ratio    0.08
maximum ellipsoid axis ratio 5.0
maximum RMS residual         5%
maximum single residual      15%
```

退出码：

- `0`：拟合完成且质量门槛通过。
- `1`：数据可读取，但拟合失败或质量门槛未通过。
- `2`：参数、输入文件或输出文件错误，校准未执行。

## 输出安全边界

候选配置始终包含：

```text
compass_calibration_valid = false
```

椭球拟合通过不等于整机航向验收通过。必须另外完成八方向静态误差、不同倾角、重复性、动态稳定性和当地磁偏角验证，确认矩阵方向与安装一致后，才能在正式配置中人工改为 `true`。

工具不计算当地磁偏角，也不能区分固定设备磁干扰和环境临时磁干扰。采集时应远离钢制桌面、扬声器、大电流导线和磁性工具，并保存原始 CSV 作为复盘证据。
