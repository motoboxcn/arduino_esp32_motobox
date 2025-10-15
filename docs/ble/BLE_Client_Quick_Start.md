# MotoBox BLE客户端快速开发指南

## 📋 概述

MotoBox设备提供4个独立的BLE特征值，客户端可以按需订阅。每个特征值控制在400字节以内，避免传输截断问题。

## 🔧 连接信息

### 基本信息
- **设备名称**: `MotoBox-XXXXXX`
- **服务UUID**: `A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D`

### 特征值列表
| 功能 | UUID | 频率 | 大小 | 用途 |
|------|------|------|------|------|
| GPS位置 | `B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E` | 5Hz | ~280字节 | 实时定位 |
| IMU传感器 | `C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F` | 10Hz | ~320字节 | 运动检测 |
| 罗盘数据 | `D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A` | 2Hz | ~250字节 | 方向导航 |
| 系统状态 | `E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B` | 1Hz | ~380字节 | 设备监控 |

## 📱 应用场景

### 1. 基础定位应用
**只需要**: GPS位置数据
```javascript
// 只订阅GPS特征值
const gpsChar = await service.getCharacteristic('B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E');
await gpsChar.startNotifications();
```

### 2. 运动分析应用
**需要**: GPS + IMU数据
```javascript
// 订阅GPS和IMU特征值
const gpsChar = await service.getCharacteristic('B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E');
const imuChar = await service.getCharacteristic('C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F');
await gpsChar.startNotifications();
await imuChar.startNotifications();
```

### 3. 导航应用
**需要**: GPS + 罗盘数据
```javascript
// 订阅GPS和罗盘特征值
const gpsChar = await service.getCharacteristic('B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E');
const compassChar = await service.getCharacteristic('D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A');
await gpsChar.startNotifications();
await compassChar.startNotifications();
```

### 4. 完整监控应用
**需要**: 所有数据
```javascript
// 订阅所有特征值
const characteristics = [
    'B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E', // GPS
    'C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F', // IMU
    'D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A', // 罗盘
    'E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B'  // 系统
];

for (const uuid of characteristics) {
    const char = await service.getCharacteristic(uuid);
    await char.startNotifications();
    char.addEventListener('characteristicvaluechanged', handleData);
}
```

## 📊 数据格式

### 1. GPS位置数据 (5Hz, ~280字节)
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1703123456789,
  "location": {
    "lat": 39.904200,
    "lng": 116.407400,
    "altitude": 50.5,
    "speed": 25.30,
    "heading": 180.0,
    "satellites": 8,
    "hdop": 1.2,
    "vdop": 1.8,
    "pdop": 2.1,
    "fix_type": 3,
    "timestamp": 1703123456789,
    "valid": true
  },
  "status": {
    "gnss_ready": true,
    "fix_quality": "3D_FIX",
    "last_fix_age": 100
  }
}
```

### 2. IMU传感器数据 (10Hz, ~320字节)
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1703123456789,
  "imu": {
    "accel": {
      "x": 0.12,
      "y": -0.25,
      "z": 9.78
    },
    "gyro": {
      "x": 0.05,
      "y": -0.03,
      "z": 0.01
    },
    "attitude": {
      "roll": 2.5,
      "pitch": -1.8,
      "yaw": 180.0
    },
    "temperature": 25.6,
    "timestamp": 1703123456789,
    "valid": true
  },
  "status": {
    "imu_ready": true,
    "calibrated": true,
    "motion_detected": false,
    "vibration_level": 0.15
  }
}
```

### 3. 罗盘数据 (2Hz, ~250字节)
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1703123456789,
  "compass": {
    "heading": 185.2,
    "magnetic": {
      "x": 15.3,
      "y": -8.7,
      "z": 42.1
    },
    "declination": -5.2,
    "inclination": 65.8,
    "field_strength": 48.5,
    "timestamp": 1703123456789,
    "valid": true
  },
  "status": {
    "compass_ready": true,
    "calibrated": true,
    "interference": false,
    "calibration_quality": 85
  }
}
```

### 4. 系统状态数据 (1Hz, ~380字节)
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1703123456789,
  "firmware": "v4.2.0+511",
  "hardware": "esp32-air780eg",
  "power_mode": 2,
  "system": {
    "battery_voltage": 3850,
    "battery_percentage": 85,
    "is_charging": false,
    "external_power": true,
    "signal_strength": 75,
    "uptime": 3600,
    "free_heap": 250000,
    "cpu_usage": 45,
    "temperature": 42.5
  },
  "modules": {
    "wifi_ready": true,
    "ble_ready": true,
    "gsm_ready": true,
    "gnss_ready": true,
    "imu_ready": true,
    "compass_ready": true,
    "sd_ready": false,
    "audio_ready": false
  },
  "storage": {
    "total_mb": 32768,
    "free_mb": 16384,
    "used_percentage": 50
  },
  "network": {
    "wifi_connected": false,
    "gsm_connected": true,
    "ip_address": "10.0.0.100",
    "operator": "China Mobile"
  }
}
```

## 💻 完整示例代码

