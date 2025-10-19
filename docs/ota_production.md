# 生产环境OTA配置

## 公网服务器配置

### 固件服务器
- **域名**: https://daboluo.cc/
- **固件路径**: `/firmware/esp32-air780eg/`
- **完整URL**: `https://daboluo.cc/firmware/esp32-air780eg/v{version}.bin`

### MQTT Broker
推荐使用云服务MQTT：
- **EMQX Cloud**: https://www.emqx.com/zh/cloud
- **阿里云IoT**: https://iot.console.aliyun.com/
- **腾讯云IoT**: https://console.cloud.tencent.com/iotexplorer

## 固件发布流程

### 1. 编译固件
```bash
pio run --environment esp32-air780eg
```

### 2. 上传到服务器
```bash
# 复制固件到服务器
scp .pio/build/esp32-air780eg/firmware.bin user@daboluo.cc:/var/www/html/firmware/esp32-air780eg/v4.3.0.bin
```

### 3. 发布升级通知
```bash
# 通过MQTT发送升级通知
mosquitto_pub -h your-mqtt-broker.com -t "device/+/ota/check" -m '{
    "latest_version": "v4.3.0",
    "download_url": "https://daboluo.cc/firmware/esp32-air780eg/v4.3.0.bin",
    "force_update": false,
    "release_notes": "修复关键问题，建议升级"
}'
```

## 自动升级开关

### 通过串口控制
```cpp
// 启用自动升级
otaManager.setAutoUpgrade(true);

// 禁用自动升级
otaManager.setAutoUpgrade(false);

// 查询状态
bool enabled = otaManager.getAutoUpgrade();
```

### 通过MQTT控制
```json
// 发送到 device/{device_id}/ota/config
{
    "auto_upgrade": true,
    "check_interval": 3600000
}
```

## 安全建议

1. **HTTPS固件下载**：确保固件通过HTTPS传输
2. **固件签名验证**：添加MD5/SHA256校验
3. **版本回滚保护**：保留上一版本固件
4. **分批升级**：避免同时升级所有设备

## 监控和日志

设备会上报以下OTA状态：
- `checking`: 检查更新中
- `downloading`: 下载中
- `success`: 升级成功
- `failed`: 升级失败
- `auto_disabled`: 自动升级已禁用
- `conditions_not_met`: 升级条件不满足
