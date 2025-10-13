#include "BLEDataProvider.h"

#ifdef ENABLE_BLE

#include "utils/DebugUtils.h"
#include "Air780EG.h"
#include "imu/qmi8658.h"
#include "bat/BAT.h"

#ifdef ENABLE_IMU_FUSION
#include "location/FusionLocationManager.h"
#endif

const char* BLEDataProvider::TAG = "BLEDataProvider";

// 全局BLE数据提供者实例
BLEDataProvider bleDataProvider;

BLEDataProvider::BLEDataProvider()
    : air780eg(nullptr)
    , imu(nullptr)
    , battery(nullptr)
    , fusionManager(nullptr)
    , lastGPSUpdate(0)
    , lastBatteryUpdate(0)
    , lastIMUUpdate(0)
    , lastFusionUpdate(0)
    , lastSystemUpdate(0)
    , gpsDataValid(false)
    , batteryDataValid(false)
    , imuDataValid(false)
    , fusionDataValid(false)
    , systemDataValid(false)
{
    // 初始化数据结构
    memset(&cachedGPSData, 0, sizeof(cachedGPSData));
    memset(&cachedBatteryData, 0, sizeof(cachedBatteryData));
    memset(&cachedIMUData, 0, sizeof(cachedIMUData));
    memset(&cachedFusionData, 0, sizeof(cachedFusionData));
    memset(&cachedSystemStatus, 0, sizeof(cachedSystemStatus));
}

BLEDataProvider::~BLEDataProvider() {
    // 清理资源
}

void BLEDataProvider::begin() {
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 开始初始化...");
    #endif
    
    // 初始化数据缓存
    memset(&cachedGPSData, 0, sizeof(cachedGPSData));
    memset(&cachedBatteryData, 0, sizeof(cachedBatteryData));
    memset(&cachedIMUData, 0, sizeof(cachedIMUData));
    
    gpsDataValid = false;
    batteryDataValid = false;
    imuDataValid = false;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] ✅ 初始化完成");
    #endif
}

void BLEDataProvider::setDataSources(Air780EG* gps, IMU* imu, BAT* bat) {
    air780eg = gps;
    this->imu = imu;
    battery = bat;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 数据源已设置");
    #endif
}

void BLEDataProvider::setFusionManager(FusionLocationManager* fusion) {
    fusionManager = fusion;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE数据提供者] 融合定位管理器已设置");
    #endif
}

void BLEDataProvider::update() {
    unsigned long currentTime = millis();
    
    // 更新GPS数据 (每1秒更新一次)
    if (currentTime - lastGPSUpdate >= 1000) {
        updateGPSData();
        lastGPSUpdate = currentTime;
    }
    
    // 更新电池数据 (每5秒更新一次)
    if (currentTime - lastBatteryUpdate >= 5000) {
        updateBatteryData();
        lastBatteryUpdate = currentTime;
    }
    
    // 更新IMU数据 (每100ms更新一次)
    if (currentTime - lastIMUUpdate >= 100) {
        updateIMUData();
        lastIMUUpdate = currentTime;
    }
    
    // 更新融合数据 (每50ms更新一次，20Hz)
    if (currentTime - lastFusionUpdate >= BLE_FUSION_UPDATE_INTERVAL) {
        updateFusionData();
        lastFusionUpdate = currentTime;
    }
    
    // 更新系统状态 (每2秒更新一次)
    if (currentTime - lastSystemUpdate >= BLE_SYSTEM_UPDATE_INTERVAL) {
        updateSystemStatus();
        lastSystemUpdate = currentTime;
    }
}

void BLEDataProvider::forceUpdate() {
    updateGPSData();
    updateBatteryData();
    updateIMUData();
    updateFusionData();
    updateSystemStatus();
}

void BLEDataProvider::updateGPSData() {
    if (!air780eg) {
        gpsDataValid = false;
        return;
    }
    
    convertGPSData();
}

void BLEDataProvider::updateBatteryData() {
    if (!battery) {
        batteryDataValid = false;
        return;
    }
    
    convertBatteryData();
}

void BLEDataProvider::updateIMUData() {
    if (!imu) {
        imuDataValid = false;
        return;
    }
    
    convertIMUData();
}

void BLEDataProvider::updateFusionData() {
#ifdef ENABLE_IMU_FUSION
    if (!fusionManager) {
        fusionDataValid = false;
        return;
    }
    
    convertFusionData();
#else
    fusionDataValid = false;
#endif
}

void BLEDataProvider::updateSystemStatus() {
    convertSystemStatus();
}

