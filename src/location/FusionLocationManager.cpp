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
        Serial.printf("[GPS] GPS数据: 位置(%.6f,%.6f) 高度%.1fm 精度%.1fm 卫星%d 来源:%s\n", 
                     data.lat, data.lng, data.altitude, data.accuracy, gnss.satellites, gnss.location_type.c_str());
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
      simpleFusion(nullptr), ekfTracker(nullptr), currentAlgorithm(FUSION_EKF_VEHICLE),
      initialized(false), debug_enabled(false),
      update_interval(100), last_update_time(0), last_debug_print_time(0),
      initial_latitude(39.9042), initial_longitude(116.4074) {
    
    memset(&stats, 0, sizeof(stats));
    
    // 初始化兜底定位配置
    fallbackConfig.enabled = false;
    fallbackConfig.gnss_timeout = 30000;      // 30秒
    fallbackConfig.lbs_interval = 300000;     // 5分钟
    fallbackConfig.wifi_interval = 180000;    // 3分钟
    fallbackConfig.prefer_wifi_over_lbs = true;
    fallbackConfig.last_lbs_time = 0;
    fallbackConfig.last_wifi_time = 0;
    fallbackConfig.lbs_in_progress = false;
    fallbackConfig.wifi_in_progress = false;
    
    // 设置默认EKF配置（适合摩托车）
    ekfConfig.processNoisePos = 0.5f;        // 摩托车位置变化较快
    ekfConfig.processNoiseVel = 2.0f;        // 速度变化较大
    ekfConfig.processNoiseHeading = 0.05f;   // 航向变化较频繁
    ekfConfig.processNoiseHeadingRate = 0.2f;
    
    ekfConfig.gpsNoisePos = 25.0f;           // GPS精度约5m
    ekfConfig.imuNoiseAccel = 0.2f;          // 摩托车振动较大
    ekfConfig.imuNoiseGyro = 0.02f;
    
    // 设置摩托车模型参数
    vehicleModel.wheelbase = 1.4f;           // 摩托车轴距约1.4m
    vehicleModel.maxAcceleration = 4.0f;     // 摩托车加速性能较好
    vehicleModel.maxDeceleration = 10.0f;    // 制动性能
    vehicleModel.maxSteeringAngle = 0.8f;    // 摩托车转向角度较大
}

FusionLocationManager::~FusionLocationManager() {
    if (simpleFusion) delete simpleFusion;
    if (ekfTracker) delete ekfTracker;
    if (imuProvider) delete imuProvider;
    if (gpsProvider) delete gpsProvider;
    if (magProvider) delete magProvider;
}

bool FusionLocationManager::begin(FusionAlgorithm algorithm, double initLat, double initLng) {
    if (initialized) return true;
    
    Serial.printf("[%s] 初始化融合定位系统 (算法: %s)...\n", TAG, 
                 algorithm == FUSION_EKF_VEHICLE ? "EKF车辆模型" : "简单卡尔曼");
    
    initial_latitude = initLat;
    initial_longitude = initLng;
    currentAlgorithm = algorithm;
    
    // 创建传感器提供者
    imuProvider = new MotoBoxIMUProvider();
    gpsProvider = new MotoBoxGPSProvider();
    magProvider = new MotoBoxMagProvider();
    
    if (!imuProvider) {
        Serial.printf("[%s] ❌ IMU提供者创建失败\n", TAG);
        return false;
    }
    
    // 根据算法类型创建融合对象
    if (algorithm == FUSION_EKF_VEHICLE) {
        ekfTracker = new EKFVehicleTracker(imuProvider, initLat, initLng);
        if (!ekfTracker) {
            Serial.printf("[%s] ❌ EKF追踪器创建失败\n", TAG);
            return false;
        }
        
        // 设置传感器和配置
        ekfTracker->setGPSProvider(gpsProvider);
        ekfTracker->setMagProvider(magProvider);
        ekfTracker->setEKFConfig(ekfConfig);
        ekfTracker->setVehicleModel(vehicleModel);
        ekfTracker->begin();
        
        Serial.printf("[%s] ✅ EKF车辆追踪器初始化成功\n", TAG);
    } else {
        simpleFusion = new FusionLocation(imuProvider, initLat, initLng);
        if (!simpleFusion) {
            Serial.printf("[%s] ❌ 简单融合对象创建失败\n", TAG);
            return false;
        }
        
        simpleFusion->setGPSProvider(gpsProvider);
        simpleFusion->setMagProvider(magProvider);
        simpleFusion->begin();
        
        Serial.printf("[%s] ✅ 简单卡尔曼滤波器初始化成功\n", TAG);
    }
    
    initialized = true;
    last_update_time = millis();
    
    Serial.printf("[%s] ✅ 融合定位系统初始化成功\n", TAG);
    Serial.printf("[%s] 初始位置: %.6f, %.6f\n", TAG, initLat, initLng);
    
    return true;
}

