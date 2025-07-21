#include "FusionLocationManager.h"

const char* FusionLocationManager::TAG = "FusionLocation";

// ============================================================================
// MotoBoxIMUProvider 实现
// ============================================================================

MotoBoxIMUProvider::MotoBoxIMUProvider() : debug_enabled(false), last_update_time(0) {}

bool MotoBoxIMUProvider::getData(IMUData& data) {
#ifdef ENABLE_IMU
    // IMU总是可用的，不需要检查isInitialized
    
    // 获取IMU数据并转换格式
    data.accel[0] = imu.getAccelX() * 9.8f;  // 转换为 m/s²
    data.accel[1] = imu.getAccelY() * 9.8f;
    data.accel[2] = imu.getAccelZ() * 9.8f;
    
    data.gyro[0] = imu.getGyroX() * DEG_TO_RAD;  // 转换为 rad/s
    data.gyro[1] = imu.getGyroY() * DEG_TO_RAD;
    data.gyro[2] = imu.getGyroZ() * DEG_TO_RAD;
    
    data.timestamp = millis();
    data.valid = true;
    last_update_time = data.timestamp;
    
    if (debug_enabled) {
        Serial.printf("[IMU] IMU数据: 加速度(%.2f,%.2f,%.2f) 陀螺仪(%.3f,%.3f,%.3f)\n", 
                     data.accel[0], data.accel[1], data.accel[2],
                     data.gyro[0], data.gyro[1], data.gyro[2]);
    }
    
    return true;
#else
    return false;
#endif
}

bool MotoBoxIMUProvider::isAvailable() {
#ifdef ENABLE_IMU
    return true;  // IMU总是可用的
#else
    return false;
#endif
}

// ============================================================================
// MotoBoxGPSProvider 实现
// ============================================================================

MotoBoxGPSProvider::MotoBoxGPSProvider() : debug_enabled(false), last_update_time(0) {}

bool MotoBoxGPSProvider::getData(GPSData& data) {
    // 获取Air780EG的GNSS数据
    if (!air780eg.getGNSS().isDataValid()) return false;
    
    gnss_data_t& gnss = air780eg.getGNSS().gnss_data;
    
    if (!gnss.is_fixed || !gnss.data_valid) return false;
    
    data.lat = gnss.latitude;
    data.lng = gnss.longitude;
    data.altitude = gnss.altitude;
    data.accuracy = gnss.hdop * 5.0f;  // 简单的精度估算
    data.timestamp = millis();
    data.valid = true;
    last_update_time = data.timestamp;
    
    if (debug_enabled) {
        Serial.printf("[GPS] GPS数据: 位置(%.6f,%.6f) 高度%.1fm 精度%.1fm 卫星%d\n", 
                     data.lat, data.lng, data.altitude, data.accuracy, gnss.satellites);
    }
    
    return true;
}

bool MotoBoxGPSProvider::isAvailable() {
    return air780eg.getGNSS().isEnabled() && air780eg.getGNSS().isDataValid();
}

// ============================================================================
// MotoBoxMagProvider 实现
// ============================================================================

MotoBoxMagProvider::MotoBoxMagProvider() : debug_enabled(false), last_update_time(0) {}

bool MotoBoxMagProvider::getData(MagData& data) {
#ifdef ENABLE_COMPASS
    if (!compass.isInitialized() || !compass.isDataValid()) return false;
    
    // 获取原始磁场数据
    int16_t x, y, z;
    compass.getRawData(x, y, z);
    
    // 转换为微特斯拉 (μT)
    data.mag[0] = x * 0.1f;  // QMC5883L的分辨率约为0.1μT/LSB
    data.mag[1] = y * 0.1f;
    data.mag[2] = z * 0.1f;
    
    data.timestamp = millis();
    data.valid = true;
    last_update_time = data.timestamp;
    
    if (debug_enabled) {
        Serial.printf("[MAG] 磁场数据: (%.1f,%.1f,%.1f)μT 航向%.1f°\n", 
                     data.mag[0], data.mag[1], data.mag[2], compass.getHeading());
    }
    
    return true;
#else
    return false;
#endif
}

bool MotoBoxMagProvider::isAvailable() {
#ifdef ENABLE_COMPASS
    return compass.isInitialized();
#else
    return false;
#endif
}

// ============================================================================
// FusionLocationManager 实现
// ============================================================================