void BLEDataProvider::convertGPSData() {
    // 从device_state获取GPS数据（已经由系统更新）
    cachedGPSData.latitude = device_state.latitude;
    cachedGPSData.longitude = device_state.longitude;
    cachedGPSData.altitude = 0;  // device_state中没有海拔数据
    cachedGPSData.speed = 0;     // device_state中没有速度数据
    cachedGPSData.course = 0;    // device_state中没有航向数据
    cachedGPSData.satellites = device_state.satellites;
    cachedGPSData.valid = device_state.gnssReady;
    
    gpsDataValid = cachedGPSData.valid;
    
    #ifdef BLE_DEBUG_ENABLED
    if (gpsDataValid) {
        Serial.printf("[BLE数据提供者] GPS数据: %.6f, %.6f, %.1fm, %.1fkm/h\n", 
                     cachedGPSData.latitude, cachedGPSData.longitude, 
                     cachedGPSData.altitude, cachedGPSData.speed);
    }
    #endif
}

void BLEDataProvider::convertBatteryData() {
    if (!battery) {
        return;
    }
    
    // 从BAT模块获取电池数据
    cachedBatteryData.voltage = battery->getVoltage();
    cachedBatteryData.percentage = battery->getPercentage();
    cachedBatteryData.is_charging = battery->isCharging();
    
    // 获取外部电源状态（从device_state）
    cachedBatteryData.external_power = device_state.external_power;
    
    batteryDataValid = true;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE数据提供者] 电池数据: %dmV, %d%%, 充电:%s, 外部电源:%s\n",
                 cachedBatteryData.voltage, cachedBatteryData.percentage,
                 cachedBatteryData.is_charging ? "是" : "否",
                 cachedBatteryData.external_power ? "是" : "否");
    #endif
}

void BLEDataProvider::convertIMUData() {
    if (!imu) {
        return;
    }
    
    // 从IMU获取数据
    // 使用IMU类的公共方法获取数据
    cachedIMUData.accel_x = imu->getAccelX();
    cachedIMUData.accel_y = imu->getAccelY();
    cachedIMUData.accel_z = imu->getAccelZ();
    
    cachedIMUData.gyro_x = imu->getGyroX();
    cachedIMUData.gyro_y = imu->getGyroY();
    cachedIMUData.gyro_z = imu->getGyroZ();
    
    // 获取姿态角数据
    cachedIMUData.pitch = imu->getPitch();
    cachedIMUData.roll = imu->getRoll();
    cachedIMUData.yaw = imu->getYaw();
    
    // 数据有效性（假设IMU数据总是有效的）
    cachedIMUData.valid = true;
    imuDataValid = cachedIMUData.valid;
    
    #ifdef BLE_DEBUG_ENABLED
    if (imuDataValid) {
        Serial.printf("[BLE数据提供者] IMU数据: 俯仰=%.1f°, 横滚=%.1f°, 偏航=%.1f°\n",
                     cachedIMUData.pitch, cachedIMUData.roll, cachedIMUData.yaw);
    }
    #endif
}

void BLEDataProvider::convertFusionData() {
#ifdef ENABLE_IMU_FUSION
    if (!fusionManager) {
        return;
    }
    
    // 获取融合位置数据
    MotorcycleTrajectoryPoint pos = fusionManager->getCurrentPosition();
    MotorcycleMotionState motion = fusionManager->getCurrentMotionState();
    
    // 融合位置
    cachedFusionData.lat = pos.lat;
    cachedFusionData.lng = pos.lng;
    cachedFusionData.altitude = pos.altitude;
    
    // 运动状态
    cachedFusionData.speed = pos.speed;
    cachedFusionData.heading = pos.heading;
    
    // 原始Madgwick输出
    fusionManager->getRawMadgwickOutput(
        cachedFusionData.raw_roll,
        cachedFusionData.raw_pitch,
        cachedFusionData.raw_yaw
    );
    
    // Kalman滤波输出
    fusionManager->getKalmanOutput(
        cachedFusionData.kalman_roll,
        cachedFusionData.kalman_pitch,
        cachedFusionData.kalman_yaw
    );
    
    // 摩托车特定数据
    cachedFusionData.lean_angle = motion.leanAngle;
    cachedFusionData.lean_rate = motion.leanRate;
    cachedFusionData.forward_accel = motion.forwardAccel;
    cachedFusionData.lateral_accel = motion.lateralAccel;
    
    // 位置速度积分数据
    float pos_data[3], vel_data[3];
    fusionManager->getIntegrationData(pos_data, vel_data);
    cachedFusionData.pos_x = pos_data[0];
    cachedFusionData.pos_y = pos_data[1];
    cachedFusionData.pos_z = pos_data[2];
    cachedFusionData.vel_x = vel_data[0];
    cachedFusionData.vel_y = vel_data[1];
    cachedFusionData.vel_z = vel_data[2];
    
    // 状态标志
    cachedFusionData.source = (uint8_t)fusionManager->getCurrentSource();
    cachedFusionData.kalman_enabled = fusionManager->isKalmanEnabled();
    cachedFusionData.valid = fusionManager->isInitialized();
    cachedFusionData.timestamp = millis();
    
    fusionDataValid = cachedFusionData.valid;
    
    #ifdef BLE_DEBUG_ENABLED
    if (fusionDataValid) {
        Serial.printf("[BLE数据提供者] 融合数据: 位置(%.6f,%.6f) 速度%.1fm/s 来源:%d\n",
                     cachedFusionData.lat, cachedFusionData.lng, 
                     cachedFusionData.speed, cachedFusionData.source);
    }
    #endif
#endif
}

