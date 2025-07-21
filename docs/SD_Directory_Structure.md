# SD卡目录结构定义

## 概述
本文档描述了ESP32-S3 MotoBox项目中SD卡的目录结构定义和相关宏。所有目录和文件路径都在`src/SD/SDManager.h`中定义为宏，便于维护和修改。

## 目录结构宏定义

### 根目录文件
```cpp
#define SD_DEVICE_INFO_FILE     "/deviceinfo.txt"    // 设备信息文件
```

### 主要目录
```cpp
#define SD_UPDATES_DIR          "/updates"           // 升级包目录
#define SD_DATA_DIR             "/data"              // 数据存储目录
#define SD_VOICE_DIR            "/voice"             // 语音文件目录
#define SD_LOGS_DIR             "/logs"              // 日志文件目录
#define SD_CONFIG_DIR           "/config"            // 配置文件目录
```

### 数据子目录
```cpp
#define SD_GPS_DATA_DIR         "/data/gps"          // GPS数据存储
#define SD_SENSOR_DATA_DIR      "/data/sensor"       // 传感器数据存储
#define SD_SYSTEM_DATA_DIR      "/data/system"       // 系统数据存储
```

### 配置文件
```cpp
#define SD_WIFI_CONFIG_FILE     "/config/wifi.json"  // WiFi配置
#define SD_MQTT_CONFIG_FILE     "/config/mqtt.json"  // MQTT配置
#define SD_DEVICE_CONFIG_FILE   "/config/device.json" // 设备配置
```

### 特殊文件
```cpp
#define SD_WELCOME_VOICE_FILE   "/voice/welcome.wav" // 欢迎语音文件
#define SD_FIRMWARE_FILE        "/updates/firmware.bin" // 固件升级包
#define SD_UPDATE_INFO_FILE     "/updates/update_info.json" // 升级信息
```

### 日志文件
```cpp
#define SD_SYSTEM_LOG_FILE      "/logs/system.log"   // 系统日志
#define SD_ERROR_LOG_FILE       "/logs/error.log"    // 错误日志
#define SD_GPS_LOG_FILE         "/logs/gps.log"      // GPS日志
```

## 目录结构树形图
```
SD卡根目录/
├── deviceinfo.txt              # 设备信息文件
├── config/                     # 配置文件目录
│   ├── wifi.json              # WiFi配置
│   ├── mqtt.json              # MQTT配置
│   └── device.json            # 设备配置
├── data/                       # 数据存储目录
│   ├── gps/                   # GPS数据
│   ├── sensor/                # 传感器数据
│   └── system/                # 系统数据
├── updates/                    # 升级包目录
│   ├── firmware.bin           # 固件升级包
│   └── update_info.json       # 升级信息
├── voice/                      # 语音文件目录
│   └── welcome.wav            # 欢迎语音文件
└── logs/                       # 日志文件目录
    ├── system.log             # 系统日志
    ├── error.log              # 错误日志
    └── gps.log                # GPS日志
```

## 使用方法

### 在代码中使用宏
```cpp
// 保存设备信息到根目录
bool success = writeFile(SD_DEVICE_INFO_FILE, deviceInfo);

// 记录GPS数据到指定目录
String gpsFile = String(SD_GPS_DATA_DIR) + "/gps_session.geojson";

// 检查语音文件是否存在
if (fileExists(SD_WELCOME_VOICE_FILE)) {
    // 处理语音文件
}
```

### 串口命令（简化版）
SDManager提供了简化的串口命令接口，避免与主命令系统重复：

#### 基本信息查询
- `sd.info` - 显示SD卡详细信息（容量、使用率等）
- `sd.status` - 检查SD卡和关键目录状态
- `sd.help` - 显示简化命令帮助

#### 目录结构查看
- `sd.tree` - 显示目录树结构（最多3层）
- `sd.structure` - 显示完整的目录结构定义

#### 系统操作
- `sd.fmt` - 显示格式化说明（ESP32不支持直接格式化）
- `sd.init` - 重新初始化SD卡并创建目录结构

#### 注意事项
- 更多文件操作功能（如ls、cat、mkdir、rm等）请使用主命令系统的`help`查看
- 简化版命令专注于SD卡状态查询和基本维护功能
- 避免了与`serialCommand.cpp`中命令处理的重复

## 自动创建的目录
系统启动时会自动创建以下目录结构：
- `/data` - 数据存储根目录
- `/data/gps` - GPS数据存储
- `/data/sensor` - 传感器数据存储
- `/data/system` - 系统数据存储
- `/config` - 配置文件目录
- `/updates` - 升级包目录
- `/voice` - 语音文件目录
- `/logs` - 日志文件目录

## 文件用途说明

### 设备信息文件 (`/deviceinfo.txt`)
存储设备的基本信息，包括：
- 设备ID
- 固件版本
- 硬件版本
- 创建时间
- 最后更新时间
- 启动次数
- SD卡容量信息

### GPS数据文件 (`/data/gps/`)
存储GPS轨迹数据，文件格式为GeoJSON：
- 文件命名格式：`gps_YYYYMMDD_HHMMSS_bootN.geojson`
- 包含GPS坐标、时间戳、速度等信息

### 配置文件 (`/config/`)
存储各种系统配置：
- `wifi.json` - WiFi连接配置
- `mqtt.json` - MQTT服务器配置
- `device.json` - 设备运行配置

### 升级文件 (`/updates/`)
存储固件升级相关文件：
- `firmware.bin` - 新固件文件
- `update_info.json` - 升级信息和版本说明

## 注意事项
1. 所有路径宏都以`SD_`前缀开头，便于识别
2. 目录会在SD卡初始化时自动创建
3. 如果目录创建失败，系统会记录错误日志
4. 建议在修改目录结构时更新此文档
