#include "PWMLED.h"
#include "LEDDebug.h"  // 新增：LED调试支持

#ifdef PWM_LED_PIN
PWMLED pwmLed(PWM_LED_PIN);

// 定义颜色映射表 - 需要与 LEDColor 枚举对应
const PWMLED::RGB PWMLED::COLOR_MAP[] = {
    {0, 0, 0},       // LED_COLOR_NONE
    {255, 255, 255}, // LED_COLOR_WHITE (GPS)
    {0, 0, 255},     // LED_COLOR_BLUE (WiFi)
    {255, 255, 0},   // LED_COLOR_YELLOW (4G)
    {255, 0, 255},   // LED_COLOR_PURPLE (IMU)
    {0, 255, 0},     // LED_COLOR_GREEN (System OK)
    {255, 0, 0}      // LED_COLOR_RED (System Error)
};

const uint8_t PWMLED::RAINBOW_COLORS[7][3] = {
    {255, 0, 0},   // 红
    {255, 127, 0}, // 橙
    {255, 255, 0}, // 黄
    {0, 255, 0},   // 绿
    {0, 0, 255},   // 蓝
    {75, 0, 130},  // 靛
    {148, 0, 211}  // 紫
};

PWMLED::PWMLED(uint8_t pin) :
    _pin(pin),
    _mode(LED_OFF),
    _currentColor(LED_COLOR_NONE),
    _brightness(DEFAULT_BRIGHTNESS),
    _lastUpdate(0),
    _blinkState(false),
    _breathValue(0),
    _breathIncreasing(true)
{
}

void PWMLED::begin() {
    LED_DEBUG_ENTER("PWMLED::begin");
    LED_DEBUG_PRINTF("初始化PWM LED，引脚: %d\n", _pin);
    
    FastLED.addLeds<WS2812B, PWM_LED_PIN, GRB>(_leds, NUM_LEDS);
    
    LED_DEBUG_PRINTF("PWM LED 初始化完成，引脚: %d\n", _pin);
    Serial.printf("[PWMLED] 初始化完成，引脚: %d\n", _pin);
    
    LED_DEBUG_EXIT("PWMLED::begin");
}

void PWMLED::deinit() {
    LED_DEBUG_ENTER("PWMLED::deinit");
    LED_DEBUG_PRINTF("反初始化PWM LED，引脚: %d\n", _pin);
    
    // 关闭所有LED
    setBrightness(0);
    _mode = LED_OFF;
    
    // 清除FastLED配置
    FastLED.clear();
    FastLED.show();
    
    LED_DEBUG_PRINTF("PWM LED 反初始化完成，引脚: %d\n", _pin);
    Serial.printf("[PWMLED] 反初始化完成，引脚: %d\n", _pin);
    
    LED_DEBUG_EXIT("PWMLED::deinit");
}

void PWMLED::initRainbow() {
    _rainbowIndex = 0;
    _lastUpdate = millis();
}

void PWMLED::loop() {
    unsigned long currentMillis = millis();
    
    switch (_mode) {
        case LED_BREATH:
            if (currentMillis - _lastUpdate >= BREATH_INTERVAL) {
                updateBreathEffect();
                _lastUpdate = currentMillis;
            }
            break;
            
        case LED_BLINK_SLOW:
            if (currentMillis - _lastUpdate >= BLINK_SLOW_INTERVAL) {
                updateBlinkEffect();
                _lastUpdate = currentMillis;
            }
            break;
            
        case LED_BLINK_FAST:
            if (currentMillis - _lastUpdate >= BLINK_FAST_INTERVAL) {
                updateBlinkEffect();
                _lastUpdate = currentMillis;
            }
            break;
        case LED_ON:
            break;
        case LED_BLINK_5_SECONDS:
            
        case LED_BLINK_SINGLE:
            if (currentMillis - _lastUpdate >= BLINK_SLOW_INTERVAL) {  // 使用 BLINK_SLOW_INTERVAL 作为单闪间隔
                updateBlinkEffect();
                _lastUpdate = currentMillis;
            }
            break;
    }
}

