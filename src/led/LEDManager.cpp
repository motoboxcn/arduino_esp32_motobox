#include "LEDManager.h"
#include "LEDDebug.h"  // 新增：LED调试支持
#include "device.h"
#include "led/LED.h"
#include "led/PWMLED.h"
#include "bat/BAT.h"  // 新增：包含电池管理

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager::LEDManager() : _mode(LED_OFF), _color(LED_COLOR_GREEN), _brightness(10), 
                          _lastChargingState(false), _autoChargingMode(true) {
    LED_DEBUG_PRINTLN("LEDManager 构造函数调用");
}
#endif


void LEDManager::begin() {
    LED_DEBUG_ENTER("LEDManager::begin");
    
#ifdef PWM_LED_PIN
    LED_DEBUG_PRINTLN("初始化 PWM LED");
    pwmLed.begin();
     // PWM LED 彩虹色初始化
    pwmLed.initRainbow();
     // PWM LED 初始设置为绿色2%亮度常亮（未充电状态）
    pwmLed.setMode(LED_ON);
    pwmLed.setColor(LED_COLOR_GREEN);
    pwmLed.setBrightness(5); // 2% of 255 ≈ 5
    
    // 启用自动充电显示模式
    _autoChargingMode = true;
    LED_DEBUG_PRINTF("PWM LED 初始化完成 - 模式: %s, 颜色: %s, 亮度: %d\n", 
                     ledModeToString(LED_ON), ledColorToString(LED_COLOR_GREEN), 5);
#endif

#ifdef LED_PIN
    LED_DEBUG_PRINTLN("初始化普通 LED");
    led.begin();
     // 普通 LED 闪烁两次
    led.initBlink(2);
    // 普通 LED 设置为常亮
    led.setMode(LED_ON);
    LED_DEBUG_PRINTF("普通 LED 初始化完成 - 模式: %s\n", ledModeToString(LED_ON));
#endif

    LED_DEBUG_EXIT("LEDManager::begin");
}


void LEDManager::setLEDState(LEDMode mode, LEDColor color, uint8_t brightness) {
    LED_DEBUG_ENTER("LEDManager::setLEDState");
    
    // 记录状态变化
    if (_mode != mode) {
        LED_DEBUG_STATE_CHANGE(ledModeToString(_mode), ledModeToString(mode), "LED模式");
    }
    if (_color != color) {
        LED_DEBUG_STATE_CHANGE(ledColorToString(_color), ledColorToString(color), "LED颜色");
    }
    if (_brightness != brightness) {
        LED_DEBUG_PRINTF("亮度变化: %d -> %d\n", _brightness, brightness);
    }
    
    // 如果手动设置LED状态，则暂时禁用自动充电显示模式
    if (_autoChargingMode) {
        _autoChargingMode = false;
        LED_DEBUG_PRINTLN("手动设置LED状态，禁用自动充电显示模式");
    }
    
    _mode = mode;
    _color = color;
    _brightness = brightness;
    device_state.led_mode = mode;
    
    LED_DEBUG_PRINTF("LED状态设置完成 - 模式: %s, 颜色: %s, 亮度: %d\n", 
                     ledModeToString(mode), ledColorToString(color), brightness);
    
    updateLED();
    LED_DEBUG_EXIT("LEDManager::setLEDState");
}

void LEDManager::updateChargingStatus() {
    // 获取当前充电状态
    bool currentChargingState = device_state.telemetry.system.is_charging;
    
    // 检测充电状态变化
    if (currentChargingState != _lastChargingState) {
        _lastChargingState = currentChargingState;
        
        LED_DEBUG_PRINTF("充电状态变化: %s -> %s\n", 
                        _lastChargingState ? "充电中" : "未充电",
                        currentChargingState ? "充电中" : "未充电");
        
        // 只在状态真正变化时才更新LED显示
        setChargingDisplay(currentChargingState);
        
        Serial.printf("[LEDManager] 充电状态变化: %s\n", 
                     currentChargingState ? "充电中" : "未充电");
    }
    
    // 定期调试信息输出（降低频率，避免日志过多）
    LED_DEBUG_THROTTLED(10000, "状态监控 - 充电: %s, 自动模式: %s, LED模式: %s, 颜色: %s, 亮度: %d\n",
                       currentChargingState ? "是" : "否",
                       _autoChargingMode ? "启用" : "禁用",
                       ledModeToString(_mode),
                       ledColorToString(_color),
                       _brightness);
}

