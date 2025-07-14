# ESP32-S3 MotoBox OTA升级系统

## 功能概述

本OTA升级系统支持两种升级方式：
1. **SD卡本地升级**：通过SD卡中的固件文件进行离线升级
2. **MQTT在线升级**：通过4G网络接收服务器推送的固件更新

## 升级条件

系统会自动检查以下升级条件：
- ✅ **电池电量**：必须≥90%才能进行升级
- ✅ **版本检查**：新版本号必须高于当前版本
- ✅ **文件完整性**：固件文件存在且校验通过
- ✅ **存储空间**：有足够的Flash空间进行升级

## SD卡升级使用方法

### 1. 准备固件文件
在SD卡根目录放置以下文件：
```
/firmware.bin      # 固件文件
/version.txt       # 版本号文件（如：v4.1.0）
/checksum.txt      # 校验和文件（可选）
```

### 2. 自动升级流程
1. 设备启动时自动检测SD卡
2. 发现新版本固件时播放语音提示
3. 检查电池电量（≥90%）
4. 自动执行升级并重启

### 3. 语音提示说明
- "检测到SD卡新固件，电池电量充足，开始升级"
- "正在从SD卡升级固件，请勿断电"
- "升级成功，即将重启" / "升级失败"

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
    "status": "downloading|success|failed",
    "progress": 75,
    "message": "下载进度75%",
    "timestamp": 1642678800,
    "version": "v4.0.0"
}
```

### 4. 手动触发升级
通过MQTT发送命令：
```json
{
    "cmd": "check_ota"
}
```

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
#include "audio/VoicePrompt.h"

void setup() {
    // 初始化语音提示
    voicePrompt.begin();
    
    // 初始化OTA管理器
    otaManager.begin();
    
    // 设置MQTT回调
    otaManager.setMQTTPublishCallback(mqttPublishCallback);
}

void loop() {
    // OTA管理器会自动处理升级逻辑
    // 无需在主循环中添加代码
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
            "force_update": latest.force_update
        }
    
    return {"message": "已是最新版本"}
```

## 故障排除

### 常见问题
1. **电池电量不足**：确保电池电量≥90%
2. **SD卡读取失败**：检查SD卡格式和文件完整性
3. **网络下载失败**：检查4G网络连接和服务器可用性
4. **版本号格式错误**：使用标准格式如 v4.1.0

### 调试信息
系统会输出详细的调试信息：
```
[OTAManager] 设备ID: ESP32_ABCD1234
[OTAManager] 当前版本: v4.0.0
[OTAManager] 当前电池电量: 95%
[VoicePrompt] [UPGRADE_START] 检测到SD卡新固件
```

## 注意事项

1. **升级过程中请勿断电**，否则可能导致设备无法启动
2. **确保固件文件完整性**，建议使用校验和验证
3. **网络升级需要稳定的4G连接**
4. **升级前会自动备份关键配置**
5. **支持版本回滚**，升级失败时自动恢复

## 版本历史

- v1.0.0: 基础OTA功能，支持SD卡和MQTT升级
- v1.1.0: 添加语音提示和电池电量检查
- v1.2.0: 增强安全验证和错误处理
