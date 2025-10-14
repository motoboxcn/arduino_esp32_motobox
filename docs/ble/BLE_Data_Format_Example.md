# BLE数据格式说明

## 订阅信息

### 设备名称
- **格式**: `MotoBox-{设备ID}`
- **示例**: `MotoBox-ESP32_XXXXXX`

### 服务UUID
- **主服务**: `12345678-1234-1234-1234-123456789ABC`

### 特征值UUID
- **遥测数据特征值**: `12345678-1234-1234-1234-123456789ABD`

## 数据格式

BLE传输的数据为JSON格式，包含以下结构：

```json
{
  "device_id": "ESP32_XXXXXX",
  "timestamp": 12345678,
  "firmware": "v4.2.0+498",
  "hardware": "esp32-air780eg",
  "power_mode": 2,
  
  "location": {
    "lat": 39.9042,
    "lng": 116.4074,
    "alt": 50.5,
    "speed": 25.3,
    "course": 180.0,
    "satellites": 8,
    "hdop": 1.2,
    "timestamp": 12345678
  },
  
  "sensors": {
    "imu": {
      "accel_x": 0.1,
      "accel_y": -0.2,
      "accel_z": 9.8,
      "gyro_x": 0.05,
      "gyro_y": -0.03,
      "gyro_z": 0.01,
      "roll": 2.5,
      "pitch": -1.8,
      "yaw": 45.2,
      "timestamp": 12345678
    },
    
    "compass": {
      "heading": 45.2,
      "mag_x": 25.3,
      "mag_y": -15.7,
      "mag_z": 8.9,
      "timestamp": 12345678
    }
  },
  
  "system": {
    "battery": 3800,
    "battery_pct": 80,
    "charging": true,
    "external_power": true,
    "signal": 85,
    "uptime": 3600,
    "free_heap": 250000
  },
  
  "modules": {
    "wifi": false,
    "ble": true,
    "gsm": true,
    "gnss": false,
    "imu": true,
    "compass": false,
    "sd": false,
    "audio": false
  },
  
  "storage": {
    "size_mb": 32768,
    "free_mb": 16384
  }
}
```

## 数据更新频率

- **IMU数据**: 5Hz (每200ms更新一次)
- **融合数据**: 5Hz (每200ms更新一次)
- **系统状态**: 0.5Hz (每2秒更新一次)

## 客户端连接步骤

1. **扫描设备**: 查找名称以 `MotoBox-` 开头的BLE设备
2. **连接设备**: 连接到目标设备
3. **发现服务**: 查找服务UUID `12345678-1234-1234-1234-123456789ABC`
4. **订阅特征值**: 订阅特征值UUID `12345678-1234-1234-1234-123456789ABD`
5. **接收数据**: 监听特征值通知，接收JSON格式的遥测数据

## 数据字段说明

### 设备信息
- `device_id`: 设备唯一标识符
- `timestamp`: 数据生成时间戳（毫秒）
- `firmware`: 固件版本
- `hardware`: 硬件版本
- `power_mode`: 电源模式

### 位置数据 (location)
- `lat`: 纬度
- `lng`: 经度
- `alt`: 海拔高度（米）
- `speed`: 速度（km/h）
- `course`: 航向角（度）
- `satellites`: 卫星数量
- `hdop`: 水平精度因子
- `timestamp`: 位置数据时间戳

### 传感器数据 (sensors)

#### IMU数据 (imu)
- `accel_x/y/z`: 加速度计数据（m/s²）
- `gyro_x/y/z`: 陀螺仪数据（rad/s）
- `roll/pitch/yaw`: 姿态角（度）
- `timestamp`: IMU数据时间戳

#### 罗盘数据 (compass)
- `heading`: 磁航向（度）
- `mag_x/y/z`: 磁力计数据（μT）
- `timestamp`: 罗盘数据时间戳

### 系统状态 (system)
- `battery`: 电池电压（mV）
- `battery_pct`: 电池电量百分比
- `charging`: 是否正在充电
- `external_power`: 是否有外部电源
- `signal`: 信号强度
- `uptime`: 运行时间（秒）
- `free_heap`: 可用内存（字节）

### 模块状态 (modules)
- `wifi/ble/gsm/gnss/imu/compass/sd/audio`: 各模块就绪状态

### 存储信息 (storage)
- `size_mb`: 存储总容量（MB）
- `free_mb`: 可用存储空间（MB）

## 注意事项

1. **数据有效性**: 某些字段可能为空或无效，客户端应检查数据有效性
2. **更新频率**: 不同数据类型的更新频率不同，客户端应根据需要处理
3. **JSON解析**: 客户端需要解析JSON格式的数据
4. **连接管理**: 客户端应处理连接断开和重连逻辑
5. **数据缓存**: 建议客户端缓存最新数据，避免数据丢失
