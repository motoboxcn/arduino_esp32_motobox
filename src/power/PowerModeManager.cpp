#include "PowerModeManager.h"
#include "device.h"
#include "Air780EG.h"

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_FUSION_LOCATION
#include "location/FusionLocationManager.h"
#endif


#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif

PowerModeManager powerModeManager;

PowerModeManager::PowerModeManager() {
    currentMode = POWER_MODE_NORMAL;
    previousMode = POWER_MODE_NORMAL;
    autoModeEnabled = true;
    
    motionDetected = false;
    vehicleActive = false;
    lastMotionTime = 0;
    lastModeChangeTime = 0;
    lastEvaluationTime = 0;
}

void PowerModeManager::begin() {
    Serial.println("[功耗管理] 初始化功耗模式管理器");
    
    // 初始化默认配置
    initializeDefaultConfigs();
    
    // 设置初始模式
    setMode(POWER_MODE_NORMAL);
    
    Serial.println("[功耗管理] 功耗模式管理器初始化完成");
    Serial.printf("[功耗管理] 当前模式: %s\n", getModeString());
    Serial.printf("[功耗管理] 自动模式切换: %s\n", autoModeEnabled ? "启用" : "禁用");
}

void PowerModeManager::initializeDefaultConfigs() {
    // 休眠模式配置
    modeConfigs[POWER_MODE_SLEEP] = {
        .gps_interval_ms = 0,                    // 不更新GPS
        .gps_log_interval_ms = 0,                // 不记录GPS
        .imu_update_interval_ms = 0,             // 不更新IMU
        .imu_high_precision = false,
        .mqtt_location_interval_ms = 0,          // 不发送MQTT
        .mqtt_device_status_interval_ms = 0,
        .fusion_update_interval_ms = 0,          // 不更新融合定位
        .system_check_interval_ms = 0,           // 不检查系统
        .idle_timeout_ms = 0,                    // 立即进入
        .enable_peripheral_power_save = true
    };
    
    // 基本模式配置 - 低功耗，基础功能
    modeConfigs[POWER_MODE_BASIC] = {
        .gps_interval_ms = 5000,                 // 5秒GPS定位
        .gps_log_interval_ms = 10000,            // 10秒记录一次
        .imu_update_interval_ms = 200,           // 200ms IMU更新 (5Hz)
        .imu_high_precision = false,
        .mqtt_location_interval_ms = 15000,      // 15秒上报位置
        .mqtt_device_status_interval_ms = 60000, // 60秒上报状态
        .fusion_update_interval_ms = 500,        // 500ms融合定位更新
        .system_check_interval_ms = 2000,        // 2秒系统检查
        .idle_timeout_ms = 120000,               // 2分钟无运动切换到休眠
        .enable_peripheral_power_save = true
    };
    
    // 正常模式配置 - 标准功能
    modeConfigs[POWER_MODE_NORMAL] = {
        .gps_interval_ms = 1000,                 // 1秒GPS定位
        .gps_log_interval_ms = 5000,             // 5秒记录一次
        .imu_update_interval_ms = 100,           // 100ms IMU更新 (10Hz)
        .imu_high_precision = false,
        .mqtt_location_interval_ms = 5000,       // 5秒上报位置
        .mqtt_device_status_interval_ms = 30000, // 30秒上报状态
        .fusion_update_interval_ms = 100,        // 100ms融合定位更新
        .system_check_interval_ms = 1000,        // 1秒系统检查
        .idle_timeout_ms = 300000,               // 5分钟无运动切换到基本模式
        .enable_peripheral_power_save = false
    };
    
    // 运动模式配置 - 高精度，高频率
    modeConfigs[POWER_MODE_SPORT] = {
        .gps_interval_ms = 1000,                 // 1秒GPS定位（最高频率）
        .gps_log_interval_ms = 1000,             // 1秒记录一次（高精度记录）
        .imu_update_interval_ms = 50,            // 50ms IMU更新 (20Hz)
        .imu_high_precision = true,              // 启用高精度IMU
        .mqtt_location_interval_ms = 2000,       // 2秒上报位置（更频繁）
        .mqtt_device_status_interval_ms = 15000, // 15秒上报状态
        .fusion_update_interval_ms = 50,         // 50ms融合定位更新（高频）
        .system_check_interval_ms = 500,         // 500ms系统检查
        .idle_timeout_ms = 60000,                // 1分钟无运动切换到正常模式
        .enable_peripheral_power_save = false
    };
    
    Serial.println("[功耗管理] 默认模式配置已初始化");
}

