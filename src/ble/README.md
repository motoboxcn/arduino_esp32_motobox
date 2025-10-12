# BLE功能模块

## 概述

本模块实现了ESP32-S3的蓝牙BLE功能，允许客户端通过蓝牙连接获取设备的实时数据。

## 文件结构

```
src/ble/
├── BLEManager.h          # BLE管理器头文件
├── BLEManager.cpp        # BLE管理器实现
├── BLEDataProvider.h     # 数据提供者头文件
├── BLEDataProvider.cpp   # 数据提供者实现
└── README.md            # 本文件
```

## 核心组件

### 1. BLEManager
负责BLE服务器的管理，包括：
- BLE设备初始化
- 服务和特征值创建
- 客户端连接管理
- 数据广播

### 2. BLEDataProvider
负责从现有模块获取数据并转换为BLE格式：
- GPS位置数据
- 电池电量数据
- IMU倾角数据

## 数据特征值

| 特征值 | UUID | 数据类型 | 更新频率 | 描述 |
|--------|------|----------|----------|------|
| GPS位置 | `12345678-1234-1234-1234-123456789ABD` | JSON | 1秒 | 经纬度、海拔、速度、航向等 |
| 电池电量 | `12345678-1234-1234-1234-123456789ABE` | JSON | 5秒 | 电压、百分比、充电状态等 |
| IMU倾角 | `12345678-1234-1234-1234-123456789ABF` | JSON | 100ms | 姿态角、加速度、角速度等 |

## 使用方法

### 1. 启用BLE功能
在 `config.h` 中设置：
```cpp
#define ENABLE_BLE
```

### 2. 初始化
系统会自动初始化BLE功能，设备名称格式为 "MotoBox-{设备ID}"。

### 3. 客户端连接
客户端可以：
- 扫描设备名称 "MotoBox-{设备ID}"
- 连接到设备
- 订阅需要的数据特征值
- 接收实时数据更新

## 配置选项

### 编译时配置
```cpp
#define BLE_DEVICE_NAME_PREFIX            "MotoBox-"
#define BLE_SERVICE_UUID                  "12345678-1234-1234-1234-123456789ABC"
#define BLE_CHAR_GPS_UUID                 "12345678-1234-1234-1234-123456789ABD"
#define BLE_CHAR_BATTERY_UUID             "12345678-1234-1234-1234-123456789ABE"
#define BLE_CHAR_IMU_UUID                 "12345678-1234-1234-1234-123456789ABF"
#define BLE_UPDATE_INTERVAL               1000
#define BLE_DEBUG_ENABLED                 false
```

### 运行时配置
- 数据更新频率可调整
- 调试输出可开关
- 连接管理自动处理

## 功耗优化

1. **连接时停止广播**：客户端连接后自动停止BLE广播
2. **按需更新**：只有在有客户端连接时才推送数据
3. **数据变化检测**：只有数据变化时才推送更新

## 调试功能

### 启用调试
```cpp
#define BLE_DEBUG_ENABLED true
```

### 状态查询
- 连接状态
- 广播状态
- 数据有效性
- 客户端连接数

## 依赖关系

- ESP32 BLE库（内置）
- ArduinoJson（用于数据序列化）
- 现有模块：Air780EG、QMI8658、BAT

## 注意事项

1. **数据源依赖**：BLE功能依赖于GPS、IMU和电池模块
2. **连接管理**：系统自动管理客户端连接
3. **数据格式**：所有数据采用JSON格式
4. **时间戳**：数据包含时间戳便于同步

## 测试

使用提供的Python客户端示例进行测试：
```bash
cd examples
python ble_client_example.py
```

## 故障排除

### 常见问题
1. BLE初始化失败 → 检查ESP32支持
2. 客户端无法连接 → 检查设备名称和UUID
3. 数据不更新 → 检查数据源和订阅状态

### 调试步骤
1. 启用调试输出
2. 检查串口日志
3. 验证数据源状态
4. 测试客户端连接
