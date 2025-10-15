# BLE特征值详细说明

## 概述

MotoBox ESP32设备通过BLE（蓝牙低功耗）提供三个特征值，用于不同的数据传输需求。每个特征值都有特定的用途和更新频率。

## 服务信息

- **服务UUID**: `12345678-1234-1234-1234-123456789ABC`
- **设备名称**: `MotoBox` 或 `MotoBox_XXXXXX`（带设备ID）
- **连接方式**: BLE GATT服务

---

## 特征值1: 遥测数据 (Telemetry Data)

### 基本信息
- **UUID**: `12345678-1234-1234-1234-123456789ABD`
- **属性**: READ + NOTIFY
- **更新频率**: 1Hz (每1秒)
- **用途**: 传输完整的设备状态信息，用于实时监控

### 数据格式
```json
{
  "device_id": "ESP32_ABC123",
  "timestamp": 12345678,
  "firmware": "v4.2.0+502",
  "hardware": "esp32-air780eg",
  "power_mode": 2,
  "location": {
    "lat": 39.904200,
    "lng": 116.407400,
    "altitude": 50.5,
    "speed": 25.30,
    "heading": 180.0,
    "satellites": 8,
    "hdop": 1.2,
    "timestamp": 12345678,
    "valid": true
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
      "yaw": 180.0,
      "timestamp": 12345678,
      "valid": true
    },
    "compass": {
      "heading": 185.2,
      "mag_x": 15.3,
      "mag_y": -8.7,
      "mag_z": 42.1,
      "timestamp": 12345678,
      "valid": true
    }
  },
  "system": {
    "battery_voltage": 3850,
    "battery_percentage": 85,
    "is_charging": false,
    "external_power": true,
    "signal_strength": 75,
    "uptime": 3600,
    "free_heap": 250000
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
    "size_mb": 32768,
    "free_mb": 16384
  }
}
```

### 字段说明
- `device_id`: 设备唯一标识符
- `timestamp`: 数据时间戳（毫秒）
- `firmware/hardware`: 固件和硬件版本
- `power_mode`: 电源模式（0=省电, 1=正常, 2=高性能）
- `location`: GPS位置信息
- `sensors`: 传感器数据（IMU、罗盘）
- `system`: 系统状态（电池、信号、内存等）
- `modules`: 各模块就绪状态
- `storage`: 存储空间信息

---

## 特征值2: 调试数据 (Debug Data)

### 基本信息
- **UUID**: `12345678-1234-1234-1234-123456789ABE`
- **属性**: READ + NOTIFY
- **更新频率**: 按需更新
- **用途**: 传输系统调试信息和状态消息

### 数据格式
```json
{
  "timestamp": 12345678,
  "level": "INFO",
  "module": "BLE",
  "message": "BLE客户端已连接",
  "data": {
    "client_count": 1,
    "connection_time": 1500
  }
}
```

### 示例数据

#### 连接状态消息
```json
{
  "timestamp": 12345678,
  "level": "INFO",
  "module": "BLE",
  "message": "BLE客户端已连接",
  "data": {
    "client_address": "AA:BB:CC:DD:EE:FF",
    "connection_time": 1500
  }
}
```

#### 错误消息
```json
{
  "timestamp": 12345679,
  "level": "ERROR",
  "module": "GPS",
  "message": "GPS信号丢失",
  "data": {
    "last_fix_time": 12345000,
    "satellites": 0
  }
}
```

#### 系统状态消息
```json
{
  "timestamp": 12345680,
  "level": "WARN",
  "module": "BATTERY",
  "message": "电池电量低",
  "data": {
    "voltage": 3500,
    "percentage": 15
  }
}
```

### 字段说明
- `timestamp`: 消息时间戳
- `level`: 日志级别（DEBUG, INFO, WARN, ERROR）
- `module`: 产生消息的模块名称
- `message`: 调试消息内容
- `data`: 附加的调试数据（可选）

---

## 特征值3: 融合调试数据 (Fusion Debug Data)

### 基本信息
- **UUID**: `12345678-1234-1234-1234-123456789ABF`
- **属性**: READ + NOTIFY
- **更新频率**: 5Hz (每200ms)
- **用途**: 传输融合定位系统的详细调试信息

