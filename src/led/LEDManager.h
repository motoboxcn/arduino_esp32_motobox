#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "LEDTypes.h"  // 包含LED类型定义

class LEDManager {
public:
    LEDManager();
    void begin();
    void setLEDState(LEDMode mode, LEDColor color = LED_COLOR_GREEN, uint8_t brightness = 10);
    void loop();
    void updateChargingStatus(); // 新增：更新充电状态显示
    
private:
    LEDMode _mode;
    LEDColor _color;
    uint8_t _brightness;
    bool _lastChargingState; // 新增：记录上次充电状态
    bool _autoChargingMode;  // 新增：是否启用自动充电显示模式

    void updateLED();
    void setChargingDisplay(bool isCharging); // 新增：设置充电显示
};

#if defined(LED_PIN) || defined(PWM_LED_PIN)
extern LEDManager ledManager;
#endif

#endif
