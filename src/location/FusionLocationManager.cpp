#include "FusionLocationManager.h"

const char* FusionLocationManager::TAG = "FusionLocation";

// 常量定义
#define EARTH_RADIUS 6371000.0f
// DEG_TO_RAD and RAD_TO_DEG are already defined in Arduino.h

// 条件编译的调试日志宏
#if FUSION_LOCATION_DEBUG_ENABLED
#define FUSION_LOGD(tag, format, ...) AIR780EG_LOGD(tag, format, ##__VA_ARGS__)
#else
#define FUSION_LOGD(tag, format, ...) // 禁用调试日志
#endif

// ============================================================================
// 构造函数和析构函数
// ============================================================================

FusionLocationManager::FusionLocationManager() 
    : initialized(false), debug_enabled(false),
      update_interval(100), last_update_time(0), last_debug_print_time(0), last_motion_update_time(0),
      initial_latitude(39.9042), initial_longitude(116.4074),
      originLat(0), originLng(0), hasOrigin(false),
      isStationary(false), stationaryStartTime(0),
      currentSource(SOURCE_GPS_ONLY), lastGPSUpdateTime(0), lastGPSLat(0.0), lastGPSLng(0.0), lastGPSSpeed(0.0f),
      isRecording(false), totalDistance(0), maxSpeed(0), maxLeanAngle(0),
      accelerationThreshold(1.0f), brakingThreshold(-1.0f), leanThreshold(0.175f),
      wheelieThreshold(0.35f), stoppieThreshold(-0.35f), driftThreshold(3.0f) {
    
    // 初始化速度位置数组
    for(int i = 0; i < 3; i++) {
        velocity[i] = 0.0f;
        position[i] = 0.0f;
        lastVelocity[i] = 0.0f;
    }
    
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
    
    // 初始化统计信息
    memset(&stats, 0, sizeof(stats));
}

FusionLocationManager::~FusionLocationManager() {
    // 析构函数，清理资源
}

// ============================================================================
// 初始化和主循环
// ============================================================================

bool FusionLocationManager::begin(double initLat, double initLng) {
    if (initialized) return true;
    
    Serial.printf("[%s] 初始化MadgwickAHRS融合定位系统...\n", TAG);
    
    initial_latitude = initLat;
    initial_longitude = initLng;
    originLat = initLat;
    originLng = initLng;
    hasOrigin = (initLat != 0 || initLng != 0);
    
    // 初始化Madgwick AHRS
    ahrs.begin(100.0f); // 100Hz采样率
    
    // 设置初始位置
    if (hasOrigin) {
        currentPosition.lat = originLat;
        currentPosition.lng = originLng;
        currentPosition.valid = true;
    }
    
    initialized = true;
    last_update_time = millis();
    
    Serial.printf("[%s] ✅ MadgwickAHRS融合定位系统初始化成功\n", TAG);
    Serial.printf("[%s] 初始位置: %.6f, %.6f\n", TAG, initLat, initLng);
    
    return true;
}

void FusionLocationManager::loop() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - last_update_time) / 1000.0f;
    
    if (deltaTime < 0.001f || deltaTime > 1.0f) return; // 跳过异常时间间隔
    
    // 检查GPS状态
    bool gpsLost = (currentTime - lastGPSUpdateTime > FUSION_GPS_TIMEOUT);
    
    if (gpsLost && currentSource != SOURCE_IMU_DEAD_RECKONING) {
        Serial.printf("[%s] ⚠️ GPS信号丢失，切换到纯惯导模式\n", TAG);
        currentSource = SOURCE_IMU_DEAD_RECKONING;
    }
    
    // 更新AHRS姿态
    updateAHRS();
    
    // 只有GPS丢失时才进行位置积分
    if (currentSource == SOURCE_IMU_DEAD_RECKONING) {
        updatePosition();
        calculateRelativePosition();
    }
    
    // 更新运动状态
    updateMotionState();
    
    // 零速检测和更新
    if (detectStationary()) {
        applyZUPT();
    }
    
    // 更新统计信息
    updateTrajectoryStatistics();
    
    // 处理兜底定位逻辑
    if (fallbackConfig.enabled) {
        handleFallbackLocation();
    }
    
    last_update_time = currentTime;
    
    // 每5秒输出关键信息
    if (currentTime - last_debug_print_time > 5000) {
        Serial.printf("[融合定位] 来源:%s GPS:(%.6f,%.6f,%.1fm/s) 当前:(%.6f,%.6f,%.1fm/s) 姿态:(R%.1f°P%.1f°Y%.1f°)\n",
                     currentSource == SOURCE_GPS_IMU_FUSED ? "GPS+IMU" : 
                     currentSource == SOURCE_GPS_ONLY ? "GPS" : "惯导",
                     lastGPSLat, lastGPSLng, lastGPSSpeed,
                     currentPosition.lat, currentPosition.lng, currentPosition.speed,
                     currentPosition.roll, currentPosition.pitch, currentPosition.heading);
        last_debug_print_time = currentTime;
    }
}

