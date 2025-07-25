#include "LEDManager.h"
#include "device.h"
#include "led/LED.h"
#include "led/PWMLED.h"
#include "bat/BAT.h"  // 新增：包含电池管理

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager::LEDManager() : _mode(LED_OFF), _color(LED_COLOR_GREEN), _brightness(10), 
                          _lastChargingState(false), _autoChargingMode(true) {}
#endif


void LEDManager::begin() {
#ifdef PWM_LED_PIN
    pwmLed.begin();
     // PWM LED 彩虹色初始化
    pwmLed.initRainbow();
     // PWM LED 初始设置为绿色2%亮度常亮（未充电状态）
    pwmLed.setMode(LED_ON);
    pwmLed.setColor(LED_COLOR_GREEN);
    pwmLed.setBrightness(5); // 2% of 255 ≈ 5
    
    // 启用自动充电显示模式
    _autoChargingMode = true;

#endif
#ifdef LED_PIN
    led.begin();
     // 普通 LED 闪烁两次
    led.initBlink(2);
    // 普通 LED 设置为常亮
    led.setMode(LED_ON);
#endif
}


void LEDManager::setLEDState(LEDMode mode, LEDColor color, uint8_t brightness) {
    // 如果手动设置LED状态，则暂时禁用自动充电显示模式
    _autoChargingMode = false;
    
    _mode = mode;
    _color = color;
    _brightness = brightness;
    device_state.led_mode = mode;
    updateLED();
}

void LEDManager::updateChargingStatus() {
    // 只有在自动充电显示模式下才进行状态检测
    if (!_autoChargingMode) {
        return;
    }
    
    bool currentChargingState = bat.isCharging();
    
    // 检测充电状态变化
    if (currentChargingState != _lastChargingState) {
        _lastChargingState = currentChargingState;
        setChargingDisplay(currentChargingState);
        
        Serial.printf("[LEDManager] 充电状态变化: %s\n", 
                     currentChargingState ? "充电中" : "未充电");
    }
}

void LEDManager::setChargingDisplay(bool isCharging) {
    if (isCharging) {
        // 充电中：绿灯呼吸效果，亮度适中
        _mode = LED_BREATH;
        _color = LED_COLOR_GREEN;
        _brightness = 50; // 约20%亮度用于呼吸效果
    } else {
        // 未充电：绿色2%亮度常亮
        _mode = LED_ON;
        _color = LED_COLOR_GREEN;
        _brightness = 5; // 2%亮度
    }
    
    device_state.led_mode = _mode;
    updateLED();
}

void LEDManager::loop() {
    // 更新充电状态显示
    updateChargingStatus();
    
#ifdef PWM_LED_PIN
    pwmLed.loop();
#endif

#ifdef LED_PIN
    led.loop();
#endif
}

void LEDManager::updateLED() {
#ifdef PWM_LED_PIN
    pwmLed.setMode(_mode);
    pwmLed.setColor(_color);
    pwmLed.setBrightness(_brightness);
#endif

#ifdef LED_PIN
    led.setMode(_mode);
#endif
}

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager ledManager;
#endif
