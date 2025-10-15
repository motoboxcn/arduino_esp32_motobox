# PowerManager 重构说明

## 概述

本次重构完全简化了 ENABLE_SLEEP 功能，专注于核心的 IMU 运动检测和睡眠唤醒功能，移除了复杂的状态管理和不必要的功能。

## 主要变更

### 1. 简化状态枚举
```cpp
// 原来的复杂状态
enum PowerState {
    POWER_STATE_NORMAL,
    POWER_STATE_COUNTDOWN,        // 已移除
    POWER_STATE_PREPARING_SLEEP
};

// 现在只保留必要状态
enum PowerState {
    POWER_STATE_NORMAL,
    POWER_STATE_PREPARING_SLEEP
};
```

### 2. 核心功能保留
- ✅ IMU 运动检测
- ✅ IMU 中断唤醒 (GPIO34)
- ✅ 车辆电门检测 (GPIO16, 如果启用)
- ✅ 休眠时间设置和获取
- ✅ 深度睡眠进入和唤醒处理

### 3. 移除的复杂功能
- ❌ 10秒倒计时功能
- ❌ 屏幕亮度渐变
- ❌ 复杂的中断处理
- ❌ GPIO39 稳定性检查
- ❌ 音频提示

## 工作流程

### 正常运行
1. 系统启动后进入 `POWER_STATE_NORMAL` 状态
2. 每 200ms 检查一次运动状态和车辆状态
3. 如果检测到运动或车辆启动，重置 `lastMotionTime`
4. 如果设备静止时间超过设定的 `sleepTimeSec`，进入睡眠

### 睡眠流程
1. 设置状态为 `POWER_STATE_PREPARING_SLEEP`
2. 配置 IMU 为深度睡眠模式 (WakeOnMotion)
3. 配置 EXT0 唤醒源 (GPIO34)
4. 关闭外设 (WiFi, 蓝牙, 串口等)
5. 配置电源域
6. 进入深度睡眠

### 唤醒流程
1. 检测唤醒原因 (IMU运动 或 定时器)
2. 如果是 IMU 唤醒，恢复 IMU 正常模式
3. 重置 `lastMotionTime`
4. 返回 `POWER_STATE_NORMAL` 状态

## 配置参数

### 引脚定义 (esp32-air780eg)
```cpp
#define IMU_INT_PIN 34          // IMU 中断引脚
#define RTC_INT_PIN 16          // 车辆电门检测引脚 (可选)
```

### 默认设置
- 默认休眠时间: 300 秒 (5分钟)
- 运动检测间隔: 200ms
- 备用定时器唤醒: 1小时

## API 接口

### 公共方法
```cpp
void begin();                           // 初始化
void loop();                           // 主循环
void enterLowPowerMode();              // 进入低功耗模式
void setSleepTime(unsigned long seconds); // 设置休眠时间
unsigned long getSleepTime() const;    // 获取休眠时间
PowerState getPowerState();            // 获取当前状态
bool isSleepEnabled();                 // 检查是否启用睡眠
void printWakeupReason();              // 打印唤醒原因
```

## 编译配置

### 启用睡眠功能
```ini
build_flags = 
    -D ENABLE_SLEEP
    -D ENABLE_IMU
    -D IMU_INT_PIN=34
```

### 禁用睡眠功能
移除 `-D ENABLE_SLEEP` 编译标志即可。

## 优势

1. **简化代码**: 移除了复杂的状态机和倒计时逻辑
2. **更稳定**: 减少了潜在的错误点
3. **更高效**: 专注于核心功能，减少 CPU 开销
4. **易维护**: 代码结构清晰，逻辑简单
5. **兼容性好**: 保持了原有的 API 接口

## 测试建议

1. 测试正常的运动检测和睡眠进入
2. 测试 IMU 中断唤醒功能
3. 测试车辆电门检测 (如果启用)
4. 测试休眠时间设置和获取
5. 测试编译时禁用睡眠功能

## 注意事项

1. IMU_INT_PIN (GPIO34) 必须是有效的 RTC GPIO
2. 车辆电门检测是可选功能，通过 RTC_INT_PIN 配置
3. 睡眠时间通过 Preferences 持久化存储
4. 唤醒后会自动恢复 IMU 到正常工作模式