// ============================================================================
// AHRS姿态解算
// ============================================================================

void FusionLocationManager::updateAHRS() {
#ifdef ENABLE_IMU
    // IMU is always available when ENABLE_IMU is defined
    
    // 获取IMU数据
    float ax = imu.getAccelX() * 9.8f;  // 转换为 m/s²
    float ay = imu.getAccelY() * 9.8f;
    float az = imu.getAccelZ() * 9.8f;
    
    float gx = imu.getGyroX() * DEG_TO_RAD;  // 转换为 rad/s
    float gy = imu.getGyroY() * DEG_TO_RAD;
    float gz = imu.getGyroZ() * DEG_TO_RAD;
    
    // 如果有地磁传感器，使用9轴融合
    #ifdef ENABLE_COMPASS
    if (compass.isInitialized() && compass.isDataValid()) {
        float mx = compass.getX();
        float my = compass.getY();
        float mz = compass.getZ();
        
        // 归一化磁力计数据
        float mag_norm = sqrt(mx*mx + my*my + mz*mz);
        if (mag_norm > 0.01f) {
            mx /= mag_norm;
            my /= mag_norm;
            mz /= mag_norm;
            
            ahrs.update(gx, gy, gz, ax, ay, az, mx, my, mz);
            stats.mag_updates++;
        } else {
            ahrs.updateIMU(gx, gy, gz, ax, ay, az);
        }
    } else {
        ahrs.updateIMU(gx, gy, gz, ax, ay, az);
    }
    #else
    ahrs.updateIMU(gx, gy, gz, ax, ay, az);
    #endif
    
    stats.imu_updates++;
    stats.total_updates++;
    
    // 获取姿态角
    currentPosition.roll = ahrs.getRoll();
    currentPosition.pitch = ahrs.getPitch();
    currentPosition.heading = ahrs.getYaw();
    
    if (debug_enabled) {
        Serial.printf("[AHRS] Roll: %.1f°, Pitch: %.1f°, Yaw: %.1f°\n",
                     currentPosition.roll, currentPosition.pitch, currentPosition.heading);
    }
#endif
}

// ============================================================================
// 位置和速度解算
// ============================================================================

void FusionLocationManager::updatePosition() {
#ifdef ENABLE_IMU
    // IMU is always available when ENABLE_IMU is defined
    
    uint32_t currentTime = millis();
    float deltaTime = (currentTime - last_update_time) / 1000.0f;
    
    if (deltaTime <= 0) return;
    
    // 获取原始加速度数据
    float ax = imu.getAccelX() * 9.8f;  // X轴加速度
    float ay = imu.getAccelY() * 9.8f;  // Y轴加速度
    float az = imu.getAccelZ() * 9.8f;  // Z轴加速度
    
    // 使用Madgwick算法输出的姿态角，将加速度从机体坐标系转换到地理坐标系
    // 然后去除重力分量
    float roll_rad = currentPosition.roll * DEG_TO_RAD;
    float pitch_rad = currentPosition.pitch * DEG_TO_RAD;
    
    // 旋转矩阵转换 (简化版本，只考虑roll和pitch)
    float ax_world = ax * cos(pitch_rad) + az * sin(pitch_rad);
    float ay_world = ay * cos(roll_rad) - ax * sin(roll_rad) * sin(pitch_rad) + az * sin(roll_rad) * cos(pitch_rad);
    float az_world = -ax * sin(pitch_rad) * cos(roll_rad) + ay * sin(roll_rad) + az * cos(pitch_rad) * cos(roll_rad);
    
    // 去除重力（地理坐标系下重力在z轴）
    float linearAccel[3];
    linearAccel[0] = ax_world;
    linearAccel[1] = ay_world;
    linearAccel[2] = az_world - 9.8f;
    
    // 速度积分
    for(int i = 0; i < 3; i++) {
        velocity[i] += linearAccel[i] * deltaTime;
    }
    
    // 位置积分
    for(int i = 0; i < 3; i++) {
        position[i] += velocity[i] * deltaTime;
    }
    
    // 计算速度大小
    currentPosition.speed = sqrt(velocity[0]*velocity[0] + velocity[1]*velocity[1]);
    
    // 更新位移信息
    currentPosition.displacement.x = position[0];
    currentPosition.displacement.y = position[1];
    currentPosition.displacement.z = position[2];
    
    // 更新位置精度（基于速度的简单估计）
    currentPosition.accuracy = 5.0f + currentPosition.speed * 0.1f;
    
    if (debug_enabled) {
        Serial.printf("[Position] 速度: %.2f m/s, 位置: (%.2f, %.2f, %.2f)\n",
                     currentPosition.speed, position[0], position[1], position[2]);
    }
#endif
}

