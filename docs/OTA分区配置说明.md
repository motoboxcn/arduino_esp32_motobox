# ESP32 OTA升级分区配置说明

## 概述

为了支持OTA（Over-The-Air）固件升级功能，需要配置专门的分区表。轻量化版本使用自定义分区表 `partitions_ota.csv` 来支持OTA升级。

## 分区表配置

### 文件：`partitions_ota.csv`

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x5000,
otadata,  data, ota,     0xe000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x180000,
app1,     app,  ota_1,   0x190000,0x180000,
spiffs,   data, spiffs,  0x310000,0xF0000,
```

### 分区说明

| 分区名 | 类型 | 子类型 | 偏移地址 | 大小 | 说明 |
|--------|------|--------|----------|------|------|
| nvs | data | nvs | 0x9000 | 20KB | 非易失性存储，保存配置数据 |
| otadata | data | ota | 0xe000 | 8KB | OTA数据分区，记录当前活跃的app分区 |
| app0 | app | ota_0 | 0x10000 | 1.5MB | 应用程序分区0（主分区） |
| app1 | app | ota_1 | 0x190000 | 1.5MB | 应用程序分区1（OTA分区） |
| spiffs | data | spiffs | 0x310000 | 960KB | SPIFFS文件系统分区 |

## OTA工作原理

1. **双分区设计**：系统包含两个相同大小的app分区（app0和app1）
2. **活跃分区**：系统从其中一个分区启动，另一个分区用于接收新固件
3. **升级过程**：新固件写入非活跃分区，完成后切换启动分区
4. **回滚机制**：如果新固件启动失败，系统自动回滚到旧版本

## 编译结果

使用自定义OTA分区表后的编译结果：

- **RAM使用**: 16.4% (53,832 / 327,680 bytes)
- **Flash使用**: 92.7% (1,458,829 / 1,572,864 bytes)
- **可用空间**: 每个app分区1.5MB，足够容纳当前固件

## 配置方法

在 `platformio.ini` 中指定自定义分区表：

```ini
[env:esp32-air780eg]
platform = espressif32
board = esp32dev
framework = arduino
board_build.partitions = partitions_ota.csv
```

## 注意事项

1. **分区大小**：每个app分区必须足够大以容纳完整的固件
2. **对齐要求**：分区偏移地址必须按4KB对齐
3. **总大小限制**：所有分区总大小不能超过Flash容量（4MB）
4. **兼容性**：更改分区表需要完全擦除Flash重新烧录

## OTA升级支持

轻量化版本支持以下OTA升级方式：

1. **SD卡升级**：通过SD卡中的固件文件进行升级
2. **网络升级**：通过4G网络下载固件进行升级（预留功能）

## 故障排除

### 编译错误：程序大小超出限制

**问题**：`The program size (1458829 bytes) is greater than maximum allowed (1310720 bytes)`

**解决方案**：使用自定义分区表 `partitions_ota.csv`，提供更大的app分区空间（1.5MB vs 1.25MB）

### 分区表不兼容

**问题**：更改分区表后设备无法启动

**解决方案**：
1. 完全擦除Flash：`pio run --target erase`
2. 重新烧录固件：`pio run --target upload`

## 相关文件

- `partitions_ota.csv` - 自定义OTA分区表
- `src/ota/SDCardOTA.cpp` - SD卡OTA升级实现
- `src/ota/OTAManager.cpp` - OTA管理器
