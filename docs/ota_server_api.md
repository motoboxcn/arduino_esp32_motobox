# OTA服务器端API实现

## 1. MQTT Broker配置

```bash
# 使用Mosquitto
mosquitto -p 1883 -v

# 或使用EMQX
docker run -d --name emqx -p 1883:1883 -p 8083:8083 -p 8084:8084 -p 8883:8883 -p 18083:18083 emqx/emqx
```

## 2. 固件服务器

### Python Flask示例
```python
from flask import Flask, request, jsonify, send_file
import json
import os

app = Flask(__name__)

# 固件版本信息
FIRMWARE_INFO = {
    "latest_version": "v4.3.0",
    "download_url": "http://your-server.com/firmware/v4.3.0.bin",
    "force_update": False,
    "release_notes": "Air780EG原生OTA升级"
}

@app.route('/firmware/<version>.bin')
def download_firmware(version):
    """下载固件文件"""
    firmware_path = f"firmware/{version}.bin"
    if os.path.exists(firmware_path):
        return send_file(firmware_path, as_attachment=True)
    return "Firmware not found", 404

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080)
```

## 3. MQTT消息处理

### Node.js示例
```javascript
const mqtt = require('mqtt');
const client = mqtt.connect('mqtt://localhost:1883');

client.on('connect', () => {
    console.log('MQTT服务器连接成功');
    // 订阅所有设备的OTA检查请求
    client.subscribe('device/+/ota/check');
});

client.on('message', (topic, message) => {
    const deviceId = topic.split('/')[1];
    const payload = JSON.parse(message.toString());
    
    if (topic.endsWith('/ota/check')) {
        // 处理版本检查请求
        const response = {
            latest_version: "v4.3.0",
            download_url: "http://your-server.com/firmware/v4.3.0.bin",
            force_update: false
        };
        
        // 发送响应
        client.publish(`device/${deviceId}/ota/check`, JSON.stringify(response));
    }
});
```

## 4. 测试脚本

### 手动触发OTA测试
```bash
# 发送版本检查响应
mosquitto_pub -h localhost -t "device/ESP32_ABCD1234/ota/check" -m '{
    "latest_version": "v4.3.0",
    "download_url": "http://192.168.1.100:8080/firmware/v4.3.0.bin",
    "force_update": false
}'
```