// ============================================================================
// 运动状态检测
// ============================================================================

void FusionLocationManager::updateMotionState() {
    uint32_t currentTime = millis();
    float deltaTime = (currentTime - last_motion_update_time) / 1000.0f;
    
    if (deltaTime <= 0) return;
    
    // 运动检测
    currentMotionState.isAccelerating = detectAcceleration();
    currentMotionState.isBraking = detectBraking();
    currentMotionState.isLeaning = detectLeaning();
    currentMotionState.isWheelie = detectWheelie();
    currentMotionState.isStoppie = detectStoppie();
    currentMotionState.isDrifting = detectDrifting();
    
    // 计算倾斜角（基于roll角）
    currentPosition.leanAngle = abs(currentPosition.roll);
    currentMotionState.leanAngle = currentPosition.leanAngle;
    
    // 计算倾斜角速度
    if (deltaTime > 0) {
        currentMotionState.leanRate = (currentPosition.leanAngle - currentMotionState.leanAngle) / deltaTime;
    }
    
    // 计算加速度
    currentMotionState.forwardAccel = velocity[0] - lastVelocity[0];
    currentMotionState.lateralAccel = velocity[1] - lastVelocity[1];
    
    // 更新历史数据
    for(int i = 0; i < 3; i++) {
        lastVelocity[i] = velocity[i];
    }
    last_motion_update_time = currentTime;
    currentMotionState.timestamp = currentTime;
    
    if (debug_enabled) {
        Serial.printf("[Motion] 倾斜: %.1f°, 加速: %s, 制动: %s\n",
                     currentPosition.leanAngle,
                     currentMotionState.isAccelerating ? "是" : "否",
                     currentMotionState.isBraking ? "是" : "否");
    }
}

// 运动检测方法
bool FusionLocationManager::detectAcceleration() {
    return currentMotionState.forwardAccel > accelerationThreshold;
}

bool FusionLocationManager::detectBraking() {
    return currentMotionState.forwardAccel < brakingThreshold;
}

bool FusionLocationManager::detectLeaning() {
    return currentPosition.leanAngle > leanThreshold;
}

bool FusionLocationManager::detectWheelie() {
    return currentPosition.pitch > wheelieThreshold;
}

bool FusionLocationManager::detectStoppie() {
    return currentPosition.pitch < stoppieThreshold;
}

bool FusionLocationManager::detectDrifting() {
    return abs(currentMotionState.lateralAccel) > driftThreshold;
}

// ============================================================================
// 零速检测和更新
// ============================================================================

