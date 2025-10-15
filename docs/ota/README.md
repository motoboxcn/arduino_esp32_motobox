# OTA升级系统文档

本目录包含ESP32-S3 MotoBox轻量化版本的OTA固件升级系统相关文档。

## OTA实现
- `Multi_Firmware_OTA_Complete.md` - 多固件OTA完整方案
- `OTA_Implementation_Summary.md` - OTA实现总结
- `OTA分区配置说明.md` - OTA分区配置详细说明

## 固件管理
- `Firmware_Release_Guide.md` - 固件发布指南
- `Firmware_System_Complete.md` - 固件系统完整说明

## 核心功能
- **SD卡升级**：支持SD卡固件升级
- **远程升级**：支持4G网络OTA升级
- **版本管理**：固件版本控制和回滚
- **安全验证**：固件签名验证机制

## OTA特性
- **双分区设计**：A/B分区确保升级安全
- **断点续传**：网络中断后可继续下载
- **完整性校验**：MD5/SHA256校验确保文件完整
- **自动回滚**：升级失败自动回滚到上一版本

## 升级流程
```
检查更新 → 下载固件 → 验证签名 → 写入分区 → 重启验证
    ↓         ↓         ↓         ↓         ↓
版本比较   → 分块下载 → MD5校验  → 刷写Flash → 启动新固件
```

## 分区配置
| 分区名称 | 大小 | 用途 |
|----------|------|------|
| bootloader | 32KB | 引导程序 |
| partition_table | 4KB | 分区表 |
| ota_0 | 1.5MB | 主固件分区 |
| ota_1 | 1.5MB | 备份固件分区 |
| ota_data | 8KB | OTA状态数据 |
| nvs | 24KB | 配置存储 |

## 使用方法
1. 将固件文件放入SD卡`/firmware/`目录
2. 重启设备，系统自动检测并升级
3. 或通过MQTT命令触发远程升级