FusionLocationManager::FusionLocationManager() 
    : imuProvider(nullptr), gpsProvider(nullptr), magProvider(nullptr),
      fusionLocation(nullptr), initialized(false), debug_enabled(false),
      update_interval(100), last_update_time(0), last_debug_print_time(0),
      initial_latitude(39.9042), initial_longitude(116.4074) {
    
    memset(&stats, 0, sizeof(stats));
}

FusionLocationManager::~FusionLocationManager() {
    if (fusionLocation) delete fusionLocation;
    if (imuProvider) delete imuProvider;
    if (gpsProvider) delete gpsProvider;
    if (magProvider) delete magProvider;
}

bool FusionLocationManager::begin(double initLat, double initLng) {
    if (initialized) return true;
    
    Serial.printf("[%s] 初始化融合定位系统...\n", TAG);
    
    initial_latitude = initLat;
    initial_longitude = initLng;
    
    // 创建传感器提供者
    imuProvider = new MotoBoxIMUProvider();
    gpsProvider = new MotoBoxGPSProvider();
    magProvider = new MotoBoxMagProvider();
    
    if (!imuProvider) {
        Serial.printf("[%s] ❌ IMU提供者创建失败\n", TAG);
        return false;
    }
    
    // 创建融合定位对象（IMU是必需的）
    fusionLocation = new FusionLocation(imuProvider, initLat, initLng);
    if (!fusionLocation) {
        Serial.printf("[%s] ❌ 融合定位对象创建失败\n", TAG);
        return false;
    }
    
    // 设置可选传感器
    if (gpsProvider) {
        fusionLocation->setGPSProvider(gpsProvider);
        Serial.printf("[%s] ✅ GPS提供者已设置\n", TAG);
    }
    
    if (magProvider) {
        fusionLocation->setMagProvider(magProvider);
        Serial.printf("[%s] ✅ 地磁计提供者已设置\n", TAG);
    }
    
    // 初始化融合定位
    fusionLocation->begin();
    
    initialized = true;
    last_update_time = millis();
    
    Serial.printf("[%s] ✅ 融合定位系统初始化成功\n", TAG);
    Serial.printf("[%s] 初始位置: %.6f, %.6f\n", TAG, initLat, initLng);
    
    return true;
}

void FusionLocationManager::loop() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    if (currentTime - last_update_time < update_interval) return;
    
    // 更新融合定位
    fusionLocation->update();
    
    // 获取融合位置并更新统计
    Position pos = fusionLocation->getPosition();
    updateStats(pos);
    
    last_update_time = currentTime;
    
    // 定期打印调试信息
    if (debug_enabled && currentTime - last_debug_print_time > 5000) {
        printStatus();
        last_debug_print_time = currentTime;
    }
}

Position FusionLocationManager::getFusedPosition() {
    if (!initialized) {
        Position invalid_pos;
        invalid_pos.valid = false;
        return invalid_pos;
    }
    
    return fusionLocation->getPosition();
}

void FusionLocationManager::debugPrint(const String& message) {
    if (debug_enabled) {
        Serial.printf("[%s] %s\n", TAG, message.c_str());
    }
}

void FusionLocationManager::updateStats(const Position& pos) {
    stats.total_updates++;
    
    if (pos.valid) {
        stats.fusion_updates++;
        
        if (pos.sources.hasGPS) stats.gps_updates++;
        if (pos.sources.hasIMU) stats.imu_updates++;
        if (pos.sources.hasMag) stats.mag_updates++;
    }
}

float FusionLocationManager::getPositionAccuracy() {
    if (!initialized) return -1.0f;
    return fusionLocation->getPositionAccuracy();
}

float FusionLocationManager::getHeading() {
    if (!initialized) return -1.0f;
    return fusionLocation->getHeading();
}

float FusionLocationManager::getSpeed() {
    if (!initialized) return -1.0f;
    return fusionLocation->getSpeed();
}

bool FusionLocationManager::isPositionValid() {
    if (!initialized) return false;
    Position pos = fusionLocation->getPosition();
    return pos.valid;
}

void FusionLocationManager::setInitialPosition(double lat, double lng) {
    initial_latitude = lat;
    initial_longitude = lng;
    
    if (initialized && fusionLocation) {
        fusionLocation->setInitialPosition(lat, lng);
        debugPrint("初始位置已更新: " + String(lat, 6) + ", " + String(lng, 6));
    }
}

void FusionLocationManager::setUpdateInterval(unsigned long interval_ms) {
    update_interval = interval_ms;
    debugPrint("更新间隔设置为: " + String(interval_ms) + "ms");
}

