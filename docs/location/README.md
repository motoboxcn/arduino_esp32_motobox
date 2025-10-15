# 定位系统文档

本目录包含ESP32-S3 MotoBox轻量化版本的智能定位系统相关文档。

## 融合定位
- `FusionLocation.md` - IMU + GNSS 融合定位系统说明
- `fallback_location_example.md` - 定位回退机制示例

## IMU传感器
- `IMU_Frequency_Optimization.md` - IMU频率优化方案
- `IMU_High_Frequency_Update_Optimization.md` - IMU高频更新优化
- `gravity_compensation_explanation.md` - 重力补偿算法说明

## 罗盘功能
- `compass_usage.md` - 罗盘功能使用说明
- `compass_example.cpp` - 罗盘功能示例代码
- `compass_refactor_summary.md` - 罗盘重构总结
- `compass_pin_fix.md` - 罗盘引脚修复说明

## GNSS定位
- `GNSS_Stack_Overflow_Fix.md` - GNSS栈溢出修复
- `GNSS_Functionality_Restoration.md` - GNSS功能恢复说明
- `GNSS_URC_Control_Solution.md` - GNSS URC控制方案
- `GPS_DEBUG_GUIDE.md` - GPS调试指南

## 核心特性
- **IMU + GNSS 融合定位**：QMI8658A 9轴传感器 + 卫星定位
- **骑行轨迹记录**：实时记录行驶路径，支持历史轨迹回放
- **倾角统计**：实时监测车辆倾斜角度
- **运动状态检测**：行驶/静止状态智能识别
- **罗盘功能**：方向检测和导航辅助

## 技术架构
```
传感器数据 → 数据融合算法 → 位置输出
    ↓              ↓           ↓
QMI8658A IMU → 卡尔曼滤波 → 精确位置
GNSS接收机   → 状态估计   → 运动状态
```
