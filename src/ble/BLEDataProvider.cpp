#include "BLEDataProvider.h"

#ifdef ENABLE_BLE

#include "device.h"
#include "utils/DebugUtils.h"

#ifdef ENABLE_IMU_FUSION
#include "location/FusionLocationManager.h"
#endif

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
        
        // 从全局状态重新同步数据
        setDeviceStateFromGlobal();
        
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
    // 通过外部函数获取全局device_state指针
    extern device_state_t device_state;
    
    // 初始化转换后的状态
    memset(&convertedState, 0, sizeof(convertedState));
    
    // 设置基本设备信息
    convertedState.device_id = device_state.device_id;
    convertedState.timestamp = millis();
    convertedState.firmware = device_state.device_firmware_version;
    convertedState.hardware = device_state.device_hardware_version;
    convertedState.power_mode = device_state.power_mode;
    
    // 设置位置数据
    convertedState.location.lat = device_state.telemetry.location.lat;
    convertedState.location.lng = device_state.telemetry.location.lng;
    convertedState.location.type = device_state.telemetry.location.type;
    convertedState.location.gps_time_string = device_state.telemetry.location.gps_time_string;
    convertedState.location.altitude = device_state.telemetry.location.altitude;
    convertedState.location.speed = device_state.telemetry.location.speed;
    convertedState.location.heading = device_state.telemetry.location.heading;
    convertedState.location.satellites = device_state.telemetry.location.satellites;
    convertedState.location.hdop = device_state.telemetry.location.hdop;
    convertedState.location.vdop = 1.8f; // 默认值
    convertedState.location.pdop = 2.1f; // 默认值
    convertedState.location.fix_type = device_state.telemetry.location.valid ? 3 : 0;
    convertedState.location.valid = device_state.telemetry.location.valid;
    
    // 设置IMU传感器数据
    convertedState.sensors.imu.accel_x = device_state.telemetry.sensors.imu.accel_x;
    convertedState.sensors.imu.accel_y = device_state.telemetry.sensors.imu.accel_y;
    convertedState.sensors.imu.accel_z = device_state.telemetry.sensors.imu.accel_z;
    convertedState.sensors.imu.gyro_x = device_state.telemetry.sensors.imu.gyro_x;
    convertedState.sensors.imu.gyro_y = device_state.telemetry.sensors.imu.gyro_y;
    convertedState.sensors.imu.gyro_z = device_state.telemetry.sensors.imu.gyro_z;
    convertedState.sensors.imu.roll = device_state.telemetry.sensors.imu.roll;
    convertedState.sensors.imu.pitch = device_state.telemetry.sensors.imu.pitch;
    convertedState.sensors.imu.yaw = device_state.telemetry.sensors.imu.yaw;
    convertedState.sensors.imu.temperature = 25.0f; // 默认温度
    convertedState.sensors.imu.valid = device_state.telemetry.sensors.imu.valid;
    
    // 设置罗盘传感器数据
    convertedState.sensors.compass.heading = device_state.telemetry.sensors.compass.heading;
    convertedState.sensors.compass.mag_x = device_state.telemetry.sensors.compass.mag_x;
    convertedState.sensors.compass.mag_y = device_state.telemetry.sensors.compass.mag_y;
    convertedState.sensors.compass.mag_z = device_state.telemetry.sensors.compass.mag_z;
    convertedState.sensors.compass.declination = -5.2f; // 默认磁偏角
    convertedState.sensors.compass.inclination = 65.8f; // 默认磁倾角
    convertedState.sensors.compass.field_strength = 48.5f; // 默认磁场强度
    convertedState.sensors.compass.valid = device_state.telemetry.sensors.compass.valid;
    
    // 设置模块状态
    convertedState.modules.wifi_ready = device_state.telemetry.modules.wifi_ready;
    convertedState.modules.ble_ready = device_state.telemetry.modules.ble_ready;
    convertedState.modules.gsm_ready = device_state.telemetry.modules.gsm_ready;
    convertedState.modules.imu_ready = device_state.telemetry.modules.imu_ready;
    convertedState.modules.compass_ready = device_state.telemetry.modules.compass_ready;
    
    // 设置系统状态
    convertedState.system.battery_voltage = device_state.telemetry.system.battery_voltage;
    convertedState.system.battery_percentage = device_state.telemetry.system.battery_percentage;
    convertedState.system.is_charging = device_state.telemetry.system.is_charging;
    convertedState.system.external_power = device_state.telemetry.system.external_power;
    convertedState.system.signal_strength = device_state.telemetry.system.signal_strength;
    convertedState.system.uptime = device_state.telemetry.system.uptime;
    convertedState.system.free_heap = device_state.telemetry.system.free_heap;
    convertedState.system.cpu_usage = 45; // 默认CPU使用率
    convertedState.system.temperature = 42.5f; // 默认系统温度
    
    // 设置网络信息
    convertedState.network.wifi_connected = false; // 需要根据实际情况设置
    convertedState.network.gsm_connected = device_state.telemetry.modules.gsm_ready;
    
    deviceState = &convertedState;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 从全局状态转换完成");
    Serial.printf("[BLE数据提供者] 位置有效: %s, IMU有效: %s, 电池: %dmV\n",
                 convertedState.location.valid ? "是" : "否",
                 convertedState.sensors.imu.valid ? "是" : "否",
                 convertedState.system.battery_voltage);
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

