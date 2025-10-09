# ESP32-S3 MotoBox MQTT在线OTA升级系统

## 功能概述

本OTA升级系统通过**MQTT在线升级**方式，通过4G网络接收服务器推送的固件更新，实现远程固件升级功能。

## 升级条件

系统会自动检查以下升级条件：
- ✅ **电池电量**：必须≥90%才能进行升级
- ✅ **版本检查**：新版本号必须高于当前版本
- ✅ **存储空间**：有足够的Flash空间进行升级
- ✅ **网络连接**：4G网络连接正常

## MQTT在线升级

### 1. MQTT主题结构
```
device/{device_id}/ota/check      # 版本检查请求/响应
device/{device_id}/ota/download   # 下载指令
device/{device_id}/ota/status     # 状态上报
```

### 2. 版本检查消息格式

**设备发送（请求）：**
```json
{
    "device_id": "ESP32_ABCD1234",
    "current_version": "v4.0.0",
    "hardware_version": "esp32-air780eg",
    "timestamp": 1642678800
}
```

**服务器响应：**
```json
{
    "latest_version": "v4.1.0",
    "download_url": "https://server.com/firmware/v4.1.0.bin",
    "force_update": false,
    "release_notes": "修复蓝牙连接问题"
}
```

### 3. 状态上报格式
```json
{
    "device_id": "ESP32_ABCD1234",
    "status": "downloading|success|failed|conditions_not_met",
    "progress": 75,
    "message": "下载进度75%",
    "timestamp": 1642678800,
    "version": "v4.0.0"
}
```

### 4. 升级流程

1. **版本检查**：设备定期或手动发送版本检查请求
2. **条件验证**：检查电池电量、版本号、存储空间
3. **开始下载**：通过HTTP/HTTPS下载固件文件
4. **实时进度**：每20%上报一次下载进度
5. **安装升级**：下载完成后自动安装
6. **自动重启**：升级成功后自动重启到新版本

### 5. 手动触发升级

通过串口命令或MQTT消息触发：
```cpp
// 代码中调用
otaManager.checkForUpdates();
```

## 语音/提示音功能

系统支持蜂鸣器提示音（需要定义`BUZZER_PIN`）：

| 提示音 | 类型 | 含义 |
|--------|------|------|
| 🔊 3声短促音 | type=1 | 开始升级 |
| 🔊 2声中等音 | type=2 | 升级进行中 |
| 🔊 上升音调 | type=3 | 升级成功 |
| 🔊 长声低音 | type=4 | 升级失败 |
| 🔊 单声短促 | type=5 | 进度提示 |

## 安全特性

1. **固件签名验证**：支持MD5/SHA256校验
2. **断点续传**：网络中断时可继续下载
3. **回滚保护**：升级失败时自动回滚
4. **电量保护**：低电量时拒绝升级
5. **分片下载**：适应网络不稳定环境

## 配置参数

在 `config.h` 中可调整以下参数：
```cpp
#define OTA_BATTERY_MIN_LEVEL    90      // 最低电池电量要求(%)
#define OTA_CHECK_INTERVAL       3600000 // 检查间隔(1小时)
#define OTA_DOWNLOAD_TIMEOUT     300000  // 下载超时(5分钟)
#define OTA_MAX_RETRY_COUNT      3       // 最大重试次数
```

## 使用示例

### 代码集成
```cpp
#include "ota/OTAManager.h"

void setup() {
    // 初始化OTA管理器
    otaManager.begin();
    
    // 设置MQTT回调
    otaManager.setMQTTPublishCallback(mqttPublishCallback);
}

void loop() {
    // 定期检查更新（可选）
    static unsigned long lastCheck = 0;
    if (millis() - lastCheck > OTA_CHECK_INTERVAL) {
        otaManager.checkForUpdates();
        lastCheck = millis();
    }
}

// MQTT消息处理
void onMqttMessage(String topic, String payload) {
    if (topic.indexOf("/ota/check") > 0) {
        otaManager.handleMQTTMessage(topic, payload);
    }
}

// MQTT发布回调
void mqttPublishCallback(const char* topic, const char* payload) {
    mqttClient.publish(topic, payload);
}
```