bool FusionLocationManager::detectStationary() {
    // 多条件判断静止状态
    float speedThreshold = 0.3f;  // 降低速度阈值到0.3 m/s
    float accelThreshold = 0.2f;  // 加速度阈值 (m/s²)
    
    // 获取当前加速度数据计算幅值
    float ax = imu.getAccelX() * 9.8f;
    float ay = imu.getAccelY() * 9.8f;
    float az = imu.getAccelZ() * 9.8f;
    
    // 计算加速度幅值
    float accel_magnitude = sqrt(ax*ax + ay*ay + az*az);
    
    // 同时满足：速度低、加速度小
    bool currentlyStationary = (currentPosition.speed < speedThreshold) && 
                              (accel_magnitude < accelThreshold);
    
    if (currentlyStationary && !isStationary) {
        stationaryStartTime = millis();
        isStationary = true;
    } else if (!currentlyStationary) {
        isStationary = false;
    }
    
    return isStationary && (millis() - stationaryStartTime > STATIONARY_THRESHOLD);
}

void FusionLocationManager::applyZUPT() {
    // 零速更新：重置速度和部分位置误差
    for(int i = 0; i < 3; i++) {
        velocity[i] *= 0.1f;  // 衰减而非直接清零，避免突变
    }
    
    if (debug_enabled) {
        Serial.println("[ZUPT] 应用零速更新，速度衰减");
    }
}

// ============================================================================
// 坐标转换
// ============================================================================

void FusionLocationManager::calculateRelativePosition() {
    if (!hasOrigin) return;
    
    // 将本地坐标转换为经纬度
    xyToLatLng(position[0], position[1], currentPosition.lat, currentPosition.lng);
    currentPosition.altitude = position[2];
    currentPosition.timestamp = millis();
    currentPosition.valid = true;
}

void FusionLocationManager::latLngToXY(double lat, double lng, float& x, float& y) {
    if (!hasOrigin) {
        x = y = 0;
        return;
    }
    
    double deltaLat = (lat - originLat) * DEG_TO_RAD;
    double deltaLng = (lng - originLng) * DEG_TO_RAD;
    
    y = deltaLat * EARTH_RADIUS;
    x = deltaLng * EARTH_RADIUS * cos(originLat * DEG_TO_RAD);
}

void FusionLocationManager::xyToLatLng(float x, float y, double& lat, double& lng) {
    if (!hasOrigin) {
        lat = lng = 0;
        return;
    }
    
    double deltaLat = y / EARTH_RADIUS;
    double deltaLng = x / (EARTH_RADIUS * cos(originLat * DEG_TO_RAD));
    
    lat = originLat + deltaLat * RAD_TO_DEG;
    lng = originLng + deltaLng * RAD_TO_DEG;
}

// ============================================================================
// 轨迹管理
// ============================================================================

void FusionLocationManager::updateTrajectoryStatistics() {
    // 更新最大速度
    if (currentPosition.speed > maxSpeed) {
        maxSpeed = currentPosition.speed;
    }
    
    // 更新最大倾斜角
    if (currentPosition.leanAngle > maxLeanAngle) {
        maxLeanAngle = currentPosition.leanAngle;
    }
    
    // 计算总距离（简化实现）
    static float lastX = 0, lastY = 0;
    if (lastX != 0 || lastY != 0) {
        float dx = position[0] - lastX;
        float dy = position[1] - lastY;
        totalDistance += sqrt(dx*dx + dy*dy);
    }
    lastX = position[0];
    lastY = position[1];
}

void FusionLocationManager::startRecording() {
    isRecording = true;
    totalDistance = 0;
    maxSpeed = 0;
    maxLeanAngle = 0;
    
    if (debug_enabled) {
        Serial.println("[Trajectory] 开始记录轨迹");
    }
}

void FusionLocationManager::stopRecording() {
    isRecording = false;
    
    if (debug_enabled) {
        Serial.println("[Trajectory] 停止记录轨迹");
    }
}

void FusionLocationManager::clearTrajectory() {
    totalDistance = 0;
    maxSpeed = 0;
    maxLeanAngle = 0;
    
    if (debug_enabled) {
        Serial.println("[Trajectory] 清除轨迹记录");
    }
}

// ============================================================================
// 公共接口方法
// ============================================================================

MotorcycleTrajectoryPoint FusionLocationManager::getCurrentPosition() {
    return currentPosition;
}

Position FusionLocationManager::getFusedPosition() {
    return currentPosition;  // Position is typedef for MotorcycleTrajectoryPoint
}

MotorcycleMotionState FusionLocationManager::getMotionState() {
    return currentMotionState;
}

float FusionLocationManager::getPositionAccuracy() {
    // 简化的精度估计
    return 5.0f; // 5米精度
}