### 数据格式
```json
{
  "timestamp": 12345678,
  "fusion_status": "active",
  "position": {
    "lat": 39.904200,
    "lng": 116.407400,
    "alt": 50.5,
    "speed": 25.30,
    "heading": 180.0,
    "accuracy": 2.5
  },
  "attitude": {
    "roll": 2.5,
    "pitch": -1.8,
    "yaw": 180.0
  },
  "fusion_info": {
    "position_valid": true,
    "accuracy": 2.5
  },
  "stats": {
    "uptime": 3600,
    "free_heap": 250000
  }
}
```

### 示例数据

#### 正常融合状态
```json
{
  "timestamp": 12345678,
  "fusion_status": "active",
  "position": {
    "lat": 39.904200,
    "lng": 116.407400,
    "alt": 50.5,
    "speed": 25.30,
    "heading": 180.0,
    "accuracy": 2.5
  },
  "attitude": {
    "roll": 2.5,
    "pitch": -1.8,
    "yaw": 180.0
  },
  "fusion_info": {
    "position_valid": true,
    "accuracy": 2.5
  },
  "stats": {
    "uptime": 3600,
    "free_heap": 250000
  }
}
```

#### 融合系统未初始化
```json
{
  "timestamp": 12345679,
  "fusion_status": "inactive",
  "error": "Fusion system not initialized"
}
```

#### 融合功能被禁用
```json
{
  "timestamp": 12345680,
  "fusion_status": "disabled",
  "error": "IMU fusion not enabled"
}
```

### 字段说明
- `timestamp`: 数据时间戳
- `fusion_status`: 融合状态（active/inactive/disabled）
- `position`: 融合后的位置信息
- `attitude`: 设备姿态角（横滚、俯仰、偏航）
- `fusion_info`: 融合算法相关信息
- `stats`: 系统统计信息

---

## 连接步骤

### 1. 扫描和连接
1. 扫描BLE设备，查找名称为 `MotoBox` 或 `MotoBox_XXXXXX` 的设备
2. 连接到该设备
3. 发现服务UUID: `12345678-1234-1234-1234-123456789ABC`

### 2. 订阅特征值
根据需求订阅相应的特征值：

```javascript
// 订阅遥测数据（1Hz更新）
await characteristic1.startNotifications();

// 订阅调试数据（按需更新）
await characteristic2.startNotifications();

// 订阅融合调试数据（5Hz更新）
await characteristic3.startNotifications();
```

### 3. 处理数据
```javascript
// 处理遥测数据
characteristic1.addEventListener('characteristicvaluechanged', (event) => {
  const data = JSON.parse(new TextDecoder().decode(event.target.value));
  console.log('遥测数据:', data);
});

// 处理调试数据
characteristic2.addEventListener('characteristicvaluechanged', (event) => {
  const data = JSON.parse(new TextDecoder().decode(event.target.value));
  console.log('调试消息:', data.message);
});

// 处理融合调试数据
characteristic3.addEventListener('characteristicvaluechanged', (event) => {
  const data = JSON.parse(new TextDecoder().decode(event.target.value));
  console.log('融合状态:', data.fusion_status);
});
```

---

## 使用建议

### 实时监控应用
- 主要订阅 **遥测数据特征值**
- 获取完整的设备状态信息
- 适合仪表盘、实时监控界面

### 调试和诊断应用
- 订阅 **调试数据特征值**
- 获取系统运行状态和错误信息
- 适合开发调试、故障诊断

### 融合定位分析应用
- 订阅 **融合调试特征值**
- 深入了解融合定位算法的工作状态
- 适合定位精度分析、算法优化

### 组合使用
- 可以同时订阅多个特征值
- 根据应用需求选择合适的数据流
- 注意数据更新频率，避免过度处理

---

## 注意事项

1. **数据格式**: 所有数据均为UTF-8编码的JSON字符串
2. **更新频率**: 不同特征值有不同的更新频率，请根据需求选择
3. **连接稳定性**: BLE连接可能因距离、干扰等因素中断
4. **数据完整性**: 建议在客户端实现数据校验和错误处理
5. **内存使用**: 高频数据更新可能影响设备性能，请合理使用

---

## 版本信息

- **文档版本**: v1.0
- **固件版本**: v4.2.0+502