String BLEDataProvider::generateFusionDebugData() {
    String debugData = "";
    
#ifdef ENABLE_IMU_FUSION
    extern FusionLocationManager fusionLocationManager;
    
    if (fusionLocationManager.isInitialized()) {
        // 获取融合位置
        Position pos = fusionLocationManager.getFusedPosition();
        
        // 构建JSON格式的调试数据
        debugData = "{";
        debugData += "\"timestamp\":" + String(millis()) + ",";
        debugData += "\"fusion_status\":\"active\",";
        
        if (pos.valid) {
            debugData += "\"position\":{";
            debugData += "\"lat\":" + String(pos.lat, 6) + ",";
            debugData += "\"lng\":" + String(pos.lng, 6) + ",";
            debugData += "\"alt\":" + String(pos.altitude, 1) + ",";
            debugData += "\"speed\":" + String(pos.speed * 3.6f, 2) + ",";
            debugData += "\"heading\":" + String(pos.heading, 1) + ",";
            debugData += "\"accuracy\":" + String(pos.accuracy, 2);
            debugData += "},";
        } else {
            debugData += "\"position\":{\"valid\":false},";
        }
        
        // 添加姿态信息
        debugData += "\"attitude\":{";
        debugData += "\"roll\":" + String(pos.roll, 1) + ",";
        debugData += "\"pitch\":" + String(pos.pitch, 1) + ",";
        debugData += "\"yaw\":" + String(pos.heading, 1);
        debugData += "},";
        
        // 添加融合状态信息
        debugData += "\"fusion_info\":{";
        debugData += "\"position_valid\":" + String(pos.valid ? "true" : "false") + ",";
        debugData += "\"accuracy\":" + String(pos.accuracy, 2);
        debugData += "},";
        
        // 添加统计信息
        debugData += "\"stats\":{";
        debugData += "\"uptime\":" + String(millis() / 1000) + ",";
        debugData += "\"free_heap\":" + String(ESP.getFreeHeap());
        debugData += "}";
        
        debugData += "}";
    } else {
        debugData = "{\"fusion_status\":\"inactive\",\"error\":\"Fusion system not initialized\"}";
    }
#else
    debugData = "{\"fusion_status\":\"disabled\",\"error\":\"IMU fusion not enabled\"}";
#endif

    return debugData;
}

#endif // ENABLE_BLE
