# BLE模块化特征值测试指南

## 测试概述

本指南用于测试新的模块化BLE特征值实现，验证四个独立特征值的功能和数据传输。

## 测试环境

### 硬件要求
- ESP32-S3 MotoBox设备
- 支持BLE的测试设备（手机、电脑等）

### 软件要求
- BLE调试工具（如nRF Connect、LightBlue等）
- 或自定义测试客户端

## 特征值信息

### 服务UUID
```
A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D
```

### 特征值UUID列表
```
GPS位置数据:    B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E
IMU传感器数据:  C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F
罗盘数据:       D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A
系统状态数据:   E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B
```

## 测试步骤

### 1. 设备连接测试

1. **扫描设备**
   - 查找名称为 `MotoBox-XXXXXX` 的设备
   - 验证设备可被发现

2. **建立连接**
   - 连接到设备
   - 验证连接成功

3. **服务发现**
   - 发现主服务 `A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D`
   - 验证四个特征值都存在

### 2. GPS特征值测试

**特征值**: `B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E`

1. **订阅通知**
   ```javascript
   await gpsCharacteristic.startNotifications();
   ```

2. **验证数据格式**
   ```json
   {
     "device_id": "ESP32_ABC123",
     "timestamp": 1234567890,
     "location": {
       "lat": 39.904200,
       "lng": 116.407400,
       "altitude": 50.5,
       "speed": 25.30,
       "heading": 180.0,
       "satellites": 8,
       "hdop": 1.2,
       "valid": true
     },
     "status": {
       "gnss_ready": true,
       "fix_quality": "3D_FIX"
     }
   }
   ```

3. **验证更新频率**
   - 确认数据每200ms更新一次（5Hz）
   - 验证数据量在280字节左右

### 3. IMU特征值测试

**特征值**: `C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F`

1. **订阅通知**
   ```javascript
   await imuCharacteristic.startNotifications();
   ```

2. **验证数据格式**
   ```json
   {
     "device_id": "ESP32_ABC123",
     "timestamp": 1234567890,
     "imu": {
       "accel": {"x": 0.12, "y": -0.25, "z": 9.78},
       "gyro": {"x": 0.05, "y": -0.03, "z": 0.01},
       "attitude": {"roll": 2.5, "pitch": -1.8, "yaw": 180.0},
       "temperature": 25.6,
       "valid": true
     },
     "status": {
       "imu_ready": true,
       "calibrated": true
     }
   }
   ```

3. **验证更新频率**
   - 确认数据每100ms更新一次（10Hz）
   - 验证数据量在320字节左右

### 4. 罗盘特征值测试

**特征值**: `D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A`

1. **订阅通知**
   ```javascript
   await compassCharacteristic.startNotifications();
   ```

2. **验证数据格式**
   ```json
   {
     "device_id": "ESP32_ABC123",
     "timestamp": 1234567890,
     "compass": {
       "heading": 185.2,
       "magnetic": {"x": 15.3, "y": -8.7, "z": 42.1},
       "declination": -5.2,
       "field_strength": 48.5,
       "valid": true
     },
     "status": {
       "compass_ready": true,
       "calibrated": true
     }
   }
   ```

3. **验证更新频率**
   - 确认数据每500ms更新一次（2Hz）
   - 验证数据量在250字节左右

### 5. 系统状态特征值测试

**特征值**: `E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B`

1. **订阅通知**
   ```javascript
   await systemCharacteristic.startNotifications();
   ```

2. **验证数据格式**
   ```json
   {
     "device_id": "ESP32_ABC123",
     "timestamp": 1234567890,
     "firmware": "v4.2.0+502",
     "system": {
       "battery_percentage": 85,
       "is_charging": false,
       "signal_strength": 75,
       "uptime": 3600,
       "free_heap": 250000
     },
     "modules": {
       "wifi_ready": true,
       "ble_ready": true,
       "gsm_ready": true
     },
     "storage": {
       "free_mb": 16384,
       "used_percentage": 50
     }
   }
   ```

3. **验证更新频率**
   - 确认数据每1000ms更新一次（1Hz）
   - 验证数据量在380字节左右

## 性能测试

### 1. 数据量测试
- 验证每个特征值的数据量都在400字节以内
- 确认没有数据截断问题

### 2. 更新频率测试
- GPS: 5Hz (200ms间隔)
- IMU: 10Hz (100ms间隔)
- 罗盘: 2Hz (500ms间隔)
- 系统: 1Hz (1000ms间隔)

### 3. 按需订阅测试
- 测试只订阅GPS特征值
- 测试只订阅IMU特征值
- 测试订阅GPS+IMU组合
- 测试订阅所有特征值

### 4. 连接稳定性测试
- 长时间连接测试（30分钟以上）
- 断线重连测试
- 多客户端连接测试

## 测试客户端示例

### Python测试客户端

