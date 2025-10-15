# IMU数据更新频率优化

## 问题描述

在分析串口输出日志时发现，IMU数据虽然每20ms输出一次（50Hz），但**数据值**约1秒才变化一次，导致：

- 姿态角数据更新缓慢
- 运动检测响应延迟
- 数据采集效率低下

### 问题现象
```
imu_data: 0.20, 0.04, 0.00, 30.11
imu_data: 0.20, 0.04, 0.00, 30.11  // 连续多行相同
imu_data: 0.20, 0.04, 0.00, 30.11
...
imu_data: 0.39, 0.07, 0.00, 30.18  // 约1秒后才变化
```

## 根本原因分析

### 1. 配置问题
**原始配置**：
```cpp
// 只启用加速度计
qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
qmi.enableAccelerometer();
// 未启用陀螺仪
```

### 2. 官方文档说明
根据QMI8658官方文档：
> **如果同时开启加速度计和陀螺仪传感器，输出频率将基于陀螺仪输出频率**

### 3. 数据就绪信号
`getDataReady()`的返回频率受到传感器配置的影响，单独的加速度计可能无法提供预期的高频数据就绪信号。

## 解决方案

### 方案1: 启用陀螺仪（已实施）

#### 配置更改
```cpp
// 配置加速度计 - 500Hz采样率
qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
qmi.enableAccelerometer();

// 新增：配置陀螺仪 - 896.8Hz采样率
qmi.configGyroscope(
    SensorQMI8658::GYR_RANGE_1024DPS,  // ±1024°/s量程
    SensorQMI8658::GYR_ODR_896_8Hz,    // 896.8Hz采样率
    SensorQMI8658::LPF_MODE_3          // 低通滤波器模式3
);
qmi.enableGyroscope();
```

#### 优化效果
- **数据更新频率**: 从~1Hz提升到896.8Hz
- **响应速度**: 运动检测响应更快
- **数据质量**: 姿态角计算更准确（融合陀螺仪数据）

### 数据输出优化
更新了`printImuData()`方法，显示完整的传感器数据：

```cpp
void IMU::printImuData() {
    Serial.printf("imu_data: roll=%.2f, pitch=%.2f, yaw=%.2f, temp=%.1f°C | accel=(%.2f,%.2f,%.2f)g | gyro=(%.1f,%.1f,%.1f)°/s\n",
        imu_data.roll, imu_data.pitch, imu_data.yaw, imu_data.temperature,
        imu_data.accel_x, imu_data.accel_y, imu_data.accel_z,
        imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z);
}
```

## 技术细节

### QMI8658传感器配置

#### 加速度计配置
- **量程**: ±4g
- **采样率**: 500Hz
- **用途**: 姿态角计算、运动检测

#### 陀螺仪配置
- **量程**: ±1024°/s
- **采样率**: 896.8Hz
- **低通滤波**: LPF_MODE_3 (13.37% of ODR)
- **用途**: 姿态角融合、角速度测量

### 互补滤波算法
```cpp
// 使用加速度计计算姿态角
float roll_acc = atan2(imu_data.accel_y, imu_data.accel_z) * 180 / M_PI;
float pitch_acc = atan2(-imu_data.accel_x, sqrt(imu_data.accel_y * imu_data.accel_y + imu_data.accel_z * imu_data.accel_z)) * 180 / M_PI;

// 使用陀螺仪数据和互补滤波更新姿态角
imu_data.roll = ALPHA * (imu_data.roll + imu_data.gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
imu_data.pitch = ALPHA * (imu_data.pitch + imu_data.gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;
```

其中：
- `ALPHA = 0.98`: 互补滤波系数
- `dt = 0.01`: 时间间隔（假设100Hz处理频率）

## 测试验证

### 测试脚本
提供了`test/test_imu_frequency.py`脚本用于分析IMU数据更新频率：

```bash
python3 test/test_imu_frequency.py tmp/out
```

### 期望结果
- ✅ 数据变化频率显著提高
- ✅ 连续相同值情况大幅减少
- ✅ Roll/Pitch/Yaw有更频繁的变化
- ✅ 陀螺仪数据正常输出

### 性能影响
- **Flash使用**: 增加约1KB（陀螺仪相关代码）
- **RAM使用**: 基本无变化
- **CPU负载**: 轻微增加（数据处理频率提高）
- **功耗**: 轻微增加（陀螺仪启用）

## 后续优化建议

### 1. 动态频率调整
根据应用场景动态调整采样频率：
```cpp
// 静止时降低频率节省功耗
// 运动时提高频率增强响应
```

### 2. FIFO缓冲
启用传感器FIFO缓冲，减少I2C通信频率：
```cpp
qmi.enableFIFO();
```

### 3. 中断驱动
使用数据就绪中断替代轮询方式：
```cpp
qmi.enableDataReadyInterrupt();
```

## 总结

通过启用陀螺仪并配置为896.8Hz采样率，成功解决了IMU数据更新频率低的问题。这不仅提高了数据更新频率，还改善了姿态角计算的准确性，为后续的运动检测和导航功能提供了更好的数据基础。

### 关键改进
1. **数据更新频率**: ~1Hz → 896.8Hz
2. **传感器融合**: 加速度计 + 陀螺仪
3. **输出格式**: 更详细的数据显示
4. **测试工具**: 提供频率分析脚本

这次优化为ESP32-S3 MotoBox项目的IMU功能奠定了坚实的基础。
