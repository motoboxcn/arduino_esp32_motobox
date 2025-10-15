# 融合定位系统

## 概述

ESP32-S3 MotoBox 项目集成了多传感器融合定位系统，通过融合IMU（惯性测量单元）、GPS（通过Air780EG模块）和地磁计（罗盘）的数据，提供高精度、高可靠性的实时位置信息。

## 系统架构

### 传感器组合
- **IMU (QMI8658)**: 提供加速度和陀螺仪数据，用于短期位置预测和姿态估计
- **GPS (Air780EG内置GNSS)**: 提供绝对位置参考，校正累积误差
- **地磁计 (QMC5883L)**: 提供航向参考，校正航向漂移

### 融合算法
- 基于简化卡尔曼滤波器的数据融合
- GPS数据用于位置校正，防止IMU积分漂移
- IMU数据提供高频位置更新（100Hz）
- 地磁计数据校正航向角漂移

## 功能特点

### 🎯 高精度定位
- GPS信号良好时：精度可达5-10米
- GPS信号丢失时：依靠IMU继续提供位置估计
- 多传感器融合：提高整体定位可靠性

### 🚀 高频更新
- 融合定位更新频率：10Hz（可配置）
- IMU数据采样：100Hz
- GPS数据更新：1Hz
- 地磁计数据：10Hz

### 🛡️ 容错设计
- 传感器故障时系统仍能工作
- 渐进式配置：从基本IMU到完整三传感器融合
- 自动检测传感器可用性

## 使用场景

### 🏍️ 摩托车导航
- 城市道路：GPS+IMU提供连续定位
- 隧道/地下：IMU接管位置估计
- 高速行驶：高频更新保证轨迹平滑

### 🚗 车载应用
- 停车场导航：GPS信号弱时依靠IMU
- 立交桥：多层道路准确定位
- 恶劣天气：多传感器提高可靠性

## 配置参数

```cpp
// 融合定位配置 (config.h)
#define FUSION_LOCATION_UPDATE_INTERVAL  100     // 更新间隔（毫秒）
#define FUSION_LOCATION_DEBUG_ENABLED    true    // 调试输出
#define FUSION_LOCATION_INITIAL_LAT      39.9042 // 初始纬度
#define FUSION_LOCATION_INITIAL_LNG      116.4074// 初始经度
```

## API 使用

### 基本使用
```cpp
#include "location/FusionLocationManager.h"

void setup() {
    // 初始化融合定位系统
    if (fusionLocationManager.begin()) {
        Serial.println("融合定位系统初始化成功");
        fusionLocationManager.setDebug(true);
    }
}

void loop() {
    // 更新融合定位
    fusionLocationManager.loop();
    
    // 获取位置信息
    Position pos = fusionLocationManager.getFusedPosition();
    if (pos.valid) {
        Serial.printf("位置: %.6f, %.6f\n", pos.lat, pos.lng);
        Serial.printf("精度: %.1fm, 航向: %.1f°\n", pos.accuracy, pos.heading);
    }
}
```

### 高级功能
```cpp
// 设置初始位置
fusionLocationManager.setInitialPosition(39.9042, 116.4074);

// 设置更新间隔
fusionLocationManager.setUpdateInterval(50); // 20Hz

// 获取详细状态
auto status = fusionLocationManager.getDataSourceStatus();
Serial.printf("传感器状态 - IMU:%s GPS:%s MAG:%s\n",
             status.imu_available ? "✅" : "❌",
             status.gps_available ? "✅" : "❌", 
             status.mag_available ? "✅" : "❌");

// 获取JSON格式数据
String json = fusionLocationManager.getPositionJSON();
Serial.println(json);
```

## 数据结构

### Position 结构体
```cpp
struct Position {
    double lat, lng;           // 融合后的经纬度
    float altitude;            // 高度
    float accuracy;            // 位置精度估计 (米)
    float heading;             // 航向角 (度, 0-360)
    float speed;               // 速度估计 (m/s)
    uint32_t timestamp;        // 时间戳
    bool valid;                // 数据有效性
    
    struct {
        bool hasGPS;           // 是否有GPS数据
        bool hasIMU;           // 是否有IMU数据
        bool hasMag;           // 是否有地磁计数据
    } sources;
};
```

## 性能指标

### 精度表现
- **GPS可用时**: 5-10米精度
- **GPS丢失后1分钟**: 10-50米精度
- **GPS丢失后5分钟**: 50-200米精度
- **航向精度**: ±5度（有地磁计校正）

### 资源消耗
- **内存占用**: ~8KB RAM
- **CPU占用**: ~2%（ESP32-S3 @ 240MHz）
- **更新延迟**: <10ms

## 调试和监控

### 调试输出
```cpp
fusionLocationManager.setDebug(true);
```

### 状态监控
```cpp
// 打印系统状态
fusionLocationManager.printStatus();

// 打印统计信息
fusionLocationManager.printStats();
```

### 串口命令
- `fusion.status` - 显示融合定位状态
- `fusion.stats` - 显示统计信息
- `fusion.reset` - 重置统计信息

## 故障排除

### 常见问题

1. **位置不更新**
   - 检查IMU是否正常工作
   - 确认初始位置已设置
   - 查看调试输出

2. **精度较差**
   - 检查GPS信号质量
   - 确认地磁计校准
   - 调整融合参数

3. **航向漂移**
   - 校准地磁计
   - 检查磁干扰源
   - 调整地磁计权重

### 调试步骤
1. 启用调试输出
2. 检查各传感器状态
3. 观察数据源标识
4. 分析精度变化趋势

## 未来改进

### 计划功能
- [ ] 支持多GPS天线
- [ ] 增加气压计高度融合
- [ ] 实现地图匹配算法
- [ ] 添加轨迹预测功能

### 算法优化
- [ ] 扩展卡尔曼滤波器
- [ ] 自适应噪声模型
- [ ] 机器学习辅助校正
- [ ] 多模态传感器融合

## 参考资料

- [FusionLocation库文档](../lib/FusionLocation/README.md)
- [QMI8658 IMU规格书](https://www.qstcorp.com/upload/pdf/202003/QMI8658A%20Datasheet%20Rev.%20A.pdf)
- [QMC5883L地磁计规格书](https://datasheet.lcsc.com/szlcsc/QST-QMC5883L_C192585.pdf)
- [Air780EG GNSS功能说明](https://docs.openluat.com/air780eg/at/app/at_command/#gps)