void FusionLocationManager::loop() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    
    // 高频更新：EKF算法支持高频IMU数据处理
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        // EKF算法每次都更新，以充分利用高频IMU数据
        ekfTracker->update();
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        // 简单卡尔曼滤波保持原有的更新间隔
        if (currentTime - last_update_time >= update_interval) {
            simpleFusion->update();
            last_update_time = currentTime;
        }
    }
    
    // 状态统计和调试信息保持低频更新
    if (currentTime - last_update_time >= update_interval) {
        // 获取融合位置并更新统计
        Position pos = getFusedPosition();
        updateStats(pos);
        
        // 处理兜底定位逻辑
        if (fallbackConfig.enabled) {
            handleFallbackLocation();
        }
        
        last_update_time = currentTime;
    }
    
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
    
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        return ekfTracker->getPosition();
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        return simpleFusion->getPosition();
    }
    
    Position invalid_pos;
    invalid_pos.valid = false;
    return invalid_pos;
}

bool FusionLocationManager::switchAlgorithm(FusionAlgorithm algorithm) {
    if (!initialized || algorithm == currentAlgorithm) return true;
    
    Serial.printf("[%s] 切换融合算法: %s -> %s\n", TAG,
                 currentAlgorithm == FUSION_EKF_VEHICLE ? "EKF" : "简单卡尔曼",
                 algorithm == FUSION_EKF_VEHICLE ? "EKF" : "简单卡尔曼");
    
    // 获取当前位置作为新算法的初始位置
    Position currentPos = getFusedPosition();
    double lat = currentPos.valid ? currentPos.lat : initial_latitude;
    double lng = currentPos.valid ? currentPos.lng : initial_longitude;
    
    if (algorithm == FUSION_EKF_VEHICLE) {
        // 切换到EKF
        if (!ekfTracker) {
            ekfTracker = new EKFVehicleTracker(imuProvider, lat, lng);
            if (!ekfTracker) return false;
            
            ekfTracker->setGPSProvider(gpsProvider);
            ekfTracker->setMagProvider(magProvider);
            ekfTracker->setEKFConfig(ekfConfig);
            ekfTracker->setVehicleModel(vehicleModel);
            ekfTracker->begin();
        }
    } else {
        // 切换到简单卡尔曼
        if (!simpleFusion) {
            simpleFusion = new FusionLocation(imuProvider, lat, lng);
            if (!simpleFusion) return false;
            
            simpleFusion->setGPSProvider(gpsProvider);
            simpleFusion->setMagProvider(magProvider);
            simpleFusion->begin();
        }
    }
    
    currentAlgorithm = algorithm;
    Serial.printf("[%s] ✅ 算法切换成功\n", TAG);
    return true;
}

void FusionLocationManager::setEKFConfig(const EKFConfig& config) {
    ekfConfig = config;
    if (ekfTracker) {
        ekfTracker->setEKFConfig(config);
        debugPrint("EKF配置已更新");
    }
}

void FusionLocationManager::setVehicleModel(const VehicleModel& model) {
    vehicleModel = model;
    if (ekfTracker) {
        ekfTracker->setVehicleModel(model);
        debugPrint("车辆模型已更新");
    }
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
    
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        return ekfTracker->getPositionAccuracy();
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        return simpleFusion->getPositionAccuracy();
    }
    return -1.0f;
}

float FusionLocationManager::getHeading() {
    if (!initialized) return -1.0f;
    
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        return ekfTracker->getHeading();
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        return simpleFusion->getHeading();
    }
    return -1.0f;
}