void PowerModeManager::loop() {
    unsigned long currentTime = millis();
    
    // 更新状态检测
    updateMotionStatus();
    updateVehicleStatus();
    
    // 自动模式切换评估（每5秒评估一次）
    if (autoModeEnabled && (currentTime - lastEvaluationTime > 5000)) {
        lastEvaluationTime = currentTime;
        evaluateAndSwitchMode();
    }
}

void PowerModeManager::updateMotionStatus() {
    #ifdef ENABLE_IMU
    if (imu.detectMotion()) {
        if (!motionDetected) {
            Serial.println("[功耗管理] 检测到运动开始");
            motionDetected = true;
        }
        lastMotionTime = millis();
    } else {
        if (motionDetected) {
            // 运动停止，但保持状态一段时间避免频繁切换
            unsigned long timeSinceLastMotion = millis() - lastMotionTime;
            if (timeSinceLastMotion > 10000) { // 10秒无运动才认为停止
                Serial.println("[功耗管理] 运动停止");
                motionDetected = false;
            }
        }
    }
    #endif
}

void PowerModeManager::updateVehicleStatus() {
    #ifdef RTC_INT_PIN
    bool currentVehicleState = (digitalRead(RTC_INT_PIN) == LOW);
    if (currentVehicleState != vehicleActive) {
        vehicleActive = currentVehicleState;
        if (vehicleActive) {
            Serial.println("[功耗管理] 车辆启动");
            lastMotionTime = millis(); // 车辆启动也算作运动
        } else {
            Serial.println("[功耗管理] 车辆关闭");
        }
    }
    #endif
}

PowerMode PowerModeManager::evaluateOptimalMode() {
    unsigned long currentTime = millis();
    unsigned long timeSinceLastMotion = currentTime - lastMotionTime;
    
    // 检测高速运动或需要高精度的场景
    bool highSpeedMovement = detectHighSpeedMovement();
    bool precisionRequired = detectPrecisionRequirement();
    
    // 模式决策逻辑
    if (vehicleActive || motionDetected) {
        if (highSpeedMovement || precisionRequired) {
            return POWER_MODE_SPORT;  // 高速运动或需要高精度
        } else {
            return POWER_MODE_NORMAL; // 正常运动
        }
    } else {
        // 无运动状态，根据时间决定
        const PowerModeConfig& currentConfig = getCurrentConfig();
        
        if (timeSinceLastMotion > currentConfig.idle_timeout_ms) {
            if (currentMode == POWER_MODE_BASIC) {
                return POWER_MODE_SLEEP;  // 从基本模式进入休眠
            } else {
                return POWER_MODE_BASIC;  // 从正常/运动模式进入基本模式
            }
        }
    }
    
    return currentMode; // 保持当前模式
}

bool PowerModeManager::detectHighSpeedMovement() {
    // 通过GPS速度判断是否高速运动
    #ifdef USE_AIR780EG_GSM
    if (air780eg.getGNSS().gnss_data.is_gnss_valid && air780eg.getGNSS().gnss_data.speed > 30.0) {
        return true; // 速度超过30km/h认为是高速运动
    }
    #endif
    
    // 通过IMU加速度判断剧烈运动
    #ifdef ENABLE_IMU
    if (imu.getAccelMagnitude() > 15.0) { // 加速度超过1.5g
        return true;
    }
    #endif
    
    return false;
}

bool PowerModeManager::detectPrecisionRequirement() {
    // 可以根据用户设置或特定场景判断是否需要高精度
    // 例如：检测到急转弯、急加速、急刹车等场景
    
    #ifdef ENABLE_IMU
    // 检测角速度变化，判断是否在进行精细操作
    if (abs(imu.getGyroZ()) > 100.0) { // 角速度超过100度/秒
        return true;
    }
    #endif
    
    return false;
}

void PowerModeManager::evaluateAndSwitchMode() {
    PowerMode optimalMode = evaluateOptimalMode();
    
    if (shouldSwitchMode(optimalMode)) {
        Serial.printf("[功耗管理] 模式切换: %s -> %s\n", 
                     getModeString(), getModeString(optimalMode));
        setMode(optimalMode);
    }
}

bool PowerModeManager::shouldSwitchMode(PowerMode newMode) {
    if (newMode == currentMode) {
        return false;
    }
    
    // 避免频繁切换，至少间隔30秒
    unsigned long currentTime = millis();
    if (currentTime - lastModeChangeTime < 30000) {
        return false;
    }
    
    // 特殊情况：立即切换到运动模式或休眠模式
    if (newMode == POWER_MODE_SPORT || newMode == POWER_MODE_SLEEP) {
        return true;
    }
    
    return true;
}

