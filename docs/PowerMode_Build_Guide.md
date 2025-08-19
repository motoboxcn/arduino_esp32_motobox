# 功耗模式管理编译指南

## 编译配置

### 1. 启用功耗模式管理

在 `src/config.h` 中确保以下配置已启用：

```cpp
// 功耗模式管理
#define ENABLE_POWER_MODE_MANAGEMENT
#define POWER_MODE_AUTO_SWITCH_ENABLED    true    // 默认启用自动模式切换
#define POWER_MODE_EVALUATION_INTERVAL    5000    // 模式评估间隔（毫秒）
#define POWER_MODE_MIN_SWITCH_INTERVAL    30000   // 最小模式切换间隔（毫秒）
```

### 2. 依赖模块

功耗模式管理需要以下模块支持：

```cpp
// 必需模块
#define ENABLE_IMU              // IMU运动检测
#define USE_AIR780EG_GSM        // Air780EG模块（GPS和MQTT）

// 可选模块
#define ENABLE_AUDIO            // 音频反馈
#define ENABLE_FUSION_LOCATION  // 融合定位
#define RTC_INT_PIN             // 车辆状态检测
```

### 3. PlatformIO配置

在 `platformio.ini` 中确保包含必要的构建标志：

```ini
[env:esp32-air780eg]
build_flags = 
    -D ENABLE_POWER_MODE_MANAGEMENT
    -D ENABLE_IMU
    -D ENABLE_AUDIO
    -D USE_AIR780EG_GSM
    # 其他必要标志...
```

## 编译步骤

### 1. 检查依赖

```bash
# 确保所有必要的库已安装
pio lib list
```

### 2. 编译项目

```bash
# 编译指定环境
pio run -e esp32-air780eg

# 编译并上传
pio run -e esp32-air780eg -t upload

# 编译并监视串口
pio run -e esp32-air780eg -t upload -t monitor
```

### 3. 验证功能

编译上传后，在串口监视器中输入：

```
power.help
```

如果看到功耗命令帮助信息，说明功能已正确编译和启用。

## 内存使用

### 估算内存占用

- PowerModeManager类: ~2KB RAM
- 模式配置数据: ~1KB RAM
- 串口命令处理: ~500B RAM
- 总计: ~3.5KB RAM

### 优化建议

如果内存紧张，可以：

1. 禁用不需要的模式：
```cpp
// 只保留基本和正常模式
#define POWER_MODE_COUNT 2
```

2. 减少配置参数：
```cpp
// 简化配置结构
struct SimplePowerModeConfig {
    unsigned long gps_interval_ms;
    unsigned long imu_update_interval_ms;
    unsigned long mqtt_interval_ms;
};
```

## 故障排除

### 编译错误

1. **未找到PowerModeManager.h**
   - 检查文件路径是否正确
   - 确保 `ENABLE_POWER_MODE_MANAGEMENT` 已定义

2. **链接错误**
   - 检查所有依赖模块是否已启用
   - 确保Air780EG库版本兼容

3. **内存不足**
   - 增加堆栈大小：`CONFIG_MAIN_TASK_STACK_SIZE=16384`
   - 优化其他模块的内存使用

### 运行时问题

1. **模式不切换**
   - 检查IMU是否正常初始化
   - 确认自动模式已启用
   - 查看串口日志

2. **MQTT间隔未改变**
   - 检查Air780EG连接状态
   - 确认MQTT任务重新创建成功

## 性能优化

### 1. 减少评估频率

```cpp
// 降低评估频率以节省CPU
#define POWER_MODE_EVALUATION_INTERVAL 10000  // 10秒
```

### 2. 优化模式切换条件

```cpp
// 调整运动检测阈值
#define MOTION_DETECTION_THRESHOLD 0.05  // 提高阈值减少误触发
```

### 3. 延迟初始化

```cpp
// 延迟初始化非关键功能
void PowerModeManager::begin() {
    // 延迟5秒初始化，等待系统稳定
    delay(5000);
    initializeDefaultConfigs();
}
```

## 调试配置

### 启用详细日志

```cpp
// 在config.h中添加
#define POWER_MODE_DEBUG_ENABLED true
#define POWER_MODE_VERBOSE_LOGGING true
```

### 调试宏

```cpp
#if POWER_MODE_DEBUG_ENABLED
#define POWER_DEBUG(x) Serial.println("[功耗调试] " + String(x))
#else
#define POWER_DEBUG(x)
#endif
```

## 版本兼容性

### Air780EG库版本

- 最低版本: v1.2.0
- 推荐版本: v1.2.1+
- 测试版本: v1.2.1

### ESP32核心版本

- 最低版本: 2.0.0
- 推荐版本: 2.0.11+
- 测试版本: 2.0.11

### Arduino IDE版本

- 最低版本: 1.8.19
- 推荐版本: 2.0.0+
- 测试版本: 2.2.1