float FusionLocationManager::getSpeed() {
    if (!initialized) return -1.0f;
    
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        return ekfTracker->getVelocity();  // EKF提供速度估计
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        return simpleFusion->getSpeed();
    }
    return -1.0f;
}

bool FusionLocationManager::isPositionValid() {
    if (!initialized) return false;
    Position pos = getFusedPosition();
    return pos.valid;
}

void FusionLocationManager::setInitialPosition(double lat, double lng) {
    initial_latitude = lat;
    initial_longitude = lng;
    
    if (initialized) {
        if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
            ekfTracker->setInitialPosition(lat, lng);
        } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
            simpleFusion->setInitialPosition(lat, lng);
        }
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
    
    Position pos = getFusedPosition();
    DataSourceStatus status = getDataSourceStatus();
    String locationSource = getLocationSource();
    
    Serial.println("=== 融合定位系统状态 ===");
    Serial.printf("算法: %s\n", currentAlgorithm == FUSION_EKF_VEHICLE ? "EKF车辆模型" : "简单卡尔曼");
    Serial.printf("位置: %.6f, %.6f (精度: %.1fm) [来源: %s]\n", 
                 pos.lat, pos.lng, pos.accuracy, locationSource.c_str());
    Serial.printf("航向: %.1f° | 速度: %.1fm/s | 高度: %.1fm\n", pos.heading, pos.speed, pos.altitude);
    Serial.printf("数据源: %s%s%s\n", 
                 status.imu_available ? "IMU " : "",
                 status.gps_available ? "GPS " : "",
                 status.mag_available ? "MAG " : "");
    
    bool isInit = false;
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        isInit = ekfTracker->isInitialized();
    } else if (currentAlgorithm == FUSION_SIMPLE_KALMAN && simpleFusion) {
        isInit = simpleFusion->isInitialized();
    }
    
    Serial.printf("有效性: %s | 初始化: %s\n", 
                 pos.valid ? "有效" : "无效", isInit ? "是" : "否");
    
    // 兜底定位状态
    if (fallbackConfig.enabled) {
        Serial.printf("兜底定位: %s | GNSS信号: %s\n",
                     "启用", isGNSSSignalLost() ? "丢失" : "正常");
        if (fallbackConfig.lbs_in_progress) Serial.println("LBS定位进行中...");
        if (fallbackConfig.wifi_in_progress) Serial.println("WiFi定位进行中...");
    }
    
    // EKF特有信息
    if (currentAlgorithm == FUSION_EKF_VEHICLE && ekfTracker) {
        Serial.printf("航向角速度: %.2f°/s\n", ekfTracker->getHeadingRate() * 57.2958f);
    }
}

void FusionLocationManager::printStats() {
    Serial.println("=== 融合定位统计信息 ===");
    Serial.printf("总更新次数: %lu\n", stats.total_updates);
    Serial.printf("融合更新: %lu | GPS: %lu | IMU: %lu | MAG: %lu\n", 
                 stats.fusion_updates, stats.gps_updates, stats.imu_updates, stats.mag_updates);
    Serial.printf("兜底定位: LBS: %lu | WiFi: %lu\n", stats.lbs_updates, stats.wifi_updates);
    
    if (stats.total_updates > 0) {
        Serial.printf("成功率: %.1f%%\n", (float)stats.fusion_updates / stats.total_updates * 100.0f);
    }
    
    if (fallbackConfig.enabled) {
        Serial.println("=== 兜底定位配置 ===");
        Serial.printf("GNSS超时: %lus | LBS间隔: %lus | WiFi间隔: %lus\n",
                     fallbackConfig.gnss_timeout/1000,
                     fallbackConfig.lbs_interval/1000,
                     fallbackConfig.wifi_interval/1000);
        Serial.printf("优先WiFi: %s | 当前定位源: %s\n",
                     fallbackConfig.prefer_wifi_over_lbs ? "是" : "否",
                     getLocationSource().c_str());
    }
}

