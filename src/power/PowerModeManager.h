#ifndef POWER_MODE_MANAGER_H
#define POWER_MODE_MANAGER_H

#include <Arduino.h>
#include "config.h"

// 功耗模式枚举
enum PowerMode {
    POWER_MODE_SLEEP,       // 休眠模式 - 深度睡眠，最低功耗
    POWER_MODE_BASIC,       // 基本模式 - 低频率采集，5s GPS，降低MQTT频率
    POWER_MODE_NORMAL,      // 正常模式 - 标准频率采集，1s GPS
    POWER_MODE_SPORT        // 运动模式 - 高频率采集，1s GPS，高精度IMU
};

// 模式配置结构体
struct PowerModeConfig {
    // GPS配置
    unsigned long gps_interval_ms;          // GPS定位间隔
    unsigned long gps_log_interval_ms;      // GPS记录间隔
    
    // IMU配置
    unsigned long imu_update_interval_ms;   // IMU更新间隔
    bool imu_high_precision;                // 是否启用高精度IMU
    
    // MQTT配置
    unsigned long mqtt_location_interval_ms;    // 位置上报间隔
    unsigned long mqtt_device_status_interval_ms; // 设备状态上报间隔
    
    // 融合定位配置
    unsigned long fusion_update_interval_ms;    // 融合定位更新间隔
    
    // 系统配置
    unsigned long system_check_interval_ms;     // 系统检查间隔
    
    // 功耗相关
    unsigned long idle_timeout_ms;              // 空闲超时时间
    bool enable_peripheral_power_save;          // 是否启用外设省电
};

class PowerModeManager {
public:
    PowerModeManager();
    
    // 初始化
    void begin();
    void loop();
    
    // 模式管理
    void setMode(PowerMode mode);
    PowerMode getCurrentMode() const { return currentMode; }
    const char* getModeString() const;
    
    // 自动模式切换
    void enableAutoModeSwitch(bool enable) { autoModeEnabled = enable; }
    bool isAutoModeEnabled() const { return autoModeEnabled; }
    
    // 获取当前模式配置
    const PowerModeConfig& getCurrentConfig() const;
    
    // 手动触发模式评估
    void evaluateAndSwitchMode();
    
    // 状态查询
    bool isMotionDetected() const { return motionDetected; }
    bool isVehicleActive() const { return vehicleActive; }
    unsigned long getLastMotionTime() const { return lastMotionTime; }
    const char* getModeString(PowerMode mode) const;
    
    // 配置管理
    void setCustomConfig(PowerMode mode, const PowerModeConfig& config);
    void resetToDefaultConfig(PowerMode mode);
    
    // 调试和统计
    void printCurrentStatus();
    void printModeConfigs();
    
private:
    PowerMode currentMode;
    PowerMode previousMode;
    bool autoModeEnabled;
    
    // 状态检测
    bool motionDetected;
    bool vehicleActive;
    unsigned long lastMotionTime;
    unsigned long lastModeChangeTime;
    unsigned long lastEvaluationTime;
    
    // 模式配置
    PowerModeConfig modeConfigs[4]; // 对应4种模式
    
    // 内部方法
    void initializeDefaultConfigs();
    PowerMode evaluateOptimalMode();
    void applyModeConfig(PowerMode mode);
    bool shouldSwitchMode(PowerMode newMode);
    
    // 状态检测方法
    void updateMotionStatus();
    void updateVehicleStatus();
    bool detectHighSpeedMovement();
    bool detectPrecisionRequirement();
    
    // 配置应用方法
    void applyGPSConfig(const PowerModeConfig& config);
    void applyIMUConfig(const PowerModeConfig& config);
    void applyMQTTConfig(const PowerModeConfig& config);
    void applyFusionLocationConfig(const PowerModeConfig& config);
    void applySystemConfig(const PowerModeConfig& config);
    
    // 音频反馈
    void playModeChangeSound(PowerMode newMode);
};

extern PowerModeManager powerModeManager;

#endif // POWER_MODE_MANAGER_H