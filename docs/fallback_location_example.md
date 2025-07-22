# FusionLocationManager 兜底定位功能使用指南

## 功能概述

FusionLocationManager 现在支持 LBS（基站定位）和 WiFi 定位作为 GNSS 的兜底方案，确保在 GNSS 信号丢失时仍能获得位置信息。

## 主要特性

### 1. 智能兜底策略
- **自动检测**: 自动检测 GNSS 信号丢失
- **双重备份**: 支持 LBS 和 WiFi 两种兜底定位方式
- **优先级配置**: 可配置优先使用 WiFi 或 LBS 定位
- **间隔控制**: 防止频繁请求造成阻塞

### 2. 防阻塞设计
- **时间间隔**: LBS 和 WiFi 定位都有独立的时间间隔控制
- **状态标志**: 防止重复请求导致的阻塞
- **超时检测**: 基于时间的 GNSS 信号丢失检测

### 3. 统计监控
- **定位次数统计**: 记录各种定位方式的使用次数
- **来源标识**: 清楚显示当前位置的来源（GNSS/WiFi/LBS）
- **状态监控**: 实时显示兜底定位的工作状态

## 使用方法

### 1. 基本配置

```cpp
#include "location/FusionLocationManager.h"

void setup() {
    // 初始化融合定位系统
    fusionLocationManager.begin(FUSION_EKF_VEHICLE, 39.9042, 116.4074);
    
    // 配置兜底定位
    fusionLocationManager.configureFallbackLocation(
        true,      // 启用兜底定位
        30000,     // GNSS信号丢失超时时间 30秒
        300000,    // LBS定位间隔 5分钟
        180000,    // WiFi定位间隔 3分钟
        true       // 优先使用WiFi定位
    );
    
    // 启用调试输出
    fusionLocationManager.setDebug(true);
}

void loop() {
    // 必须在主循环中调用
    fusionLocationManager.loop();
    
    // 获取位置信息
    Position pos = fusionLocationManager.getFusedPosition();
    if (pos.valid) {
        String source = fusionLocationManager.getLocationSource();
        Serial.printf("位置: %.6f, %.6f [来源: %s]\n", 
                     pos.lat, pos.lng, source.c_str());
    }
    
    delay(1000);
}
```

### 2. 手动触发定位

```cpp
// 手动触发 LBS 定位
if (fusionLocationManager.requestLBSLocation()) {
    Serial.println("LBS定位请求已发送");
} else {
    Serial.println("LBS定位请求失败");
}

// 手动触发 WiFi 定位
if (fusionLocationManager.requestWiFiLocation()) {
    Serial.println("WiFi定位请求已发送");
} else {
    Serial.println("WiFi定位请求失败");
}
```

### 3. 状态监控

```cpp
// 打印系统状态
fusionLocationManager.printStatus();

// 打印统计信息
fusionLocationManager.printStats();

// 获取当前定位来源
String source = fusionLocationManager.getLocationSource();
Serial.println("当前定位来源: " + source);
```

## 配置参数说明

### configureFallbackLocation 参数

| 参数 | 类型 | 默认值 | 说明 |
|------|------|--------|------|
| enable | bool | true | 是否启用兜底定位 |
| gnss_timeout | unsigned long | 30000 | GNSS信号丢失超时时间(ms) |
| lbs_interval | unsigned long | 300000 | LBS定位间隔(ms) |
| wifi_interval | unsigned long | 180000 | WiFi定位间隔(ms) |
| prefer_wifi | bool | true | 是否优先使用WiFi定位 |

### 推荐配置

#### 城市环境（WiFi热点多）
```cpp
fusionLocationManager.configureFallbackLocation(
    true,      // 启用
    20000,     // 20秒超时
    600000,    // LBS 10分钟间隔
    120000,    // WiFi 2分钟间隔
    true       // 优先WiFi
);
```

#### 郊区环境（基站覆盖好）
```cpp
fusionLocationManager.configureFallbackLocation(
    true,      // 启用
    30000,     // 30秒超时
    180000,    // LBS 3分钟间隔
    600000,    // WiFi 10分钟间隔
    false      // 优先LBS
);
```

#### 省电模式
```cpp
fusionLocationManager.configureFallbackLocation(
    true,      // 启用
    60000,     // 60秒超时
    900000,    // LBS 15分钟间隔
    900000,    // WiFi 15分钟间隔
    true       // 优先WiFi
);
```

## 工作原理

### 1. GNSS信号检测
系统通过以下条件判断GNSS信号是否丢失：
- GNSS数据无效 (`data_valid = false`)
- GNSS未定位 (`is_fixed = false`)
- 数据超时（超过配置的超时时间）
- 定位类型不是"GNSS"

### 2. 兜底定位策略
当检测到GNSS信号丢失时：

**优先WiFi模式**:
1. 检查WiFi定位间隔，如果到时间则尝试WiFi定位
2. WiFi定位失败或未到间隔时间，检查LBS定位间隔
3. 如果LBS间隔到时间，则尝试LBS定位

**优先LBS模式**:
1. 检查LBS定位间隔，如果到时间则尝试LBS定位
2. LBS定位失败或未到间隔时间，检查WiFi定位间隔
3. 如果WiFi间隔到时间，则尝试WiFi定位

### 3. 防阻塞机制
- 使用进行中标志防止重复请求
- 独立的时间间隔控制避免频繁请求
- 非阻塞的状态检查和更新

## 注意事项

### 1. 性能考虑
- LBS和WiFi定位可能需要30秒时间，会阻塞串口通信
- 建议设置合理的间隔时间，避免频繁请求
- 在关键任务期间可以临时禁用兜底定位

### 2. 精度差异
- GNSS定位精度: 通常3-5米
- WiFi定位精度: 通常10-50米
- LBS定位精度: 通常50-1000米

### 3. 网络依赖
- LBS和WiFi定位都需要网络连接
- 确保Air780EG模块已连接到移动网络
- 在网络信号差的地区效果可能不佳

### 4. 功耗影响
- 兜底定位会增加功耗
- 可以根据应用场景调整间隔时间
- 在低功耗模式下可以禁用兜底定位

## 调试信息

启用调试后，系统会输出详细的工作状态：

```
[FusionLocation] GNSS信号丢失，启动兜底定位策略
[FusionLocation] 尝试WiFi定位...
[FusionLocation] WiFi定位成功
[GPS] GPS数据: 位置(39.904200,116.407400) 高度0.0m 精度0.0m 卫星0 来源:WIFI
```

## 示例输出

### 状态显示
```
=== 融合定位系统状态 ===
算法: EKF车辆模型
位置: 39.904200, 116.407400 (精度: 25.0m) [来源: WIFI]
航向: 45.0° | 速度: 0.0m/s | 高度: 0.0m
数据源: IMU GPS MAG
有效性: 有效 | 初始化: 是
兜底定位: 启用 | GNSS信号: 丢失
```

### 统计信息
```
=== 融合定位统计信息 ===
总更新次数: 1250
融合更新: 1200 | GPS: 800 | IMU: 1250 | MAG: 1100
兜底定位: LBS: 5 | WiFi: 12
成功率: 96.0%
=== 兜底定位配置 ===
GNSS超时: 30s | LBS间隔: 300s | WiFi间隔: 180s
优先WiFi: 是 | 当前定位源: WIFI
```

这个兜底定位功能确保了在GNSS信号不佳的环境下（如隧道、地下车库、高楼密集区域）仍能获得位置信息，提高了系统的可靠性和实用性。