void PowerModeManager::setMode(PowerMode mode) {
    if (mode == currentMode) {
        return;
    }
    
    previousMode = currentMode;
    currentMode = mode;
    lastModeChangeTime = millis();
    
    Serial.printf("[功耗管理] 切换到模式: %s\n", getModeString());
    
    // 应用新模式配置
    applyModeConfig(mode);
    
    // 播放模式切换音效
    playModeChangeSound(mode);
    
    // 更新设备状态
    get_device_state()->power_mode = (int)mode;
}

const char* PowerModeManager::getModeString() const {
    return getModeString(currentMode);
}

const char* PowerModeManager::getModeString(PowerMode mode) const {
    switch (mode) {
        case POWER_MODE_SLEEP:  return "休眠模式";
        case POWER_MODE_BASIC:  return "基本模式";
        case POWER_MODE_NORMAL: return "正常模式";
        case POWER_MODE_SPORT:  return "运动模式";
        default: return "未知模式";
    }
}

const PowerModeConfig& PowerModeManager::getCurrentConfig() const {
    return modeConfigs[currentMode];
}

void PowerModeManager::applyModeConfig(PowerMode mode) {
    const PowerModeConfig& config = modeConfigs[mode];
    
    Serial.printf("[功耗管理] 应用模式配置: %s\n", getModeString(mode));
    
    // 如果是休眠模式，直接进入休眠
    if (mode == POWER_MODE_SLEEP) {
        Serial.println("[功耗管理] 进入休眠模式");
        extern PowerManager powerManager;
        powerManager.enterLowPowerMode();
        return;
    }
    
    // 应用各子系统配置
    applyGPSConfig(config);
    applyIMUConfig(config);
    applyMQTTConfig(config);
    applyFusionLocationConfig(config);
    applySystemConfig(config);
    
    Serial.println("[功耗管理] 模式配置应用完成");
}

void PowerModeManager::applyGPSConfig(const PowerModeConfig& config) {
    #ifdef USE_AIR780EG_GSM
    // 设置GPS更新频率
    if (config.gps_interval_ms > 0) {
        // 注意：Air780EG的GNSS更新频率通常固定为1Hz
        // 这里主要是控制我们读取和处理GPS数据的频率
        Serial.printf("[功耗管理] GPS配置 - 间隔: %lums, 记录间隔: %lums\n", 
                     config.gps_interval_ms, config.gps_log_interval_ms);
    } else {
        Serial.println("[功耗管理] GPS配置 - 已禁用");
    }
    #endif
}

void PowerModeManager::applyIMUConfig(const PowerModeConfig& config) {
    #ifdef ENABLE_IMU
    if (config.imu_update_interval_ms > 0) {
        // 设置IMU更新频率和精度
        if (config.imu_high_precision) {
            imu.setHighPrecisionMode(true);
            Serial.println("[功耗管理] IMU配置 - 高精度模式");
        } else {
            imu.setHighPrecisionMode(false);
            Serial.println("[功耗管理] IMU配置 - 标准精度模式");
        }
        
        Serial.printf("[功耗管理] IMU更新间隔: %lums\n", config.imu_update_interval_ms);
    } else {
        Serial.println("[功耗管理] IMU配置 - 已禁用");
    }
    #endif
}

void PowerModeManager::applyMQTTConfig(const PowerModeConfig& config) {
    #ifdef USE_AIR780EG_GSM
    if (config.mqtt_location_interval_ms > 0) {
        // 由于Air780EG库不支持动态更新任务间隔，我们需要重新创建任务
        // 先移除现有任务
        air780eg.getMQTT().removeScheduledTask("telemetry");
        
        // 重新添加统一遥测任务
        extern Device device;
        air780eg.getMQTT().addScheduledTask("telemetry", 
            "vehicle/v1/" + get_device_state()->device_id + "/telemetry", 
            []() { return device.getCombinedTelemetryJSON(); }, 
            config.mqtt_location_interval_ms, 0, false);
        
        Serial.printf("[功耗管理] MQTT配置 - 遥测间隔: %lums\n", 
                     config.mqtt_location_interval_ms);
    } else {
        // 禁用MQTT任务
        air780eg.getMQTT().disableScheduledTask("telemetry");
        Serial.println("[功耗管理] MQTT配置 - 已禁用");
    }
    #endif
}