void LEDManager::setChargingDisplay(bool isCharging) {
    LED_DEBUG_ENTER("LEDManager::setChargingDisplay");
    LED_DEBUG_PRINTF("设置充电显示状态: %s\n", isCharging ? "充电中" : "未充电");
    
    LEDMode oldMode = _mode;
    LEDColor oldColor = _color;
    uint8_t oldBrightness = _brightness;
    
    // 计算新的LED状态
    LEDMode newMode;
    LEDColor newColor;
    uint8_t newBrightness;
    
    if (isCharging) {
        // 充电中：绿灯呼吸效果，亮度适中
        newMode = LED_BREATH;
        newColor = LED_COLOR_GREEN;
        newBrightness = 50; // 约20%亮度用于呼吸效果
        LED_DEBUG_PRINTLN("充电中 -> 设置为绿灯呼吸效果");
    } else {
        // 未充电：绿色2%亮度常亮
        newMode = LED_ON;
        newColor = LED_COLOR_GREEN;
        newBrightness = 5; // 2%亮度
        LED_DEBUG_PRINTLN("未充电 -> 设置为绿色低亮度常亮");
    }
    
    // 只有当状态真正需要变化时才更新
    bool needsUpdate = false;
    
    if (oldMode != newMode) {
        _mode = newMode;
        LED_DEBUG_STATE_CHANGE(ledModeToString(oldMode), ledModeToString(newMode), "充电显示模式");
        needsUpdate = true;
    }
    
    if (oldColor != newColor) {
        _color = newColor;
        LED_DEBUG_STATE_CHANGE(ledColorToString(oldColor), ledColorToString(newColor), "充电显示颜色");
        needsUpdate = true;
    }
    
    if (oldBrightness != newBrightness) {
        _brightness = newBrightness;
        LED_DEBUG_PRINTF("充电显示亮度变化: %d -> %d\n", oldBrightness, newBrightness);
        needsUpdate = true;
    }
    
    // 只在需要时才更新LED硬件
    if (needsUpdate) {
        device_state.led_mode = _mode;
        updateLED();
    } else {
        LED_DEBUG_PRINTLN("LED状态无需更新，跳过硬件更新");
    }
    
    LED_DEBUG_EXIT("LEDManager::setChargingDisplay");
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

    // 定期输出LED状态（仅在调试模式下）
    LED_DEBUG_THROTTLED(15000, "LED循环状态 - 模式: %s, 颜色: %s, 亮度: %d, 自动充电模式: %s\n",
                       ledModeToString(_mode), ledColorToString(_color), _brightness,
                       _autoChargingMode ? "启用" : "禁用");
}

void LEDManager::updateLED() {
    LED_DEBUG_PRINTF("更新LED硬件 - 模式: %s, 颜色: %s, 亮度: %d\n", 
                     ledModeToString(_mode), ledColorToString(_color), _brightness);
    
#ifdef PWM_LED_PIN
    LED_DEBUG_PRINTLN("更新PWM LED硬件");
    pwmLed.setMode(_mode);
    pwmLed.setColor(_color);
    pwmLed.setBrightness(_brightness);
#endif

#ifdef LED_PIN
    LED_DEBUG_PRINTLN("更新普通LED硬件");
    led.setMode(_mode);
#endif
}

#if defined(LED_PIN) || defined(PWM_LED_PIN)
LEDManager ledManager;
#endif