void FusionLocationManager::setDebug(bool enable) {
    debug_enabled = enable;
    
    if (imuProvider) imuProvider->setDebug(enable);
    if (gpsProvider) gpsProvider->setDebug(enable);
    if (magProvider) magProvider->setDebug(enable);
    
    Serial.printf("[%s] 调试模式: %s\n", TAG, enable ? "开启" : "关闭");
}

void FusionLocationManager::printStatus() {
    if (!initialized) {
        Serial.printf("[%s] 系统未初始化\n", TAG);
        return;
    }
    
    Position pos = fusionLocation->getPosition();
    DataSourceStatus status = getDataSourceStatus();
    
    Serial.println("=== 融合定位系统状态 ===");
    Serial.printf("位置: %.6f, %.6f (精度: %.1fm)\n", pos.lat, pos.lng, pos.accuracy);
    Serial.printf("航向: %.1f° | 速度: %.1fm/s | 高度: %.1fm\n", pos.heading, pos.speed, pos.altitude);
    Serial.printf("数据源: %s%s%s\n", 
                 status.imu_available ? "IMU " : "",
                 status.gps_available ? "GPS " : "",
                 status.mag_available ? "MAG " : "");
    Serial.printf("有效性: %s | 初始化: %s\n", 
                 pos.valid ? "有效" : "无效",
                 fusionLocation->isInitialized() ? "是" : "否");
}

void FusionLocationManager::printStats() {
    Serial.println("=== 融合定位统计信息 ===");
    Serial.printf("总更新次数: %lu\n", stats.total_updates);
    Serial.printf("融合更新: %lu | GPS: %lu | IMU: %lu | MAG: %lu\n", 
                 stats.fusion_updates, stats.gps_updates, stats.imu_updates, stats.mag_updates);
    
    if (stats.total_updates > 0) {
        Serial.printf("成功率: %.1f%%\n", (float)stats.fusion_updates / stats.total_updates * 100.0f);
    }
}

String FusionLocationManager::getPositionJSON() {
    Position pos = getFusedPosition();
    DataSourceStatus status = getDataSourceStatus();
    
    String json = "{";
    json += "\"valid\":" + String(pos.valid ? "true" : "false") + ",";
    json += "\"lat\":" + String(pos.lat, 6) + ",";
    json += "\"lng\":" + String(pos.lng, 6) + ",";
    json += "\"altitude\":" + String(pos.altitude, 2) + ",";
    json += "\"accuracy\":" + String(pos.accuracy, 1) + ",";
    json += "\"heading\":" + String(pos.heading, 1) + ",";
    json += "\"speed\":" + String(pos.speed, 2) + ",";
    json += "\"timestamp\":" + String(pos.timestamp) + ",";
    json += "\"sources\":{";
    json += "\"gps\":" + String(pos.sources.hasGPS ? "true" : "false") + ",";
    json += "\"imu\":" + String(pos.sources.hasIMU ? "true" : "false") + ",";
    json += "\"mag\":" + String(pos.sources.hasMag ? "true" : "false");
    json += "},";
    json += "\"data_sources\":{";
    json += "\"imu_available\":" + String(status.imu_available ? "true" : "false") + ",";
    json += "\"gps_available\":" + String(status.gps_available ? "true" : "false") + ",";
    json += "\"mag_available\":" + String(status.mag_available ? "true" : "false");
    json += "}";
    json += "}";
    
    return json;
}

void FusionLocationManager::resetStats() {
    memset(&stats, 0, sizeof(stats));
    debugPrint("统计信息已重置");
}

FusionLocationManager::DataSourceStatus FusionLocationManager::getDataSourceStatus() {
    DataSourceStatus status;
    memset(&status, 0, sizeof(status));
    
    if (imuProvider) {
        status.imu_available = imuProvider->isAvailable();
        status.last_imu_time = millis(); // 简化实现
    }
    
    if (gpsProvider) {
        status.gps_available = gpsProvider->isAvailable();
        status.last_gps_time = millis();
    }
    
    if (magProvider) {
        status.mag_available = magProvider->isAvailable();
        status.last_mag_time = millis();
    }
    
    if (initialized && fusionLocation) {
        Position pos = fusionLocation->getPosition();
        status.fusion_valid = pos.valid;
        status.last_fusion_time = pos.timestamp;
    }
    
    return status;
}

#ifdef ENABLE_FUSION_LOCATION
FusionLocationManager fusionLocationManager;
#endif