void PowerModeManager::applyFusionLocationConfig(const PowerModeConfig& config) {
    #ifdef ENABLE_FUSION_LOCATION
    if (config.fusion_update_interval_ms > 0) {
        fusionLocationManager.setUpdateInterval(config.fusion_update_interval_ms);
        Serial.printf("[功耗管理] 融合定位更新间隔: %lums\n", config.fusion_update_interval_ms);
    } else {
        Serial.println("[功耗管理] 融合定位 - 已禁用");
    }
    #endif
}

void PowerModeManager::applySystemConfig(const PowerModeConfig& config) {
    // 应用系统级配置
    if (config.enable_peripheral_power_save) {
        Serial.println("[功耗管理] 启用外设省电模式");
        // 这里可以添加外设省电的具体实现
    }
    
    Serial.printf("[功耗管理] 系统检查间隔: %lums\n", config.system_check_interval_ms);
}

void PowerModeManager::playModeChangeSound(PowerMode newMode) {
    return;
}

void PowerModeManager::printCurrentStatus() {
    Serial.println("\n=== 功耗模式状态 ===");
    Serial.printf("当前模式: %s\n", getModeString());
    Serial.printf("自动切换: %s\n", autoModeEnabled ? "启用" : "禁用");
    Serial.printf("运动检测: %s\n", motionDetected ? "是" : "否");
    Serial.printf("车辆状态: %s\n", vehicleActive ? "启动" : "关闭");
    Serial.printf("最后运动时间: %lu秒前\n", (millis() - lastMotionTime) / 1000);
    
    const PowerModeConfig& config = getCurrentConfig();
    Serial.println("\n当前模式配置:");
    Serial.printf("  GPS间隔: %lums\n", config.gps_interval_ms);
    Serial.printf("  IMU间隔: %lums\n", config.imu_update_interval_ms);
    Serial.printf("  MQTT位置间隔: %lums\n", config.mqtt_location_interval_ms);
    Serial.printf("  融合定位间隔: %lums\n", config.fusion_update_interval_ms);
    Serial.printf("  空闲超时: %lums\n", config.idle_timeout_ms);
    Serial.println("==================\n");
}

void PowerModeManager::setCustomConfig(PowerMode mode, const PowerModeConfig& config) {
    modeConfigs[mode] = config;
    Serial.printf("[功耗管理] 自定义配置已设置: %s\n", getModeString(mode));
    
    // 如果是当前模式，立即应用
    if (mode == currentMode) {
        applyModeConfig(mode);
    }
}

void PowerModeManager::resetToDefaultConfig(PowerMode mode) {
    Serial.printf("[功耗管理] 重置为默认配置: %s\n", getModeString(mode));
    initializeDefaultConfigs(); // 重新初始化所有默认配置
    
    // 如果是当前模式，立即应用
    if (mode == currentMode) {
        applyModeConfig(mode);
    }
}

void PowerModeManager::printModeConfigs() {
    Serial.println("\n=== 所有功耗模式配置 ===");
    
    const char* modeNames[] = {"休眠模式", "基本模式", "正常模式", "运动模式"};
    
    for (int i = 0; i < 4; i++) {
        const PowerModeConfig& config = modeConfigs[i];
        Serial.printf("\n%s %s:\n", modeNames[i], (i == currentMode) ? "(当前)" : "");
        Serial.printf("  GPS间隔: %lums\n", config.gps_interval_ms);
        Serial.printf("  GPS记录间隔: %lums\n", config.gps_log_interval_ms);
        Serial.printf("  IMU间隔: %lums\n", config.imu_update_interval_ms);
        Serial.printf("  IMU高精度: %s\n", config.imu_high_precision ? "是" : "否");
        Serial.printf("  MQTT位置间隔: %lums\n", config.mqtt_location_interval_ms);
        Serial.printf("  MQTT状态间隔: %lums\n", config.mqtt_device_status_interval_ms);
        Serial.printf("  融合定位间隔: %lums\n", config.fusion_update_interval_ms);
        Serial.printf("  系统检查间隔: %lums\n", config.system_check_interval_ms);
        Serial.printf("  空闲超时: %lums (%lu秒)\n", config.idle_timeout_ms, config.idle_timeout_ms / 1000);
        Serial.printf("  外设省电: %s\n", config.enable_peripheral_power_save ? "启用" : "禁用");
    }
    
    Serial.println("\n========================\n");
}