# BLE模块化特征值设计指南

## 概述

MotoBox ESP32设备通过BLE（蓝牙低功耗）提供四个独立的特征值，按功能模块分离，便于客户端按需订阅和数据传输优化。每个特征值控制在400字节以内，避免传输截断问题。

## 服务信息

- **服务UUID**: `A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D`
- **设备名称**: `MotoBox` 或 `MotoBox_XXXXXX`（带设备ID）
- **连接方式**: BLE GATT服务

---

## 特征值1: GPS位置数据 (GPS Location Data)

### 基本信息
- **UUID**: `B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E`
- **属性**: READ + NOTIFY
- **更新频率**: 5Hz (每200ms)
- **数据量**: ~280字节
- **用途**: 传输GPS位置和导航信息

### 数据格式
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
    "vdop": 1.8,
    "pdop": 2.1,
    "fix_type": 3,
    "timestamp": 1234567890,
    "valid": true
  },
  "status": {
    "gnss_ready": true,
    "fix_quality": "3D_FIX",
    "last_fix_age": 100
  }
}
```

### 字段说明
- `device_id`: 设备唯一标识符
- `timestamp`: 数据时间戳（Unix时间戳）
- `location`: GPS位置详细信息
  - `lat/lng`: 纬度/经度（度）
  - `altitude`: 海拔高度（米）
  - `speed`: 地面速度（km/h）
  - `heading`: 航向角（度，0-360）
  - `satellites`: 可见卫星数量
  - `hdop/vdop/pdop`: 精度因子
  - `fix_type`: 定位类型（0=无效, 1=GPS, 2=DGPS, 3=PPS）
- `status`: GPS模块状态信息

---

## 特征值2: IMU传感器数据 (IMU Sensor Data)

### 基本信息
- **UUID**: `C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F`
- **属性**: READ + NOTIFY
- **更新频率**: 10Hz (每100ms)
- **数据量**: ~320字节
- **用途**: 传输IMU加速度计、陀螺仪和姿态数据

### 数据格式
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1234567890,
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
    "timestamp": 1234567890,
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

### 字段说明
- `accel`: 加速度计数据（m/s²）
- `gyro`: 陀螺仪数据（rad/s）
- `attitude`: 姿态角度（度）
  - `roll`: 横滚角
  - `pitch`: 俯仰角
  - `yaw`: 偏航角
- `temperature`: IMU芯片温度（℃）
- `status`: IMU状态信息

---

## 特征值3: 罗盘数据 (Compass Data)

### 基本信息
- **UUID**: `D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A`
- **属性**: READ + NOTIFY
- **更新频率**: 2Hz (每500ms)
- **数据量**: ~250字节
- **用途**: 传输磁力计和罗盘方向数据

### 数据格式
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1234567890,
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
    "timestamp": 1234567890,
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

### 字段说明
- `heading`: 磁北方向角（度，0-360）
- `magnetic`: 磁场强度（μT）
- `declination`: 磁偏角（度）
- `inclination`: 磁倾角（度）
- `field_strength`: 总磁场强度（μT）
- `status`: 罗盘状态和校准信息

---

## 特征值4: 系统状态数据 (System Status Data)

### 基本信息
- **UUID**: `E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B`
- **属性**: READ + NOTIFY
- **更新频率**: 1Hz (每1秒)
- **数据量**: ~380字节
- **用途**: 传输系统状态、电源、通信和存储信息

### 数据格式
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 1234567890,
  "firmware": "v4.2.0+502",
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

### 字段说明
- `firmware/hardware`: 版本信息
- `power_mode`: 电源模式（0=省电, 1=正常, 2=高性能）
- `system`: 系统运行状态
- `modules`: 各功能模块就绪状态
- `storage`: 存储空间信息
- `network`: 网络连接状态

---

## 客户端使用指南

### 1. 按需订阅策略

#### 基础定位应用
```python
# 只需要位置信息
gps_char = service.get_characteristic("B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E")
await gps_char.start_notify(gps_handler)
```

#### 运动分析应用
```python
# 需要位置 + IMU数据
gps_char = service.get_characteristic("B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E")
imu_char = service.get_characteristic("C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F")

