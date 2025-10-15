# LED调试系统使用指南

## 概述

LED调试系统为ESP32-S3 MotoBox项目提供了完整的LED功能调试支持，帮助开发者快速定位LED相关问题，优化LED显示效果。

## 功能特点

### ✨ 主要特性
- **编译时控制**: 通过宏定义完全控制调试输出，零性能影响
- **分级调试**: 支持普通、警告、错误等不同级别的调试信息
- **时间戳支持**: 可选的毫秒级时间戳，便于性能分析
- **频率限制**: 避免高频调试输出影响系统性能
- **状态跟踪**: 自动记录LED状态变化和函数调用
- **格式化输出**: 统一的调试信息格式，便于日志分析

### 🎯 适用场景
- LED初始化问题排查
- 充电状态显示调试
- 呼吸效果参数调优
- PWM LED颜色校准
- 性能瓶颈分析
- 功能集成测试

## 快速开始

### 1. 启用调试功能

在 `src/config.h` 中取消注释以下行：

```cpp
#define LED_DEBUG_ENABLED  // 启用LED调试输出
```

### 2. 编译并上传固件

```bash
pio run --environment esp32-air780eg
pio run --target upload --environment esp32-air780eg
```

### 3. 查看调试输出

打开串口监视器，波特率设置为115200，您将看到类似输出：

```
[LED_DEBUG] LEDManager 构造函数调用
[LED_DEBUG] 进入函数: LEDManager::begin
[LED_DEBUG] 初始化 PWM LED
[LED_DEBUG] PWM LED 初始化完成 - 模式: LED_ON, 颜色: GREEN, 亮度: 5
```

## 调试宏参考

### 基础调试宏

| 宏名 | 功能 | 示例 |
|------|------|------|
| `LED_DEBUG_PRINTF(fmt, ...)` | 基本格式化输出 | `LED_DEBUG_PRINTF("亮度: %d\n", brightness)` |
| `LED_DEBUG_PRINTLN(x)` | 单行输出 | `LED_DEBUG_PRINTLN("LED初始化完成")` |
| `LED_DEBUG_PRINT(x)` | 不换行输出 | `LED_DEBUG_PRINT("状态: ")` |

### 高级调试宏

| 宏名 | 功能 | 示例 |
|------|------|------|
| `LED_DEBUG_TIMESTAMP_PRINTF(fmt, ...)` | 带时间戳输出 | `LED_DEBUG_TIMESTAMP_PRINTF("状态更新\n")` |
| `LED_DEBUG_THROTTLED(interval, fmt, ...)` | 限频输出 | `LED_DEBUG_THROTTLED(5000, "循环状态\n")` |
| `LED_DEBUG_STATE_CHANGE(old, new, desc)` | 状态变化记录 | `LED_DEBUG_STATE_CHANGE("OFF", "ON", "LED模式")` |

### 函数跟踪宏

| 宏名 | 功能 | 示例 |
|------|------|------|
| `LED_DEBUG_ENTER(func)` | 函数进入 | `LED_DEBUG_ENTER("setLEDState")` |
| `LED_DEBUG_EXIT(func)` | 函数退出 | `LED_DEBUG_EXIT("setLEDState")` |

### 数值调试宏

| 宏名 | 功能 | 示例 |
|------|------|------|
| `LED_DEBUG_VALUE(name, value)` | 整数值输出 | `LED_DEBUG_VALUE("brightness", 50)` |
| `LED_DEBUG_VALUE_F(name, value)` | 浮点值输出 | `LED_DEBUG_VALUE_F("voltage", 3.3)` |

### 错误调试宏

| 宏名 | 功能 | 示例 |
|------|------|------|
| `LED_DEBUG_ERROR(fmt, ...)` | 错误信息 | `LED_DEBUG_ERROR("初始化失败\n")` |
| `LED_DEBUG_WARNING(fmt, ...)` | 警告信息 | `LED_DEBUG_WARNING("亮度超限\n")` |

## 调试输出示例

### LED初始化过程

```
[LED_DEBUG] LEDManager 构造函数调用
[LED_DEBUG] 进入函数: LEDManager::begin
[LED_DEBUG] 初始化 PWM LED
[LED_DEBUG] 初始化PWM LED，引脚: 48
[LED_DEBUG] PWM LED 初始化完成，引脚: 48
[LED_DEBUG] PWM LED 初始化完成 - 模式: LED_ON, 颜色: GREEN, 亮度: 5
[LED_DEBUG] 启用自动充电显示模式
[LED_DEBUG] 退出函数: LEDManager::begin
```

### 充电状态变化

```
[LED_DEBUG] 进入函数: LEDManager::setChargingDisplay
[LED_DEBUG] 设置充电显示状态: 充电中
[LED_DEBUG] 充电中 -> 设置为绿灯呼吸效果
[LED_DEBUG] 状态变化 [充电显示模式]: LED_ON -> LED_BREATH
[LED_DEBUG] 充电显示亮度变化: 5 -> 50
[LED_DEBUG] 更新LED硬件 - 模式: LED_BREATH, 颜色: GREEN, 亮度: 50
[LED_DEBUG] 退出函数: LEDManager::setChargingDisplay
```

### 呼吸效果运行

