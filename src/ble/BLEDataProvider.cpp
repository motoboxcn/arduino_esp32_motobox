#include "BLEDataProvider_Simple.h"

#ifdef ENABLE_BLE

#include "utils/DebugUtils.h"
#include "device.h"  // 包含device_state_t定义

const char* BLEDataProvider::TAG = "BLEDataProvider";

// 全局BLE数据提供者实例
BLEDataProvider bleDataProvider;

BLEDataProvider::BLEDataProvider()
    : deviceState(nullptr)
    , lastUpdateTime(0)
    , dataValid(false)
{
}

BLEDataProvider::~BLEDataProvider() {
    // 清理资源
}

void BLEDataProvider::begin() {
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 开始初始化...");
    #endif
    
    lastUpdateTime = 0;
    dataValid = false;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] ✅ 初始化完成");
    #endif
}

void BLEDataProvider::update() {
    unsigned long currentTime = millis();
    
    // 每200ms更新一次（5Hz）
    if (currentTime - lastUpdateTime >= BLE_UPDATE_INTERVAL) {
        lastUpdateTime = currentTime;
        
        // 检查数据源是否有效
        if (deviceState != nullptr) {
            dataValid = true;
            
            #ifdef BLE_DEBUG_ENABLED
            Serial.printf("[BLE数据提供者] 数据已更新: 位置有效=%s, IMU有效=%s, 电池=%dmV\n",
                         deviceState->telemetry.location.valid ? "是" : "否",
                         deviceState->telemetry.sensors.imu.valid ? "是" : "否",
                         deviceState->telemetry.system.battery_voltage);
            #endif
        } else {
            dataValid = false;
            
            #ifdef BLE_DEBUG_ENABLED
            Serial.println("[BLE数据提供者] 数据源未设置");
            #endif
        }
    }
}

void BLEDataProvider::setDeviceState(const device_state_t* state) {
    deviceState = state;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 设备状态已设置");
    #endif
}

void BLEDataProvider::setDebug(bool enable) {
    // 调试功能通过编译时宏控制
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE数据提供者] 调试模式: %s\n", enable ? "启用" : "禁用");
    #endif
}

void BLEDataProvider::printDataStatus() {
    Serial.println("=== BLE数据提供者状态 ===");
    Serial.printf("数据源: %s\n", deviceState ? "已设置" : "未设置");
    Serial.printf("数据有效: %s\n", dataValid ? "是" : "否");
    
    if (deviceState && dataValid) {
        Serial.printf("位置数据: %s (%.6f, %.6f)\n", 
                     deviceState->telemetry.location.valid ? "有效" : "无效",
                     deviceState->telemetry.location.lat,
                     deviceState->telemetry.location.lng);
        
        Serial.printf("IMU数据: %s (俯仰=%.1f°, 横滚=%.1f°)\n",
                     deviceState->telemetry.sensors.imu.valid ? "有效" : "无效",
                     deviceState->telemetry.sensors.imu.pitch,
                     deviceState->telemetry.sensors.imu.roll);
        
        Serial.printf("电池状态: %dmV, %d%%, 充电:%s\n",
                     deviceState->telemetry.system.battery_voltage,
                     deviceState->telemetry.system.battery_percentage,
                     deviceState->telemetry.system.is_charging ? "是" : "否");
    }
    
    Serial.println("========================");
}

#endif // ENABLE_BLE