float FusionLocationManager::getHeading() {
    return currentPosition.heading;
}

float FusionLocationManager::getSpeed() {
    return currentPosition.speed;
}

bool FusionLocationManager::isPositionValid() {
    return initialized && currentPosition.valid;
}

void FusionLocationManager::setOrigin(double lat, double lng) {
    originLat = lat;
    originLng = lng;
    hasOrigin = true;
    
    // 重置位置积分
    for(int i = 0; i < 3; i++) {
        position[i] = 0.0f;
        velocity[i] = 0.0f;
    }
    
    if (debug_enabled) {
        Serial.printf("[Origin] 起始点已设置为: %.6f, %.6f\n", lat, lng);
    }
}

void FusionLocationManager::resetOrigin() {
    if (initialized && currentPosition.valid) {
        setOrigin(currentPosition.lat, currentPosition.lng);
        if (debug_enabled) {
            Serial.println("[Origin] 起始点已重置为当前位置");
        }
    }
}

void FusionLocationManager::setUpdateInterval(unsigned long interval_ms) {
    update_interval = interval_ms;
    if (debug_enabled) {
        Serial.printf("[Config] 更新间隔设置为: %lu ms\n", interval_ms);
    }
}

void FusionLocationManager::setDebug(bool enable) {
    debug_enabled = enable;
    Serial.printf("[%s] 调试模式: %s\n", TAG, enable ? "开启" : "关闭");
}

float FusionLocationManager::getTotalDistance() {
    return totalDistance;
}

float FusionLocationManager::getMaxSpeed() {
    return maxSpeed;
}

float FusionLocationManager::getMaxLeanAngle() {
    return maxLeanAngle;
}

void FusionLocationManager::resetStats() {
    totalDistance = 0;
    maxSpeed = 0;
    maxLeanAngle = 0;
    memset(&stats, 0, sizeof(stats));
    
    if (debug_enabled) {
        Serial.println("[Stats] 统计信息已重置");
    }
}

// ============================================================================
// 状态打印和调试
// ============================================================================

void FusionLocationManager::printStatus() {
    if (!initialized) {
        Serial.printf("[%s] 系统未初始化\n", TAG);
        return;
    }
    
    Serial.println("=== 融合定位状态 ===");
    
    // 定位来源
    const char* sourceStr[] = {"GPS", "GPS+IMU", "纯惯导"};
    Serial.printf("定位来源: %s\n", sourceStr[currentSource]);
    
    // GPS位置和速度
    if (currentSource == SOURCE_GPS_IMU_FUSED || currentSource == SOURCE_GPS_ONLY) {
        Serial.printf("GPS位置: %.6f, %.6f\n", lastGPSLat, lastGPSLng);
        Serial.printf("GPS速度: %.2f m/s (%.1f km/h)\n", lastGPSSpeed, lastGPSSpeed * 3.6f);
    }
    
    // 当前融合位置
    Serial.printf("当前位置: %.6f, %.6f\n", currentPosition.lat, currentPosition.lng);
    Serial.printf("当前速度: %.2f m/s (%.1f km/h)\n", currentPosition.speed, currentPosition.speed * 3.6f);
    
    // 位移增量（相对于上次GPS位置）
    if (currentSource == SOURCE_IMU_DEAD_RECKONING) {
        Serial.printf("惯导位移: (%.1f, %.1f, %.1f) m\n", 
                     position[0], position[1], position[2]);
        Serial.printf("GPS丢失时长: %lu 秒\n", (millis() - lastGPSUpdateTime) / 1000);
    }
    
    // 姿态
    Serial.printf("姿态: Roll=%.1f° Pitch=%.1f° Yaw=%.1f°\n",
                 currentPosition.roll, currentPosition.pitch, currentPosition.heading);
    
    // 统计
    Serial.printf("更新次数: GPS=%lu IMU=%lu MAG=%lu\n", 
                 stats.gps_updates, stats.imu_updates, stats.mag_updates);
}