await gps_char.start_notify(gps_handler)
await imu_char.start_notify(imu_handler)
```

#### 导航应用
```python
# 需要位置 + 罗盘数据
gps_char = service.get_characteristic("B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E")
compass_char = service.get_characteristic("D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A")

await gps_char.start_notify(gps_handler)
await compass_char.start_notify(compass_handler)
```

#### 完整监控应用
```python
# 订阅所有特征值
characteristics = [
    "B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E",  # GPS
    "C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F",  # IMU
    "D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A",  # Compass
    "E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B"   # System
]

for char_uuid in characteristics:
    char = service.get_characteristic(char_uuid)
    await char.start_notify(data_handler)
```

### 2. 数据处理示例

```python
def gps_handler(sender, data):
    """处理GPS数据"""
    gps_data = json.loads(data.decode('utf-8'))
    location = gps_data.get('location', {})
    
    if location.get('valid', False):
        lat = location.get('lat')
        lng = location.get('lng')
        speed = location.get('speed')
        print(f"位置: {lat}, {lng}, 速度: {speed} km/h")

def imu_handler(sender, data):
    """处理IMU数据"""
    imu_data = json.loads(data.decode('utf-8'))
    attitude = imu_data.get('imu', {}).get('attitude', {})
    
    roll = attitude.get('roll')
    pitch = attitude.get('pitch')
    yaw = attitude.get('yaw')
    print(f"姿态: Roll={roll}°, Pitch={pitch}°, Yaw={yaw}°")

def compass_handler(sender, data):
    """处理罗盘数据"""
    compass_data = json.loads(data.decode('utf-8'))
    compass = compass_data.get('compass', {})
    
    heading = compass.get('heading')
    print(f"磁北方向: {heading}°")

def system_handler(sender, data):
    """处理系统状态数据"""
    system_data = json.loads(data.decode('utf-8'))
    system = system_data.get('system', {})
    
    battery = system.get('battery_percentage')
    uptime = system.get('uptime')
    print(f"电池: {battery}%, 运行时间: {uptime}s")
```

---

## 配置文件更新

### C++头文件定义
```cpp
// BLE服务和特征值UUID定义
#define BLE_SERVICE_UUID                  "A7B3C4D5-E6F7-4A8B-9C0D-1E2F3A4B5C6D"

// 特征值UUID
#define BLE_CHAR_GPS_UUID                 "B8C9D0E1-F2A3-4B5C-6D7E-8F9A0B1C2D3E"
#define BLE_CHAR_IMU_UUID                 "C9D0E1F2-A3B4-5C6D-7E8F-9A0B1C2D3E4F"
#define BLE_CHAR_COMPASS_UUID             "D0E1F2A3-B4C5-6D7E-8F9A-0B1C2D3E4F5A"
#define BLE_CHAR_SYSTEM_UUID              "E1F2A3B4-C5D6-7E8F-9A0B-1C2D3E4F5A6B"

// 更新频率定义
#define BLE_GPS_UPDATE_INTERVAL           200   // 5Hz
#define BLE_IMU_UPDATE_INTERVAL           100   // 10Hz
#define BLE_COMPASS_UPDATE_INTERVAL       500   // 2Hz
#define BLE_SYSTEM_UPDATE_INTERVAL        1000  // 1Hz
```

---

## 优势总结

### 1. 数据传输优化
- **数据量控制**: 每个特征值≤400字节，避免截断
- **按需订阅**: 客户端只订阅需要的数据
- **更新频率优化**: 根据数据重要性设置不同频率

### 2. 系统性能优化
- **减少BLE负载**: 避免不必要的数据传输
- **降低功耗**: 客户端可选择性订阅
- **提高稳定性**: 小数据包传输更可靠

### 3. 开发便利性
- **模块化设计**: 功能独立，便于维护
- **灵活订阅**: 支持多种应用场景
- **数据一致性**: 每个模块数据结构清晰

### 4. 应用场景适配
- **实时定位**: 只订阅GPS特征值
- **运动分析**: GPS + IMU特征值
- **导航应用**: GPS + 罗盘特征值
- **系统监控**: 系统状态特征值
- **完整监控**: 全部特征值

这种模块化设计既解决了数据量过大的问题，又提供了灵活的数据访问方式，是BLE数据传输的最佳实践。