### 服务器端API示例
```python
# 检查版本更新
@app.route('/api/firmware/check', methods=['GET'])
def check_firmware():
    device_id = request.args.get('device_id')
    current_version = request.args.get('version')
    
    # 检查是否有新版本
    latest = get_latest_firmware(device_id)
    
    if version_compare(latest.version, current_version) > 0:
        return {
            "latest_version": latest.version,
            "download_url": latest.download_url,
            "force_update": latest.force_update,
            "release_notes": latest.release_notes
        }
    
    return {"message": "已是最新版本"}
```

## 升级日志示例

```
[OTAManager] OTA管理器初始化开始
[OTAManager] 设备ID: ESP32_ABCD1234
[OTAManager] 当前版本: v4.0.0
[OTAManager] 最低电池要求: 90%
[OTAManager] 检查在线更新...
[OTAManager] 发送版本检查请求
[OTAManager] 收到版本检查响应: v4.1.0
[OTAManager] 更新说明: 修复蓝牙连接问题
[OTAManager] 当前电池电量: 95%
[OTAManager] 充电状态: 充电中
[OTAManager] 可用Flash空间: 2.5 MB
[OTAManager] 检测到在线更新，电池电量充足，开始升级
[OTAManager] 开始下载在线固件，请勿断电
[OTAManager] 开始下载固件: https://server.com/firmware/v4.1.0.bin
[OTAManager] 固件大小: 1.2 MB
[OTAManager] 下载进度: 20%
[OTAManager] 下载进度: 40%
[OTAManager] 下载进度: 60%
[OTAManager] 下载进度: 80%
[OTAManager] 下载进度: 100%
[OTAManager] 固件下载完成: 1.2 MB
[OTAManager] 下载完成，开始安装
[OTAManager] 固件安装成功
[OTAManager] 升级成功，即将重启
[OTAManager] 状态报告: success (100%) - 升级成功
```

## 故障排除

### 常见问题

1. **电池电量不足**
   - 确保电池电量≥90%或正在充电
   - 检查电池电量读取是否正常

2. **网络下载失败**
   - 检查4G网络连接状态
   - 验证服务器URL是否可访问
   - 确认防火墙没有拦截

3. **版本号格式错误**
   - 使用标准格式：v4.1.0
   - 支持格式：v4.0.0, 4.0.0, v4.0.0+694

4. **MQTT连接问题**
   - 检查MQTT broker连接状态
   - 验证订阅的主题是否正确
   - 确认回调函数已设置

### 调试信息

系统会输出详细的调试信息：
```
[OTAManager] 设备ID: ESP32_ABCD1234
[OTAManager] 当前版本: v4.0.0
[OTAManager] 当前电池电量: 95%
[OTAManager] 状态报告: downloading (75%) - 下载进度75%
```

## 注意事项

1. **升级过程中请勿断电**，否则可能导致设备无法启动
2. **确保固件文件完整性**，建议使用校验和验证
3. **网络升级需要稳定的4G连接**
4. **升级前会自动备份关键配置**
5. **支持版本回滚**，升级失败时自动恢复
6. **建议在充电状态下进行升级**

## API参考

### OTAManager类

#### 公共方法

```cpp
// 初始化OTA管理器
void begin();

// 设置MQTT发布回调
void setMQTTPublishCallback(void (*callback)(const char*, const char*));

// 检查更新
void checkForUpdates();

// 处理MQTT消息
void handleMQTTMessage(String topic, String payload);

// 获取当前状态
OTAStatus getStatus();

// 获取升级进度
int getProgress();
```

#### OTA状态枚举

```cpp
enum OTAStatus {
    OTA_IDLE,           // 空闲
    OTA_CHECKING,       // 检查中
    OTA_DOWNLOADING,    // 下载中
    OTA_INSTALLING,     // 安装中
    OTA_SUCCESS,        // 成功
    OTA_FAILED          // 失败
};
```

## 版本历史

- v1.0.0: 基础OTA功能，支持SD卡和MQTT升级
- v1.1.0: 添加语音提示和电池电量检查
- v1.2.0: 增强安全验证和错误处理
- v2.0.0: 移除SD卡升级，专注MQTT在线升级

## 技术规格

- **支持的固件大小**: 100KB - 10MB
- **下载超时时间**: 5分钟（可配置）
- **进度更新频率**: 每20%
- **重试机制**: 支持（可配置）
- **内存占用**: 约4KB RAM
- **电池要求**: ≥90%电量
- **支持的版本格式**: v4.0.0, 4.0.0, v4.0.0+694

## 许可证

MIT License
