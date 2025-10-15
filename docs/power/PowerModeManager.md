# 功耗模式管理系统

## 概述

功耗模式管理系统为ESP32-S3 MotoBox提供了智能的电池功耗分级自动调节功能。系统支持4种功耗模式，可以根据车辆状态和运动检测自动切换，也支持手动控制。

## 功耗模式

### 1. 休眠模式 (POWER_MODE_SLEEP)
- **适用场景**: 长时间静止，无运动检测
- **功耗特点**: 最低功耗，深度睡眠
- **配置特点**:
  - 所有传感器停止工作
  - 网络连接断开
  - 仅保留运动唤醒功能
  - 功耗: 10-100μA

### 2. 基本模式 (POWER_MODE_BASIC)
- **适用场景**: 短暂停车，等红灯，临时停车
- **功耗特点**: 低功耗运行，保持基础功能
- **配置特点**:
  - GPS定位: 5秒间隔
  - GPS记录: 10秒间隔
  - IMU更新: 200ms间隔 (5Hz)
  - MQTT位置上报: 15秒间隔
  - MQTT状态上报: 60秒间隔
  - 融合定位: 500ms间隔
  - 空闲超时: 2分钟后进入休眠
  - 关闭音频反馈省电

### 3. 正常模式 (POWER_MODE_NORMAL)
- **适用场景**: 正常行驶，标准使用
- **功耗特点**: 平衡性能和功耗
- **配置特点**:
  - GPS定位: 1秒间隔
  - GPS记录: 5秒间隔
  - IMU更新: 100ms间隔 (10Hz)
  - MQTT位置上报: 5秒间隔
  - MQTT状态上报: 30秒间隔
  - 融合定位: 100ms间隔
  - 空闲超时: 5分钟后进入基本模式
  - 启用音频反馈

### 4. 运动模式 (POWER_MODE_SPORT)
- **适用场景**: 高速运动，测试零百加速，需要高精度数据
- **功耗特点**: 最高性能，最高功耗
- **配置特点**:
  - GPS定位: 1秒间隔（最高频率）
  - GPS记录: 1秒间隔（高精度记录）
  - IMU更新: 50ms间隔 (20Hz)
  - IMU高精度模式: 启用
  - MQTT位置上报: 2秒间隔
  - MQTT状态上报: 15秒间隔
  - 融合定位: 50ms间隔（高频）
  - 空闲超时: 1分钟后进入正常模式
  - 启用音频反馈

## 自动模式切换逻辑

### 切换条件

1. **进入运动模式**:
   - 检测到高速运动（GPS速度 > 30km/h）
   - 检测到剧烈运动（加速度 > 1.5g）
   - 检测到精细操作（角速度 > 100°/s）

2. **进入正常模式**:
   - 检测到运动但不满足运动模式条件
   - 车辆启动状态

3. **进入基本模式**:
   - 无运动检测超过配置的空闲时间
   - 从正常/运动模式降级

4. **进入休眠模式**:
   - 基本模式下无运动超过空闲时间
   - 长时间静止

### 防抖机制

- 最小模式切换间隔: 30秒
- 运动停止确认时间: 10秒
- 模式评估间隔: 5秒

## 使用方法

### 编程接口

```cpp
#include "power/PowerModeManager.h"

// 初始化
powerModeManager.begin();

// 手动设置模式
powerModeManager.setMode(POWER_MODE_SPORT);

// 启用/禁用自动模式切换
powerModeManager.enableAutoModeSwitch(true);

// 获取当前模式
PowerMode currentMode = powerModeManager.getCurrentMode();

// 获取当前配置
const PowerModeConfig& config = powerModeManager.getCurrentConfig();

// 打印状态
powerModeManager.printCurrentStatus();
```

### 串口命令

```bash
# 查看功耗模式状态
power.status

# 手动切换模式
power.basic    # 切换到基本模式
power.normal   # 切换到正常模式
power.sport    # 切换到运动模式

# 自动模式控制
power.auto.on  # 启用自动模式切换
power.auto.off # 禁用自动模式切换
power.eval     # 手动触发模式评估

# 查看配置
power.config   # 显示所有模式配置
power.help     # 显示帮助信息
```

## 配置定制

### 自定义模式配置

```cpp
// 创建自定义配置
PowerModeConfig customConfig = {
    .gps_interval_ms = 2000,              // 2秒GPS间隔
    .gps_log_interval_ms = 5000,          // 5秒记录间隔
    .imu_update_interval_ms = 100,        // 100ms IMU更新
    .imu_high_precision = false,          // 标准精度
    .mqtt_location_interval_ms = 10000,   // 10秒MQTT上报
    .mqtt_device_status_interval_ms = 30000,
    .fusion_update_interval_ms = 200,     // 200ms融合定位
    .system_check_interval_ms = 1000,
    .enable_audio_feedback = true,
    .idle_timeout_ms = 180000,            // 3分钟空闲超时
    .enable_peripheral_power_save = false
};

// 应用自定义配置
powerModeManager.setCustomConfig(POWER_MODE_NORMAL, customConfig);

// 重置为默认配置
powerModeManager.resetToDefaultConfig(POWER_MODE_NORMAL);
```

## 功耗优化效果

| 模式 | GPS频率 | IMU频率 | MQTT频率 | 预估功耗 | 适用场景 |
|------|---------|---------|----------|----------|----------|
| 休眠模式 | 关闭 | 关闭 | 关闭 | 10-100μA | 长时间停车 |
| 基本模式 | 5s | 5Hz | 15s | 50-100mA | 短暂停车 |
| 正常模式 | 1s | 10Hz | 5s | 150-250mA | 正常行驶 |
| 运动模式 | 1s | 20Hz | 2s | 250-400mA | 高速/测试 |

## 注意事项

1. **模式切换音效**: 每种模式切换时会播放不同音调的提示音
2. **MQTT任务管理**: 由于Air780EG库限制，MQTT间隔调整通过重新创建任务实现
3. **IMU高精度模式**: 运动模式下自动启用，提供更高采样率和精度
4. **防频繁切换**: 内置防抖机制避免模式频繁切换
5. **状态持久化**: 当前模式状态保存在device_state中

## 故障排除

### 常见问题

1. **模式不自动切换**
   - 检查自动模式是否启用: `power.auto.on`
   - 检查IMU是否正常工作
   - 查看运动检测状态: `power.status`

2. **功耗仍然较高**
   - 确认当前模式: `power.status`
   - 检查外设是否正确关闭
   - 查看模式配置: `power.config`

3. **GPS/MQTT频率未改变**
   - 检查Air780EG模块状态
   - 确认MQTT任务是否重新创建成功
   - 查看串口日志确认配置应用

### 调试方法

```cpp
// 启用调试输出
powerModeManager.printCurrentStatus();
powerModeManager.printModeConfigs();

// 手动触发模式评估
powerModeManager.evaluateAndSwitchMode();
```

## 扩展开发

### 添加新的模式切换条件

```cpp
// 在PowerModeManager::evaluateOptimalMode()中添加自定义逻辑
bool PowerModeManager::detectCustomCondition() {
    // 自定义检测逻辑
    return false;
}
```

### 自定义音效

```cpp
// 在PowerModeManager::playModeChangeSound()中添加
void PowerModeManager::playModeChangeSound(PowerMode newMode) {
    #ifdef ENABLE_AUDIO
    switch (newMode) {
        case POWER_MODE_CUSTOM:
            audioManager.playCustomBeep(1500, 500); // 自定义音效
            break;
    }
    #endif
}
```