/*
{
  "latitude": 0,
  "longitude": 0,
  "altitude": 0,
  "speed": 0,
  "course": 0,
  "hdop": 1,
  "date": "",
  "timestamp": "",
  "location_type": "GNSS",
  "satellites": 99,
  "is_fixed": true,
  "data_valid": false
}*/
String FusionLocationManager::getPositionJSON() {
    Position pos = getFusedPosition();
    DataSourceStatus status = getDataSourceStatus();

    String json = "{";
    json += "\"latitude\":" + String(pos.lat, 6) + ",";
    json += "\"longitude\":" + String(pos.lng, 6) + ",";
    json += "\"altitude\":" + String(pos.altitude, 2) + ",";
    json += "\"speed\":" + String(pos.speed, 2) + ",";
    json += "\"course\":" + String(pos.heading, 1) + ",";
    json += "\"hdop\":" + String(pos.accuracy, 1) + ",";
    json += "\"timestamp\":" + String(pos.timestamp) + ",";
    json += "\"location_type\":\"FUSION_LOCATION\",";
    json += "\"satellites\":" + String(status.gps_available ? air780eg.getGNSS().gnss_data.satellites : 0) + ",";
    json += "\"is_fixed\":" + String(status.gps_available ? "true" : "false") + ",";
    json += "\"data_valid\":" + String(status.gps_available ? "true" : "false") + ",";
    json += "}";
    return json;
}
// String FusionLocationManager::getPositionJSON() {
        //     Position pos = getFusedPosition();
//     DataSourceStatus status = getDataSourceStatus();
    
//     String json = "{";
//     json += "\"valid\":" + String(pos.valid ? "true" : "false") + ",";
//     json += "\"lat\":" + String(pos.lat, 6) + ",";
//     json += "\"lng\":" + String(pos.lng, 6) + ",";
//     json += "\"altitude\":" + String(pos.altitude, 2) + ",";
//     json += "\"accuracy\":" + String(pos.accuracy, 1) + ",";
//     json += "\"heading\":" + String(pos.heading, 1) + ",";
//     json += "\"speed\":" + String(pos.speed, 2) + ",";
//     json += "\"timestamp\":" + String(pos.timestamp) + ",";
//     json += "\"sources\":{";
//     json += "\"gps\":" + String(pos.sources.hasGPS ? "true" : "false") + ",";
//     json += "\"imu\":" + String(pos.sources.hasIMU ? "true" : "false") + ",";
//     json += "\"mag\":" + String(pos.sources.hasMag ? "true" : "false");
//     json += "},";
//     json += "\"data_sources\":{";
//     json += "\"imu_available\":" + String(status.imu_available ? "true" : "false") + ",";
//     json += "\"gps_available\":" + String(status.gps_available ? "true" : "false") + ",";
//     json += "\"mag_available\":" + String(status.mag_available ? "true" : "false");
//     json += "}";
//     json += "}";
    
//     return json;
// }

void FusionLocationManager::resetStats() {
    memset(&stats, 0, sizeof(stats));
    debugPrint("统计信息已重置");
}

void FusionLocationManager::configureFallbackLocation(bool enable, 
                                                     unsigned long gnss_timeout,
                                                     unsigned long lbs_interval,
                                                     unsigned long wifi_interval,
                                                     bool prefer_wifi) {
    fallbackConfig.enabled = enable;
    fallbackConfig.gnss_timeout = gnss_timeout;
    fallbackConfig.lbs_interval = lbs_interval;
    fallbackConfig.wifi_interval = wifi_interval;
    fallbackConfig.prefer_wifi_over_lbs = prefer_wifi;
    
    debugPrint("兜底定位配置: " + String(enable ? "启用" : "禁用") + 
               ", GNSS超时:" + String(gnss_timeout/1000) + "s" +
               ", LBS间隔:" + String(lbs_interval/1000) + "s" +
               ", WiFi间隔:" + String(wifi_interval/1000) + "s" +
               ", 优先WiFi:" + String(prefer_wifi ? "是" : "否"));
}

