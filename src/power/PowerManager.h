#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include <WiFi.h>
#include "device.h"
#include "imu/qmi8658.h"
#include "utils/PreferencesUtils.h"
#include "config.h"


// 简化的电源状态枚举
enum PowerState {
    POWER_STATE_NORMAL,           // 正常工作状态
    POWER_STATE_PREPARING_SLEEP   // 正在准备进入睡眠状态
};

class PowerManager {
public:
    PowerManager();
    void begin();
    void loop();
    void enterLowPowerMode();
    
    // 休眠时间管理
    void setSleepTime(unsigned long seconds);
    unsigned long getSleepTime() const;
    
    // 状态查询
    PowerState getPowerState() { return powerState; }
    bool isSleepEnabled() { return sleepEnabled; }
    
    // 测试和调试
    void testSafeEnterSleep();    // 安全的休眠测试
    
    // 唤醒处理
    void printWakeupReason();
    
    // SD卡低功耗管理
    void disableSDCard();         // 关闭SD卡并配置低功耗
    void enableSDCard();          // 重新启用SD卡

private:
    PowerState powerState;        // 当前电源状态
    unsigned long sleepTimeSec;   // 休眠时间（秒）
    unsigned long lastMotionTime; // 最后一次检测到运动的时间
    
    RTC_DATA_ATTR static bool sleepEnabled;  // 是否启用休眠功能
    
    // 核心功能方法
    bool isDeviceIdle();          // 检查设备是否空闲
    bool configureWakeupSources(); // 配置唤醒源
    void disablePeripherals();    // 关闭外设
    void configureGPIOForSleep(); // 配置GPIO低功耗模式
    void handleWakeup();          // 处理唤醒事件
    void configurePowerDomains(); // 配置电源域
    
    // 车辆状态检测（如果启用）
    #ifdef RTC_INT_PIN
    bool isVehicleStarted();
    void handleVehicleStateChange();
    #endif
};

extern PowerManager powerManager;

#endif // POWER_MANAGER_H 