void FusionLocationManager::printStats() {
    Serial.println("=== MadgwickAHRS融合定位统计信息 ===");
    Serial.printf("总距离: %.1fm\n", totalDistance);
    Serial.printf("最大速度: %.1fm/s\n", maxSpeed);
    Serial.printf("最大倾斜角: %.1f°\n", maxLeanAngle);
    Serial.printf("总更新次数: %lu\n", stats.total_updates);
    Serial.printf("融合更新: %lu | GPS: %lu | IMU: %lu | MAG: %lu\n", 
                 stats.fusion_updates, stats.gps_updates, stats.imu_updates, stats.mag_updates);
    Serial.printf("兜底定位: LBS: %lu | WiFi: %lu\n", stats.lbs_updates, stats.wifi_updates);
}

String FusionLocationManager::getPositionJSON() {
    if (!initialized || !currentPosition.valid) {
        return "{}";
    }
    
    String json = "{";
    json += "\"latitude\":" + String(currentPosition.lat, 6) + ",";
    json += "\"longitude\":" + String(currentPosition.lng, 6) + ",";
    json += "\"altitude\":" + String(currentPosition.altitude, 2) + ",";
    json += "\"speed\":" + String(currentPosition.speed, 2) + ",";
    json += "\"course\":" + String(currentPosition.heading, 1) + ",";
    json += "\"hdop\":" + String(getPositionAccuracy(), 1) + ",";
    json += "\"timestamp\":" + String(currentPosition.timestamp) + ",";
    json += "\"location_type\":\"MADGWICK_AHRS\",";
    json += "\"satellites\":0,";
    json += "\"is_fixed\":true,";
    json += "\"data_valid\":true,";
    
    // 添加姿态信息
    json += "\"attitude\":{";
    json += "\"roll\":" + String(currentPosition.roll, 1) + ",";
    json += "\"pitch\":" + String(currentPosition.pitch, 1) + ",";
    json += "\"yaw\":" + String(currentPosition.heading, 1) + ",";
    json += "\"lean_angle\":" + String(currentPosition.leanAngle, 1);
    json += "},";
    
    // 添加运动状态
    json += "\"motion\":{";
    json += "\"is_accelerating\":" + String(currentMotionState.isAccelerating ? "true" : "false") + ",";
    json += "\"is_braking\":" + String(currentMotionState.isBraking ? "true" : "false") + ",";
    json += "\"is_leaning\":" + String(currentMotionState.isLeaning ? "true" : "false") + ",";
    json += "\"is_wheelie\":" + String(currentMotionState.isWheelie ? "true" : "false") + ",";
    json += "\"is_stoppie\":" + String(currentMotionState.isStoppie ? "true" : "false") + ",";
    json += "\"is_drifting\":" + String(currentMotionState.isDrifting ? "true" : "false");
    json += "}";
    
    json += "}";
    return json;
}

void FusionLocationManager::debugPrint(const String& message) {
    if (debug_enabled) {
        Serial.printf("[%s] %s\n", TAG, message.c_str());
    }
}

// ============================================================================
// 兜底定位相关方法
// ============================================================================

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
    
    // 设置初始时间为较早的时间，确保启动后能立即尝试定位
    unsigned long currentTime = millis();
    fallbackConfig.last_lbs_time = currentTime - lbs_interval;
    fallbackConfig.last_wifi_time = currentTime - wifi_interval;
    
    debugPrint("兜底定位配置: " + String(enable ? "启用" : "禁用") + 
               ", GNSS超时:" + String(gnss_timeout/1000) + "s" +
               ", LBS间隔:" + String(lbs_interval/1000) + "s" +
               ", WiFi间隔:" + String(wifi_interval/1000) + "s" +
               ", 优先WiFi:" + String(prefer_wifi ? "是" : "否"));
}