```python
import asyncio
import json
from bleak import BleakClient, BleakScanner

# 服务和特征值UUID
SERVICE_UUID = "A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D"
GPS_CHAR_UUID = "B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E"
IMU_CHAR_UUID = "C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F"
COMPASS_CHAR_UUID = "D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A"
SYSTEM_CHAR_UUID = "E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B"

def gps_handler(sender, data):
    """处理GPS数据"""
    try:
        gps_data = json.loads(data.decode('utf-8'))
        location = gps_data.get('location', {})
        print(f"GPS: {location.get('lat')}, {location.get('lng')}, 速度: {location.get('speed')} km/h")
    except Exception as e:
        print(f"GPS数据解析错误: {e}")

def imu_handler(sender, data):
    """处理IMU数据"""
    try:
        imu_data = json.loads(data.decode('utf-8'))
        attitude = imu_data.get('imu', {}).get('attitude', {})
        print(f"IMU: Roll={attitude.get('roll')}°, Pitch={attitude.get('pitch')}°, Yaw={attitude.get('yaw')}°")
    except Exception as e:
        print(f"IMU数据解析错误: {e}")

def compass_handler(sender, data):
    """处理罗盘数据"""
    try:
        compass_data = json.loads(data.decode('utf-8'))
        compass = compass_data.get('compass', {})
        print(f"罗盘: 方向={compass.get('heading')}°")
    except Exception as e:
        print(f"罗盘数据解析错误: {e}")

def system_handler(sender, data):
    """处理系统状态数据"""
    try:
        system_data = json.loads(data.decode('utf-8'))
        system = system_data.get('system', {})
        print(f"系统: 电池={system.get('battery_percentage')}%, 内存={system.get('free_heap')}")
    except Exception as e:
        print(f"系统数据解析错误: {e}")

async def test_motobox_ble():
    """测试MotoBox BLE连接"""
    
    # 扫描设备
    print("扫描MotoBox设备...")
    devices = await BleakScanner.discover()
    motobox_device = None
    
    for device in devices:
        if device.name and "MotoBox" in device.name:
            motobox_device = device
            break
    
    if not motobox_device:
        print("未找到MotoBox设备")
        return
    
    print(f"找到设备: {motobox_device.name} ({motobox_device.address})")
    
    # 连接设备
    async with BleakClient(motobox_device.address) as client:
        print("连接成功!")
        
        # 订阅特征值
        await client.start_notify(GPS_CHAR_UUID, gps_handler)
        await client.start_notify(IMU_CHAR_UUID, imu_handler)
        await client.start_notify(COMPASS_CHAR_UUID, compass_handler)
        await client.start_notify(SYSTEM_CHAR_UUID, system_handler)
        
        print("开始接收数据...")
        
        # 运行30秒
        await asyncio.sleep(30)
        
        print("测试完成")

if __name__ == "__main__":
    asyncio.run(test_motobox_ble())
```

### JavaScript测试客户端

```javascript
// Web Bluetooth API测试客户端
class MotoBoxBLETest {
    constructor() {
        this.device = null;
        this.server = null;
        this.service = null;
        this.characteristics = {};
    }
    
    async connect() {
        try {
            // 请求设备
            this.device = await navigator.bluetooth.requestDevice({
                filters: [{ namePrefix: 'MotoBox' }],
                services: ['A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D']
            });
            
            // 连接GATT服务器
            this.server = await this.device.gatt.connect();
            
            // 获取服务
            this.service = await this.server.getPrimaryService('A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D');
            
            // 获取特征值
            this.characteristics.gps = await this.service.getCharacteristic('B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E');
            this.characteristics.imu = await this.service.getCharacteristic('C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F');
            this.characteristics.compass = await this.service.getCharacteristic('D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A');
            this.characteristics.system = await this.service.getCharacteristic('E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B');
            
            console.log('连接成功!');
            return true;
        } catch (error) {
            console.error('连接失败:', error);
            return false;
        }
    }
    
    async startNotifications() {
        // 订阅GPS数据
        await this.characteristics.gps.startNotifications();
        this.characteristics.gps.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            console.log('GPS数据:', data);
        });
        
        // 订阅IMU数据
        await this.characteristics.imu.startNotifications();
        this.characteristics.imu.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            console.log('IMU数据:', data);
        });
        
        // 订阅罗盘数据
        await this.characteristics.compass.startNotifications();
        this.characteristics.compass.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            console.log('罗盘数据:', data);
        });
        
        // 订阅系统状态数据
        await this.characteristics.system.startNotifications();
        this.characteristics.system.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            console.log('系统数据:', data);
        });
        
        console.log('开始接收数据...');
    }
}

// 使用示例
const test = new MotoBoxBLETest();
test.connect().then(success => {
    if (success) {
        test.startNotifications();
    }
});
```

## 预期结果

### 成功标准
1. ✅ 设备可被正常发现和连接
2. ✅ 四个特征值都能正常订阅
3. ✅ 数据格式符合JSON规范
4. ✅ 更新频率符合设计要求
5. ✅ 数据量控制在400字节以内
6. ✅ 按需订阅功能正常
7. ✅ 连接稳定，无异常断线

### 性能指标
- GPS数据: 5Hz, ~280字节
- IMU数据: 10Hz, ~320字节
- 罗盘数据: 2Hz, ~250字节
- 系统数据: 1Hz, ~380字节

## 故障排除

### 常见问题

1. **设备无法发现**
   - 检查BLE是否启用
   - 确认设备名称配置
   - 验证广播是否正常

2. **连接失败**
   - 检查设备距离
   - 确认没有其他客户端占用连接
   - 重启设备重试

3. **数据格式错误**
   - 检查JSON格式是否正确
   - 验证字符编码（UTF-8）
   - 确认数据完整性

4. **更新频率异常**
   - 检查时间间隔配置
   - 验证数据变化检测逻辑
   - 确认没有阻塞操作

5. **数据截断**
   - 检查数据量是否超过MTU
   - 验证JSON序列化结果
   - 确认BLE缓冲区大小

通过以上测试步骤，可以全面验证新的模块化BLE特征值实现的功能和性能。