# 存储系统文档

本目录包含ESP32-S3 MotoBox轻量化版本的SD卡存储系统相关文档。

## SD卡管理
- `SD_Directory_Structure.md` - SD卡目录结构说明
- `SD_Simplification_Summary.md` - SD卡简化总结
- `SD_Commands.md` - SD卡命令说明
- `sdcard_optimization.md` - SD卡优化方案

## OTA升级
- `SD_OTA_Complete.md` - SD卡OTA升级完整方案

## 硬件规格
- `sdcard/` - SD卡连接器相关硬件文档

## 核心功能
- **数据记录**：轨迹数据、状态日志本地存储
- **固件升级**：支持SD卡固件升级
- **配置管理**：设备配置文件存储
- **日志系统**：调试和运行日志记录

## 目录结构
```
SD卡根目录/
├── firmware/          # 固件文件
├── config/            # 配置文件
├── logs/              # 日志文件
├── tracks/            # 轨迹数据
└── temp/              # 临时文件
```

## 存储特性
- **低功耗访问**：优化SD卡读写功耗
- **数据完整性**：文件系统保护机制
- **容量管理**：自动清理过期数据
- **热插拔支持**：支持运行时SD卡更换