### JavaScript (Web Bluetooth)
```javascript
class MotoBoxBLE {
    constructor() {
        this.device = null;
        this.server = null;
        this.service = null;
        this.characteristics = {};
    }

    // 连接设备
    async connect() {
        try {
            this.device = await navigator.bluetooth.requestDevice({
                filters: [{ namePrefix: 'MotoBox' }],
                services: ['A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D']
            });

            this.server = await this.device.gatt.connect();
            this.service = await this.server.getPrimaryService('A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D');
            
            console.log('✅ 连接成功');
            return true;
        } catch (error) {
            console.error('❌ 连接失败:', error);
            return false;
        }
    }

    // 订阅GPS数据
    async subscribeGPS(callback) {
        const char = await this.service.getCharacteristic('B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E');
        await char.startNotifications();
        char.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            callback(data);
        });
        console.log('📍 GPS数据订阅成功');
    }

    // 订阅IMU数据
    async subscribeIMU(callback) {
        const char = await this.service.getCharacteristic('C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F');
        await char.startNotifications();
        char.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            callback(data);
        });
        console.log('🏃 IMU数据订阅成功');
    }

    // 订阅罗盘数据
    async subscribeCompass(callback) {
        const char = await this.service.getCharacteristic('D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A');
        await char.startNotifications();
        char.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            callback(data);
        });
        console.log('🧭 罗盘数据订阅成功');
    }

    // 订阅系统状态
    async subscribeSystem(callback) {
        const char = await this.service.getCharacteristic('E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B');
        await char.startNotifications();
        char.addEventListener('characteristicvaluechanged', (event) => {
            const data = JSON.parse(new TextDecoder().decode(event.target.value));
            callback(data);
        });
        console.log('⚙️ 系统状态订阅成功');
    }
}

// 使用示例
const motobox = new MotoBoxBLE();

// 连接并订阅数据
motobox.connect().then(success => {
    if (success) {
        // 订阅GPS数据
        motobox.subscribeGPS((data) => {
            console.log('GPS:', data.location.lat, data.location.lng, `速度: ${data.location.speed} km/h`);
        });

        // 订阅IMU数据
        motobox.subscribeIMU((data) => {
            const attitude = data.imu.attitude;
            console.log('IMU:', `Roll: ${attitude.roll}°, Pitch: ${attitude.pitch}°, Yaw: ${attitude.yaw}°`);
        });

        // 订阅系统状态
        motobox.subscribeSystem((data) => {
            console.log('系统:', `电池: ${data.system.battery_percentage}%, 信号: ${data.system.signal_strength}%`);
        });
    }
});
```

### Python (使用bleak库)
```python
import asyncio
import json
from bleak import BleakClient, BleakScanner

class MotoBoxBLE:
    def __init__(self):
        self.client = None
        self.device_address = None
        
        # 特征值UUID
        self.GPS_UUID = "B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E"
        self.IMU_UUID = "C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F"
        self.COMPASS_UUID = "D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A"
        self.SYSTEM_UUID = "E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B"

    async def scan_and_connect(self):
        """扫描并连接MotoBox设备"""
        print("🔍 扫描MotoBox设备...")
        devices = await BleakScanner.discover()
        
        for device in devices:
            if device.name and "MotoBox" in device.name:
                self.device_address = device.address
                print(f"✅ 找到设备: {device.name} ({device.address})")
                break
        
        if not self.device_address:
            print("❌ 未找到MotoBox设备")
            return False
        
        self.client = BleakClient(self.device_address)
        await self.client.connect()
        print("🔗 连接成功")
        return True

    async def subscribe_gps(self, callback):
        """订阅GPS数据"""
        def handler(sender, data):
            try:
                gps_data = json.loads(data.decode('utf-8'))
                callback(gps_data)
            except Exception as e:
                print(f"GPS数据解析错误: {e}")
        
        await self.client.start_notify(self.GPS_UUID, handler)
        print("📍 GPS数据订阅成功")

    async def subscribe_imu(self, callback):
        """订阅IMU数据"""
        def handler(sender, data):
            try:
                imu_data = json.loads(data.decode('utf-8'))
                callback(imu_data)
            except Exception as e:
                print(f"IMU数据解析错误: {e}")
        
        await self.client.start_notify(self.IMU_UUID, handler)
        print("🏃 IMU数据订阅成功")

    async def subscribe_system(self, callback):
        """订阅系统状态"""
        def handler(sender, data):
            try:
                system_data = json.loads(data.decode('utf-8'))
                callback(system_data)
            except Exception as e:
                print(f"系统数据解析错误: {e}")
        
        await self.client.start_notify(self.SYSTEM_UUID, handler)
        print("⚙️ 系统状态订阅成功")

# 数据处理函数
def handle_gps(data):
    location = data.get('location', {})
    if location.get('valid'):
        print(f"📍 位置: {location['lat']:.6f}, {location['lng']:.6f}, 速度: {location['speed']} km/h")

def handle_imu(data):
    attitude = data.get('imu', {}).get('attitude', {})
    print(f"🏃 姿态: Roll={attitude.get('roll', 0):.1f}°, Pitch={attitude.get('pitch', 0):.1f}°, Yaw={attitude.get('yaw', 0):.1f}°")

def handle_system(data):
    system = data.get('system', {})
    print(f"⚙️ 系统: 电池={system.get('battery_percentage', 0)}%, 信号={system.get('signal_strength', 0)}%")

# 使用示例
async def main():
    motobox = MotoBoxBLE()
    
    if await motobox.scan_and_connect():
        # 订阅需要的数据
        await motobox.subscribe_gps(handle_gps)
        await motobox.subscribe_imu(handle_imu)
        await motobox.subscribe_system(handle_system)
        
        print("🚀 开始接收数据...")
        
        # 运行30秒
        await asyncio.sleep(30)
        
        print("✅ 测试完成")

if __name__ == "__main__":
    asyncio.run(main())
```

