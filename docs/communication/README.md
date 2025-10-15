# 通信系统文档

本目录包含ESP32-S3 MotoBox轻量化版本的4G通信系统相关文档。

## 4G通信模块
- `air780eg_lbs_integration_complete.md` - Air780EG LBS集成完整方案

## MQTT协议
- `handleMQTTURC_transfer_summary.md` - MQTT URC处理传输总结
- `mqtt_callback_fix_final.md` - MQTT回调修复最终方案
- `mqtt_callback_fix_test.md` - MQTT回调修复测试
- `mqtt_optimization_summary.md` - MQTT优化总结

## 核心特性
- **Air780EG 集成模块**：内置4G LTE + GNSS定位
- **MQTT 数据上报**：实时位置、状态数据上传至云端
- **低功耗设计**：支持休眠唤醒，延长电池续航

## 通信架构
```
设备数据 → MQTT协议 → 4G网络 → 云端服务
    ↓         ↓         ↓         ↓
传感器数据 → JSON格式 → Air780EG → 数据存储
状态信息   → 加密传输 → LTE网络  → 实时监控
```

## 数据上报内容
- 实时位置信息（经纬度、海拔）
- 运动状态（速度、方向、倾角）
- 设备状态（电量、信号强度）
- 环境数据（温度、湿度等）

## 远程管理
- **实时监控**：通过MQTT实时获取设备状态
- **远程配置**：支持参数调整和功能开关
- **告警通知**：异常状态实时推送