void PWMLED::setMode(LEDMode mode) {
    if (_mode != mode) {
        LED_DEBUG_STATE_CHANGE(ledModeToString(_mode), ledModeToString(mode), "PWM LED模式");
        
        _mode = mode;
        _lastUpdate = 0;
        _blinkState = false;
        
        // 根据模式初始化呼吸效果
        if (mode == LED_BREATH) {
            _breathValue = _brightness / 4; // 从25%亮度开始
            _breathIncreasing = true;
            LED_DEBUG_PRINTF("设置呼吸模式，初始亮度: %d, 目标亮度: %d\n", _breathValue, _brightness);
            Serial.printf("[PWMLED] 设置呼吸模式，初始亮度: %d, 目标亮度: %d\n", _breathValue, _brightness);
        } else {
            _breathValue = 0;
            _breathIncreasing = true;
        }
        
        LED_DEBUG_PRINTF("PWM LED模式变更完成: %s, 颜色: %s, 亮度: %d\n", 
                        ledModeToString(mode), ledColorToString(_currentColor), _brightness);
        Serial.printf("[PWMLED] 模式变更: %d, 颜色: %d, 亮度: %d\n", mode, _currentColor, _brightness);
        updateColor();
    }
}

void PWMLED::setColor(LEDColor color) {
    if (_currentColor != color) {
        LED_DEBUG_STATE_CHANGE(ledColorToString(_currentColor), ledColorToString(color), "PWM LED颜色");
        _currentColor = color;
        updateColor();
    }
}

void PWMLED::setBrightness(uint8_t brightness) {
    uint8_t newBrightness = min(brightness, TOP_LEVEL);
    if (_brightness != newBrightness) {
        LED_DEBUG_PRINTF("PWM LED亮度变化: %d -> %d\n", _brightness, newBrightness);
        _brightness = newBrightness;
        updateColor();
    }
}

void PWMLED::updateBreathEffect() {
    if (_breathIncreasing) {
        _breathValue += BREATH_STEP;
        if (_breathValue >= _brightness) {
            _breathValue = _brightness;
            _breathIncreasing = false;
            LED_DEBUG_PRINTF("呼吸效果到达最大值: %d\n", _breathValue);
        }
    } else {
        if (_breathValue <= BREATH_STEP) {
            _breathValue = 0;
            _breathIncreasing = true;
            LED_DEBUG_PRINTF("呼吸效果到达最小值: %d\n", _breathValue);
        } else {
            _breathValue -= BREATH_STEP;
        }
    }
    
    LED_DEBUG_THROTTLED(3000, "呼吸效果更新: 当前值=%d, 目标=%d, 递增=%s\n", 
                       _breathValue, _brightness, _breathIncreasing ? "是" : "否");
    
    showLED();
}

void PWMLED::updateBlinkEffect() {
    _blinkState = !_blinkState;
    LED_DEBUG_PRINTF("闪烁效果更新: 状态=%s\n", _blinkState ? "亮" : "灭");
    showLED();
}

void PWMLED::updateColor() {
    // 颜色设置现在在showLED()中处理，这里只需要触发显示更新
    showLED();
}

void PWMLED::showLED() {
    uint8_t actualBrightness = _brightness;
    
    // 先设置基础颜色
    const RGB& rgb = COLOR_MAP[_currentColor];
    _leds[0].setRGB(rgb.r, rgb.g, rgb.b);
    
    // 根据模式调整亮度
    switch (_mode) {
        case LED_OFF:
            actualBrightness = 0;
            break;
        case LED_BREATH:
            actualBrightness = _breathValue;
            break;
        case LED_BLINK_SLOW:
        case LED_BLINK_FAST:
        case LED_BLINK_SINGLE:
            actualBrightness = _blinkState ? _brightness : 0;
            break;
        case LED_ON:
            // 使用设置的亮度
            break;
    }
    
    LED_DEBUG_THROTTLED(5000, "显示LED: 模式=%s, 颜色=%s, 设定亮度=%d, 实际亮度=%d, RGB=(%d,%d,%d)\n",
                       ledModeToString(_mode), ledColorToString(_currentColor), 
                       _brightness, actualBrightness, rgb.r, rgb.g, rgb.b);
    
    // 应用亮度缩放
    _leds[0].nscale8(actualBrightness);
    FastLED.show();
}

void PWMLED::updateRainbow() {
    if (millis() - _lastUpdate >= 100) { // 每100ms更新一次颜色
        _leds[0].setRGB(
            RAINBOW_COLORS[_rainbowIndex][0],
            RAINBOW_COLORS[_rainbowIndex][1],
            RAINBOW_COLORS[_rainbowIndex][2]
        );
        _leds[0].nscale8(50); // 50%亮度
        FastLED.show();
        
        _rainbowIndex = (_rainbowIndex + 1) % 7;
        _lastUpdate = millis();
    }
}
#endif