### React Native (使用react-native-ble-plx)
```javascript
import { BleManager } from 'react-native-ble-plx';

class MotoBoxBLE {
    constructor() {
        this.manager = new BleManager();
        this.device = null;
        
        // 特征值UUID
        this.SERVICE_UUID = 'A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D';
        this.GPS_UUID = 'B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E';
        this.IMU_UUID = 'C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F';
        this.COMPASS_UUID = 'D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A';
        this.SYSTEM_UUID = 'E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B';
    }

    async scanAndConnect() {
        return new Promise((resolve, reject) => {
            this.manager.startDeviceScan(null, null, (error, device) => {
                if (error) {
                    reject(error);
                    return;
                }

                if (device.name && device.name.includes('MotoBox')) {
                    this.manager.stopDeviceScan();
                    
                    device.connect()
                        .then(device => device.discoverAllServicesAndCharacteristics())
                        .then(device => {
                            this.device = device;
                            console.log('✅ 连接成功');
                            resolve(true);
                        })
                        .catch(reject);
                }
            });
        });
    }

    async subscribeGPS(callback) {
        this.device.monitorCharacteristicForService(
            this.SERVICE_UUID,
            this.GPS_UUID,
            (error, characteristic) => {
                if (error) {
                    console.error('GPS订阅错误:', error);
                    return;
                }
                
                const data = JSON.parse(atob(characteristic.value));
                callback(data);
            }
        );
        console.log('📍 GPS数据订阅成功');
    }

    async subscribeIMU(callback) {
        this.device.monitorCharacteristicForService(
            this.SERVICE_UUID,
            this.IMU_UUID,
            (error, characteristic) => {
                if (error) {
                    console.error('IMU订阅错误:', error);
                    return;
                }
                
                const data = JSON.parse(atob(characteristic.value));
                callback(data);
            }
        );
        console.log('🏃 IMU数据订阅成功');
    }
}

// 使用示例
const motobox = new MotoBoxBLE();

motobox.scanAndConnect().then(() => {
    // 订阅GPS数据
    motobox.subscribeGPS((data) => {
        const location = data.location;
        console.log(`📍 位置: ${location.lat}, ${location.lng}`);
    });

    // 订阅IMU数据
    motobox.subscribeIMU((data) => {
        const attitude = data.imu.attitude;
        console.log(`🏃 姿态: Roll=${attitude.roll}°`);
    });
});
```

## 🔧 开发提示

### 1. 错误处理
```javascript
// 始终包含错误处理
try {
    const data = JSON.parse(new TextDecoder().decode(event.target.value));
    // 处理数据
} catch (error) {
    console.error('数据解析错误:', error);
}
```

### 2. 数据验证
```javascript
// 检查数据有效性
if (data.location && data.location.valid) {
    // 使用GPS数据
}

if (data.imu && data.imu.valid) {
    // 使用IMU数据
}
```

### 3. 连接管理
```javascript
// 监听连接状态
device.addEventListener('gattserverdisconnected', () => {
    console.log('设备已断开连接');
    // 实现重连逻辑
});
```

### 4. 性能优化
```javascript
// 避免频繁更新UI，使用节流
let lastUpdate = 0;
const updateInterval = 100; // 100ms

function handleData(data) {
    const now = Date.now();
    if (now - lastUpdate > updateInterval) {
        updateUI(data);
        lastUpdate = now;
    }
}
```

## 📝 常见问题

### Q: 如何选择需要的特征值？
A: 根据应用需求选择：
- 基础定位：只要GPS
- 运动分析：GPS + IMU
- 导航应用：GPS + 罗盘
- 完整监控：全部特征值

### Q: 数据更新频率是多少？
A: 
- GPS: 5Hz (每200ms)
- IMU: 10Hz (每100ms)
- 罗盘: 2Hz (每500ms)
- 系统: 1Hz (每1000ms)

### Q: 如何处理连接断开？
A: 监听断开事件并实现自动重连逻辑。

### Q: 数据量会不会太大？
A: 每个特征值都控制在400字节以内，不会出现截断问题。

---

🎉 现在你可以开始开发MotoBox BLE客户端了！选择适合的特征值组合，享受稳定可靠的数据传输。