void FusionLocationManager::handleFallbackLocation() {
    if (!fallbackConfig.enabled) return;
    
    unsigned long currentTime = millis();
    
    // 检查GNSS信号是否丢失
    if (isGNSSSignalLost()) {
        debugPrint("GNSS信号丢失，启动兜底定位策略");
        
        // 根据配置决定使用WiFi还是LBS
        if (fallbackConfig.prefer_wifi_over_lbs) {
            // 优先尝试WiFi定位
            if (currentTime - fallbackConfig.last_wifi_time >= fallbackConfig.wifi_interval) {
                if (tryWiFiLocation()) {
                    fallbackConfig.last_wifi_time = currentTime;
                    return; // WiFi定位成功，不再尝试LBS
                }
            }
            
            // WiFi定位失败或未到间隔时间，尝试LBS
            if (currentTime - fallbackConfig.last_lbs_time >= fallbackConfig.lbs_interval) {
                if (tryLBSLocation()) {
                    fallbackConfig.last_lbs_time = currentTime;
                }
            }
        } else {
            // 优先尝试LBS定位
            if (currentTime - fallbackConfig.last_lbs_time >= fallbackConfig.lbs_interval) {
                if (tryLBSLocation()) {
                    fallbackConfig.last_lbs_time = currentTime;
                    return; // LBS定位成功，不再尝试WiFi
                }
            }
            
            // LBS定位失败或未到间隔时间，尝试WiFi
            if (currentTime - fallbackConfig.last_wifi_time >= fallbackConfig.wifi_interval) {
                if (tryWiFiLocation()) {
                    fallbackConfig.last_wifi_time = currentTime;
                }
            }
        }
    }
}

bool FusionLocationManager::isGNSSSignalLost() {
    if (!gpsProvider || !gpsProvider->isAvailable()) {
        return true;
    }
    
    // 检查GNSS数据的时效性
    gnss_data_t& gnss = air780eg.getGNSS().gnss_data;
    unsigned long currentTime = millis();
    
    // 如果数据无效或超过超时时间，认为信号丢失
    if (!gnss.data_valid || !gnss.is_fixed) {
        return true;
    }
    
    // 检查数据是否过期
    if (currentTime - gnss.last_update > fallbackConfig.gnss_timeout) {
        return true;
    }
    
    // 检查定位类型，如果不是GNSS，也认为GNSS信号丢失
    if (gnss.location_type != "GNSS") {
        return true;
    }
    
    return false;
}

bool FusionLocationManager::tryLBSLocation() {
    if (fallbackConfig.lbs_in_progress) {
        debugPrint("LBS定位正在进行中，跳过");
        return false;
    }
    
    debugPrint("尝试LBS基站定位...");
    fallbackConfig.lbs_in_progress = true;
    
    // 使用Air780EG的LBS定位功能
    bool success = air780eg.getGNSS().updateLBS();
    
    fallbackConfig.lbs_in_progress = false;
    
    if (success) {
        stats.lbs_updates++;
        debugPrint("LBS定位成功");
        return true;
    } else {
        debugPrint("LBS定位失败");
        return false;
    }
}

bool FusionLocationManager::tryWiFiLocation() {
    if (fallbackConfig.wifi_in_progress) {
        debugPrint("WiFi定位正在进行中，跳过");
        return false;
    }
    
    debugPrint("尝试WiFi定位...");
    fallbackConfig.wifi_in_progress = true;
    
    // 使用Air780EG的WiFi定位功能
    bool success = air780eg.getGNSS().updateWIFILocation();
    
    fallbackConfig.wifi_in_progress = false;
    
    if (success) {
        stats.wifi_updates++;
        debugPrint("WiFi定位成功");
        return true;
    } else {
        debugPrint("WiFi定位失败");
        return false;
    }
}

bool FusionLocationManager::requestLBSLocation() {
    if (!initialized) {
        debugPrint("系统未初始化，无法执行LBS定位");
        return false;
    }
    
    debugPrint("手动请求LBS定位");
    return tryLBSLocation();
}

bool FusionLocationManager::requestWiFiLocation() {
    if (!initialized) {
        debugPrint("系统未初始化，无法执行WiFi定位");
        return false;
    }
    
    debugPrint("手动请求WiFi定位");
    return tryWiFiLocation();
}

String FusionLocationManager::getLocationSource() {
    if (!initialized || !gpsProvider || !gpsProvider->isAvailable()) {
        return "Unknown";
    }
    
    gnss_data_t& gnss = air780eg.getGNSS().gnss_data;
    return gnss.location_type;
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
    
    if (initialized) {
        Position pos = getFusedPosition();
        status.fusion_valid = pos.valid;
        status.last_fusion_time = pos.timestamp;
    }
    
    return status;
}

#ifdef ENABLE_FUSION_LOCATION
FusionLocationManager fusionLocationManager;
#endif
