# 电源管理文档

本目录包含ESP32-S3 MotoBox轻量化版本的电源管理相关文档。

## 核心文档
- `PowerModeManager.md` - 电源模式管理器详细说明
- `低功耗设计.md` - 低功耗设计方案和实现
- `power_management_optimization.md` - 电源管理优化方案

## 实现指南
- `PowerMode_Build_Guide.md` - 电源模式构建指南
- `power_manager_refactor.md` - 电源管理器重构说明
- `vehicle_power_detection.md` - 车辆电源检测实现

## 优化方案
- `power_optimization_sd_card.md` - SD卡相关的电源优化
- `power_optimization_sd_card.md` - SD卡电源优化方案

## 功能特性
- **双电源模式**：电池供电 + 车辆电门供电
- **智能休眠**：静止状态自动进入深度睡眠
- **震动唤醒**：IMU检测震动自动唤醒系统
- **电量监控**：实时电池状态监测与低电量保护

## 工作模式
| 模式 | 功能 | 功耗 | 适用场景 |
|------|------|------|----------|
| **活跃模式** | 全功能运行 | 高 | 行驶中、数据采集 |
| **待机模式** | 基础监控 | 中 | 停车、等待状态 |
| **休眠模式** | 震动唤醒 | 极低 | 长时间静止 |
