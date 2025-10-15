# BLE功能构建修复总结

## 问题概述

在实现BLE功能后，构建过程中遇到了几个编译错误，主要涉及：
1. 数据源类型不匹配
2. BLE回调类名称冲突
3. API方法不存在

## 修复内容

### 1. IMU类型修复

**问题**：`BLEDataProvider` 期望 `QMI8658*` 类型，但实际传入的是 `IMU*` 类型。

**原因**：`IMU` 是对 `SensorQMI8658` 的封装类，`QMI8658` 只是前向声明。

**修复**：
- 将 `BLEDataProvider` 中的所有 `QMI8658*` 改为 `IMU*`
- 使用 `IMU` 类的公共方法获取数据：
  - `getAccelX()`, `getAccelY()`, `getAccelZ()`
  - `getGyroX()`, `getGyroY()`, `getGyroZ()`
  - `getPitch()`, `getRoll()`, `getYaw()`

### 2. GPS数据获取修复

**问题**：`Air780EG` 类没有直接的 GPS 数据获取方法。

**解决方案**：直接从 `device_state` 获取GPS数据，这些数据已由系统维护和更新。

```cpp
cachedGPSData.latitude = device_state.latitude;
cachedGPSData.longitude = device_state.longitude;
cachedGPSData.satellites = device_state.satellites;
cachedGPSData.valid = device_state.gnssReady;
```

### 3. 电池数据获取修复

**问题**：`BAT` 类没有 `isExternalPowerConnected()` 方法。

**解决方案**：从 `device_state` 获取外部电源状态。

```cpp
cachedBatteryData.external_power = device_state.external_power;
```

### 4. BLE回调类名称冲突修复

**问题**：ESP32 BLE库已经定义了 `BLEServerCallbacks` 和 `BLECharacteristicCallbacks` 类。

**解决方案**：使用自定义名称避免冲突：
- `BLEServerCallbacks` → `MotoBoxBLEServerCallbacks`
- `BLECharacteristicCallbacks` → `MotoBoxBLECharacteristicCallbacks`

同时简化回调方法：
- 移除 `onSubscribe()` 和 `onUnsubscribe()`（签名不匹配）
- 保留 `onRead()` 和 `onWrite()` 方法

## 文件修改清单

### 修改的文件

1. **src/ble/BLEDataProvider.h**
   - 将 `QMI8658*` 改为 `IMU*`
   - 更新 `setDataSources()` 方法签名

2. **src/ble/BLEDataProvider.cpp**
   - 更新构造函数
   - 修复 `convertGPSData()`：从 `device_state` 获取数据
   - 修复 `convertBatteryData()`：从 `device_state` 获取外部电源状态
   - 修复 `convertIMUData()`：使用 `IMU` 类方法

3. **src/ble/BLEManager.h**
   - 重命名回调类：`MotoBoxBLEServerCallbacks`
   - 重命名回调类：`MotoBoxBLECharacteristicCallbacks`
   - 简化回调方法

4. **src/ble/BLEManager.cpp**
   - 更新回调类实现
   - 使用新的回调类名称

5. **src/device.cpp**
   - 无需修改（已正确传入 `&imu`）

## 数据获取策略

### GPS数据
- **来源**：`device_state`（全局设备状态）
- **优势**：
  - 避免直接依赖 `Air780EG` 库的API
  - 数据已由系统维护，保证一致性
  - 支持融合定位数据

### 电池数据
- **来源**：`BAT` 类 + `device_state`
  - 电压、百分比、充电状态：从 `BAT` 类
  - 外部电源状态：从 `device_state`
- **优势**：充分利用现有数据源

### IMU数据
- **来源**：`IMU` 类
- **方法**：使用公共访问器方法
- **优势**：类型安全，API清晰

## 依赖关系

```
BLEManager
  └─ ESP32 BLE Arduino (内置库)

BLEDataProvider
  ├─ device_state (全局状态)
  ├─ BAT 类
  └─ IMU 类
```

## 编译状态

- ✅ BLE相关代码编译通过
- ✅ 依赖库正确识别（ESP32 BLE Arduino @ 2.0.0）
- ⚠️ FusionLocation库有独立的编译问题（与BLE无关）

## 后续工作

1. 测试BLE连接和数据传输
2. 验证GPS、电池、IMU数据的正确性
3. 优化数据更新频率
4. 添加更多数据字段（如速度、航向等）

## 注意事项

1. **数据来源**：BLE数据主要从 `device_state` 获取，确保系统正确更新这些状态
2. **回调安全**：BLE回调在独立线程中执行，注意线程安全
3. **内存管理**：BLE特征值使用JSON字符串，注意内存使用
4. **设备名称**：设备名称格式为 `MotoBox-{设备ID}`，确保唯一性