void FusionLocationManager::handleFallbackLocation() {
    if (!fallbackConfig.enabled) {
        FUSION_LOGD(TAG, "兜底定位未启用");
        return;
    }
    
    unsigned long currentTime = millis();
    
    // 检查GNSS信号是否丢失
    bool gnssLost = isGNSSSignalLost();
    FUSION_LOGD(TAG, "GNSS信号状态: %s", gnssLost ? "丢失" : "正常");
    
    if (!gnssLost) {
        return;
    }
    
    // 根据配置决定使用WiFi还是LBS
    if (fallbackConfig.prefer_wifi_over_lbs) {
        unsigned long wifi_elapsed = currentTime - fallbackConfig.last_wifi_time;
        if (wifi_elapsed >= fallbackConfig.wifi_interval) {
            debugPrint("尝试WiFi定位...");
            bool wifi_success = tryWiFiLocation();
            fallbackConfig.last_wifi_time = currentTime;
            
            if (!wifi_success) {
                unsigned long lbs_elapsed = currentTime - fallbackConfig.last_lbs_time;
                if (lbs_elapsed >= fallbackConfig.lbs_interval) {
                    debugPrint("WiFi定位失败，尝试LBS定位");
                    tryLBSLocation();
                    fallbackConfig.last_lbs_time = currentTime;
                }
            }
        }
    } else {
        unsigned long lbs_elapsed = currentTime - fallbackConfig.last_lbs_time;
        if (lbs_elapsed >= fallbackConfig.lbs_interval) {
            debugPrint("尝试LBS定位...");
            bool lbs_success = tryLBSLocation();
            fallbackConfig.last_lbs_time = currentTime;
            
            if (!lbs_success) {
                unsigned long wifi_elapsed = currentTime - fallbackConfig.last_wifi_time;
                if (wifi_elapsed >= fallbackConfig.wifi_interval) {
                    debugPrint("LBS定位失败，尝试WiFi定位");
                    tryWiFiLocation();
                    fallbackConfig.last_wifi_time = currentTime;
                }
            }
        }
    }
}

bool FusionLocationManager::isGNSSSignalLost() {
    // 简化实现：总是返回false，因为MadgwickAHRS不依赖GPS
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
    return "MADGWICK_AHRS";
}

FusionLocationManager::DataSourceStatus FusionLocationManager::getDataSourceStatus() {
    DataSourceStatus status;
    memset(&status, 0, sizeof(status));
    
    status.imu_available = true;  // MadgwickAHRS总是使用IMU
    status.gps_available = false; // 不依赖GPS
    status.mag_available = false; // 不依赖磁力计
    status.fusion_valid = currentPosition.valid;
    status.last_imu_time = millis();
    status.last_fusion_time = currentPosition.timestamp;
    
    return status;
}

// ============================================================================
// 兼容性方法实现
// ============================================================================

void FusionLocationManager::calibrateIMU() {
    if (debug_enabled) {
        Serial.println("[FusionLocation] IMU校准功能暂未实现");
    }
    // TODO: 实现IMU校准逻辑
}

void FusionLocationManager::enableGravityCompensation() {
    if (debug_enabled) {
        Serial.println("[FusionLocation] 重力补偿已启用");
    }
    // Madgwick算法已经包含重力补偿
}

void FusionLocationManager::disableGravityCompensation() {
    if (debug_enabled) {
        Serial.println("[FusionLocation] 重力补偿已禁用");
    }
    // Madgwick算法总是包含重力补偿，无法禁用
}

void FusionLocationManager::resetDisplacement() {
    // 重置位移积分
    for(int i = 0; i < 3; i++) {
        position[i] = 0.0f;
        velocity[i] = 0.0f;
    }
    
    // 重置位移结构体
    currentPosition.displacement.x = 0.0f;
    currentPosition.displacement.y = 0.0f;
    currentPosition.displacement.z = 0.0f;
    
    if (debug_enabled) {
        Serial.println("[FusionLocation] 位移已重置");
    }
}

void FusionLocationManager::updateWithGPS(double gpsLat, double gpsLng, float gpsSpeed, bool gpsValid) {
    if (gpsValid) {
        lastGPSUpdateTime = millis();
        lastGPSLat = gpsLat;
        lastGPSLng = gpsLng;
        lastGPSSpeed = gpsSpeed;
        
        // GPS可用时，校正IMU位置
        currentPosition.lat = gpsLat;
        currentPosition.lng = gpsLng;
        currentPosition.speed = gpsSpeed;
        
        // 重置IMU积分误差
        setOrigin(gpsLat, gpsLng);
        
        currentSource = SOURCE_GPS_IMU_FUSED;
        stats.gps_updates++;
        
        if (debug_enabled) {
            Serial.printf("[GPS更新] 位置:(%.6f,%.6f) 速度:%.2fm/s\n", gpsLat, gpsLng, gpsSpeed);
        }
    }
}

// 全局融合定位管理器实例
#ifdef ENABLE_IMU_FUSION
FusionLocationManager fusionLocationManager;
#endif