void BLEDataProvider::convertSystemStatus() {
    // 运行状态
    cachedSystemStatus.mode = 0; // 正常模式
    cachedSystemStatus.uptime = millis() / 1000;
    cachedSystemStatus.loop_count = 0; // TODO: 从main loop获取
    
    // 模块状态
    cachedSystemStatus.gps_status = device_state.gnssReady ? 1 : 0;
    cachedSystemStatus.imu_status = (imu != nullptr) ? 1 : 0;
    cachedSystemStatus.battery_status = (battery != nullptr) ? 
        (battery->isCharging() ? 2 : (battery->getPercentage() > 20 ? 1 : 0)) : 0;
    
#ifdef ENABLE_IMU_FUSION
    if (fusionManager) {
        cachedSystemStatus.fusion_status = fusionManager->isInitialized() ? 
            (fusionManager->getCurrentSource() == SOURCE_IMU_DEAD_RECKONING ? 2 : 1) : 0;
        
        // 统计数据
        cachedSystemStatus.total_distance = fusionManager->getTotalDistance();
        cachedSystemStatus.max_speed = fusionManager->getMaxSpeed();
        cachedSystemStatus.max_lean_angle = fusionManager->getMaxLeanAngle();
        cachedSystemStatus.gps_updates = fusionManager->getGPSUpdateCount();
        cachedSystemStatus.imu_updates = fusionManager->getIMUUpdateCount();
        cachedSystemStatus.fusion_updates = fusionManager->getFusionUpdateCount();
    } else {
        cachedSystemStatus.fusion_status = 0;
        cachedSystemStatus.total_distance = 0;
        cachedSystemStatus.max_speed = 0;
        cachedSystemStatus.max_lean_angle = 0;
        cachedSystemStatus.gps_updates = 0;
        cachedSystemStatus.imu_updates = 0;
        cachedSystemStatus.fusion_updates = 0;
    }
#else
    cachedSystemStatus.fusion_status = 0;
    cachedSystemStatus.total_distance = 0;
    cachedSystemStatus.max_speed = 0;
    cachedSystemStatus.max_lean_angle = 0;
    cachedSystemStatus.gps_updates = 0;
    cachedSystemStatus.imu_updates = 0;
    cachedSystemStatus.fusion_updates = 0;
#endif
    
    // 资源使用
    cachedSystemStatus.free_heap = ESP.getFreeHeap();
    cachedSystemStatus.free_heap_kb = cachedSystemStatus.free_heap / 1024;
    
    // 错误信息
    cachedSystemStatus.error_code = 0;
    strncpy(cachedSystemStatus.error_msg, "", sizeof(cachedSystemStatus.error_msg));
    
    cachedSystemStatus.timestamp = millis();
    
    systemDataValid = true;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE数据提供者] 系统状态: GPS:%d IMU:%d BAT:%d FUS:%d\n",
                 cachedSystemStatus.gps_status, cachedSystemStatus.imu_status, 
                 cachedSystemStatus.battery_status, cachedSystemStatus.fusion_status);
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
    Serial.printf("GPS数据源: %s\n", air780eg ? "已设置" : "未设置");
    Serial.printf("IMU数据源: %s\n", imu ? "已设置" : "未设置");
    Serial.printf("电池数据源: %s\n", battery ? "已设置" : "未设置");
    Serial.printf("GPS数据有效: %s\n", gpsDataValid ? "是" : "否");
    Serial.printf("电池数据有效: %s\n", batteryDataValid ? "是" : "否");
    Serial.printf("IMU数据有效: %s\n", imuDataValid ? "是" : "否");
    
    if (gpsDataValid) {
        Serial.printf("GPS: %.6f, %.6f, %.1fm\n", 
                     cachedGPSData.latitude, cachedGPSData.longitude, cachedGPSData.altitude);
    }
    
    if (batteryDataValid) {
        Serial.printf("电池: %dmV, %d%%\n", 
                     cachedBatteryData.voltage, cachedBatteryData.percentage);
    }
    
    if (imuDataValid) {
        Serial.printf("IMU: 俯仰=%.1f°, 横滚=%.1f°, 偏航=%.1f°\n",
                     cachedIMUData.pitch, cachedIMUData.roll, cachedIMUData.yaw);
    }
    
    Serial.println("========================");
}

#endif // ENABLE_BLE