```
[LED_DEBUG][15234] 呼吸效果更新: 当前值=25, 目标=50, 递增=是
[LED_DEBUG][15284] 显示LED: 模式=LED_BREATH, 颜色=GREEN, 设定亮度=50, 实际亮度=25, RGB=(0,255,0)
[LED_DEBUG][15334] 呼吸效果更新: 当前值=30, 目标=50, 递增=是
[LED_DEBUG][15384] 显示LED: 模式=LED_BREATH, 颜色=GREEN, 设定亮度=50, 实际亮度=30, RGB=(0,255,0)
```

### 状态监控（限频输出）

```
[LED_DEBUG][20000] 状态监控 - 充电: 是, 自动模式: 启用, LED模式: LED_BREATH, 颜色: GREEN, 亮度: 50
[LED_DEBUG][30000] 状态监控 - 充电: 是, 自动模式: 启用, LED模式: LED_BREATH, 颜色: GREEN, 亮度: 50
[LED_DEBUG][40000] 状态监控 - 充电: 否, 自动模式: 启用, LED模式: LED_ON, 颜色: GREEN, 亮度: 5
```

## 性能影响分析

### 启用调试时
- **Flash占用**: 增加约2-3KB（字符串常量）
- **RAM占用**: 增加约100-200字节（调试缓冲区）
- **CPU占用**: 调试输出时约1-2%（取决于输出频率）
- **串口带宽**: 约1-5KB/s（取决于调试级别）

### 禁用调试时
- **Flash占用**: 0字节（编译器完全优化掉）
- **RAM占用**: 0字节
- **CPU占用**: 0%
- **串口带宽**: 0

## 最佳实践

### 🎯 开发阶段
```cpp
// 启用详细调试
#define LED_DEBUG_ENABLED

// 在关键函数中添加跟踪
void criticalFunction() {
    LED_DEBUG_ENTER("criticalFunction");
    
    // 记录重要状态变化
    LED_DEBUG_STATE_CHANGE(oldState, newState, "关键状态");
    
    // 记录重要数值
    LED_DEBUG_VALUE("important_value", value);
    
    LED_DEBUG_EXIT("criticalFunction");
}
```

### 🚀 生产阶段
```cpp
// 禁用调试以节省资源
// #define LED_DEBUG_ENABLED

// 保留错误和警告输出（可选）
#define LED_ERROR_ONLY
```

### 📊 性能调优
```cpp
// 使用限频输出避免性能影响
LED_DEBUG_THROTTLED(1000, "高频状态: %d\n", state);

// 使用条件调试
#ifdef DETAILED_DEBUG
    LED_DEBUG_PRINTF("详细信息: %s\n", details);
#endif
```

## 故障排除

### 常见问题

#### 1. 调试信息不显示
**原因**: LED_DEBUG_ENABLED未定义
**解决**: 检查config.h中的宏定义

#### 2. 调试信息过多影响性能
**原因**: 高频调试输出
**解决**: 使用LED_DEBUG_THROTTLED限制频率

#### 3. 编译错误
**原因**: 调试宏使用不当
**解决**: 检查参数格式和包含头文件

### 调试技巧

#### 1. 分层调试
```cpp
// 第一层：基本功能
LED_DEBUG_PRINTF("基本状态: %d\n", state);

// 第二层：详细信息
#ifdef DETAILED_LED_DEBUG
    LED_DEBUG_PRINTF("详细参数: r=%d, g=%d, b=%d\n", r, g, b);
#endif

// 第三层：性能分析
#ifdef PERFORMANCE_LED_DEBUG
    LED_DEBUG_TIMESTAMP_PRINTF("性能点: %s\n", checkpoint);
#endif
```

#### 2. 条件编译
```cpp
#if defined(LED_DEBUG_ENABLED) && defined(CHARGING_DEBUG)
    LED_DEBUG_PRINTF("充电调试: %s\n", status);
#endif
```

## 工具支持

### 自动化测试脚本

使用提供的测试脚本快速切换调试模式：

```bash
# 启用调试并编译
python3 test/test_led_debug.py enable

# 禁用调试并编译
python3 test/test_led_debug.py disable

# 查看调试输出示例
python3 test/test_led_debug.py examples

# 查看可用调试宏
python3 test/test_led_debug.py macros
```

### 日志分析

推荐使用以下工具分析调试日志：
- **串口工具**: PuTTY, minicom, Arduino IDE串口监视器
- **日志过滤**: grep, awk等命令行工具
- **可视化**: 自定义Python脚本进行数据可视化

## 扩展开发

### 添加新的调试宏

在`LEDDebug.h`中添加自定义调试宏：

```cpp
#ifdef LED_DEBUG_ENABLED
    #define LED_DEBUG_CUSTOM(fmt, ...) \
        Serial.printf("[LED_CUSTOM][%lu] " fmt, millis(), ##__VA_ARGS__)
#else
    #define LED_DEBUG_CUSTOM(fmt, ...)
#endif
```

### 集成到其他模块

```cpp
#include "led/LEDDebug.h"

void otherModuleFunction() {
    LED_DEBUG_PRINTF("其他模块调用LED功能\n");
    // ... LED相关操作
}
```

## 总结

LED调试系统提供了完整的调试支持，帮助开发者：
- 🔍 快速定位LED相关问题
- 📊 分析LED性能表现
- 🎯 优化LED显示效果
- 🚀 确保生产环境性能

通过合理使用调试宏和遵循最佳实践，可以显著提高LED功能的开发效率和代码质量。
