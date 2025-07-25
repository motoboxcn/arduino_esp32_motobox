#include "PWMLED.h"

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
    FastLED.addLeds<WS2812B, PWM_LED_PIN, GRB>(_leds, NUM_LEDS);
    Serial.printf("[PWMLED] 初始化完成，引脚: %d\n", _pin);
}

void PWMLED::deinit() {
    // 关闭所有LED
    setBrightness(0);
    _mode = LED_OFF;
    
    // 清除FastLED配置
    FastLED.clear();
    FastLED.show();
    
    Serial.printf("[PWMLED] 反初始化完成，引脚: %d\n", _pin);
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
        _mode = mode;
        _lastUpdate = 0;
        _blinkState = false;
        
        // 根据模式初始化呼吸效果
        if (mode == LED_BREATH) {
            _breathValue = _brightness / 4; // 从25%亮度开始
            _breathIncreasing = true;
            Serial.printf("[PWMLED] 设置呼吸模式，初始亮度: %d, 目标亮度: %d\n", _breathValue, _brightness);
        } else {
            _breathValue = 0;
            _breathIncreasing = true;
        }
        
        Serial.printf("[PWMLED] 模式变更: %d, 颜色: %d, 亮度: %d\n", mode, _currentColor, _brightness);
        updateColor();
    }
}

void PWMLED::setColor(LEDColor color) {
    if (_currentColor != color) {
        _currentColor = color;
        updateColor();
    }
}

void PWMLED::setBrightness(uint8_t brightness) {
    _brightness = min(brightness, MAX_BRIGHTNESS);
    updateColor();
}

void PWMLED::updateBreathEffect() {
    static unsigned long lastDebugTime = 0;
    bool debugOutput = (millis() - lastDebugTime > 2000); // 每2秒输出一次调试信息
    
    if (_breathIncreasing) {
        _breathValue += BREATH_STEP;
        if (_breathValue >= _brightness) {
            _breathValue = _brightness;
            _breathIncreasing = false;
            if (debugOutput) {
                Serial.printf("[PWMLED] 呼吸效果到达最大值: %d\n", _breathValue);
            }
        }
    } else {
        if (_breathValue <= BREATH_STEP) {
            _breathValue = 0;
            _breathIncreasing = true;
            if (debugOutput) {
                Serial.printf("[PWMLED] 呼吸效果到达最小值: %d\n", _breathValue);
            }
        } else {
            _breathValue -= BREATH_STEP;
        }
    }
    
    if (debugOutput) {
        Serial.printf("[PWMLED] 呼吸效果更新: 当前值=%d, 目标=%d, 递增=%s\n", 
                     _breathValue, _brightness, _breathIncreasing ? "是" : "否");
        lastDebugTime = millis();
    }
    
    showLED();
}

void PWMLED::updateBlinkEffect() {
    _blinkState = !_blinkState;
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
