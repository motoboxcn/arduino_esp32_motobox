#include "BLEDataProvider.h"

#ifdef ENABLE_BLE

#include "utils/DebugUtils.h"

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
                         deviceState->location.valid ? "是" : "否",
                         deviceState->sensors.imu.valid ? "是" : "否",
                         deviceState->system.battery_voltage);
            #endif
        } else {
            dataValid = false;
            
            #ifdef BLE_DEBUG_ENABLED
            Serial.println("[BLE数据提供者] 数据源未设置");
            #endif
        }
    }
}

void BLEDataProvider::setDeviceState(const ble_device_state_t* state) {
    deviceState = state;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 设备状态已设置");
    #endif
}

void BLEDataProvider::setDeviceStateFromGlobal() {
    // 从全局device_state转换数据
    // 注意：这里需要访问全局device_state，但由于循环包含问题，
    // 我们暂时使用一个简化的实现
    
    // 初始化转换后的状态
    memset(&convertedState, 0, sizeof(convertedState));
    
    // 设置基本设备信息
    convertedState.device_id = "ESP32_XXXXXX";  // 临时值
    convertedState.timestamp = millis();
    convertedState.firmware = "v4.2.0+495";
    convertedState.hardware = "esp32-air780eg";
    convertedState.power_mode = 2;
    
    // 设置模块状态（临时值）
    convertedState.modules.wifi_ready = false;
    convertedState.modules.ble_ready = true;
    convertedState.modules.gsm_ready = true;
    convertedState.modules.gnss_ready = false;
    convertedState.modules.imu_ready = true;
    convertedState.modules.compass_ready = false;
    convertedState.modules.sd_ready = false;
    convertedState.modules.audio_ready = false;
    
    // 设置系统状态（临时值）
    convertedState.system.battery_voltage = 3800;
    convertedState.system.battery_percentage = 80;
    convertedState.system.is_charging = true;
    convertedState.system.external_power = true;
    convertedState.system.signal_strength = 85;
    convertedState.system.uptime = millis() / 1000;
    convertedState.system.free_heap = ESP.getFreeHeap();
    
    // 设置存储信息（临时值）
    convertedState.storage.size_mb = 32768;
    convertedState.storage.free_mb = 16384;
    
    deviceState = &convertedState;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 从全局状态转换完成");
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
                     deviceState->location.valid ? "有效" : "无效",
                     deviceState->location.lat,
                     deviceState->location.lng);
        
        Serial.printf("IMU数据: %s (俯仰=%.1f°, 横滚=%.1f°)\n",
                     deviceState->sensors.imu.valid ? "有效" : "无效",
                     deviceState->sensors.imu.pitch,
                     deviceState->sensors.imu.roll);
        
        Serial.printf("电池状态: %dmV, %d%%, 充电:%s\n",
                     deviceState->system.battery_voltage,
                     deviceState->system.battery_percentage,
                     deviceState->system.is_charging ? "是" : "否");
    }
    
    Serial.println("========================");
}

#endif // ENABLE_BLE
