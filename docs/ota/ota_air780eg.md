# Air780EG原生MQTT OTA升级方案

## 方案特点

### ✅ 完全基于Air780EG
- 使用Air780EG原生AT指令实现HTTP下载
- 移除WiFi依赖，符合轻量化设计
- 通过4G网络进行固件下载

### ✅ 最小化实现
- 核心代码<200行
- 内存占用<2KB
- 无冗余功能

## 架构设计

```
MQTT服务器 ←→ Air780EG 4G ←→ ESP32 OTA分区
     ↓           ↓              ↓
  版本检查    HTTP下载      固件安装
```

## 使用方法

### 1. 初始化
```cpp
#include "ota/OTAManager.h"

void setup() {
    // 初始化Air780EG
    air780eg.begin(&Serial2, 115200, 19, 18, 5);
    
    // 初始化OTA
    otaManager.begin(&air780eg);
    otaManager.setMQTTPublishCallback(mqttPublish);
}
```

### 2. MQTT消息处理
```cpp
void onMqttMessage(String topic, String payload) {
    if (topic.indexOf("/ota/") > 0) {
        otaManager.handleMQTTMessage(topic, payload);
    }
}
```

### 3. 定期检查更新
```cpp
void loop() {
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > 3600000) { // 1小时检查一次
        otaManager.checkForUpdates();
        lastCheck = millis();
    }
}
```

## MQTT消息格式

### 版本检查请求
```json
{
    "device_id": "ESP32_ABCD1234",
    "current_version": "v4.2.0",
    "hardware_version": "esp32-air780eg",
    "timestamp": 1642678800
}
```

### 服务器响应
```json
{
    "latest_version": "v4.3.0",
    "download_url": "https://server.com/firmware/v4.3.0.bin",
    "force_update": false
}
```

### 状态上报
```json
{
    "device_id": "ESP32_ABCD1234",
    "status": "downloading|success|failed",
    "progress": 75,
    "message": "下载进度75%",
    "timestamp": 1642678800
}
```

## 升级条件

- 电池电量≥90%或正在充电
- 可用Flash空间≥500KB
- 4G网络连接正常
- 新版本号高于当前版本

## 优势

1. **轻量化**: 移除WiFi/HTTP库依赖
2. **稳定性**: 基于Air780EG成熟AT指令
3. **低功耗**: 4G模块统一管理网络
4. **简单**: 核心逻辑清晰，易于维护

## 编译验证

```bash
cd /Users/mikas/daboluo/arduino_esp32_motobox
pio run --environment esp32-air780eg
```

预期Flash使用率降低到75%以下。
