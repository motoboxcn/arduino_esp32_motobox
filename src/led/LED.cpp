#include "LED.h"
#include "LEDDebug.h"  // 新增：LED调试支持

#ifdef LED_PIN
LED led(LED_PIN);
#endif

LED::LED(uint8_t pin) : _pin(pin), _mode(LED_OFF), _lastToggle(0), _state(false), _blinkCount(0), _blinkTimes(0)
{
    LED_DEBUG_PRINTF("LED构造函数调用，引脚: %d\n", pin);
}

void LED::begin()
{
    LED_DEBUG_ENTER("LED::begin");
    LED_DEBUG_PRINTF("初始化普通LED，引脚: %d\n", _pin);
    
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH); // 初始状态设为HIGH（LED灭）
    
    LED_DEBUG_PRINTF("普通LED初始化完成，引脚: %d\n", _pin);
    Serial.printf("[LED] 引脚: %d\n", _pin);
    
    LED_DEBUG_EXIT("LED::begin");
}

void LED::setMode(LEDMode mode)
{
    if (_mode != mode)
    {
        LED_DEBUG_STATE_CHANGE(ledModeToString(_mode), ledModeToString(mode), "普通LED模式");
        
        _mode = mode;
        _lastToggle = 0; // 切换模式时重置计时器
        _blinkCount = 0;
        _state = false;
        digitalWrite(_pin, HIGH); // 切换模式时设为HIGH（LED灭）
        
        LED_DEBUG_PRINTF("普通LED模式设置完成: %s\n", ledModeToString(mode));
    }
}

void LED::loop()
{
    unsigned long currentMillis = millis();
    bool oldState = _state;

    switch (_mode)
    {
    case LED_ON:
        _state = true;
        break;

    case LED_OFF:
        _state = false;
        break;

    case LED_BLINK_SINGLE:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _state = true;
            _lastToggle = currentMillis;
        }
        else if (_state && currentMillis - _lastToggle >= BLINK_INTERVAL)
        {
            _state = false;
        }
        break;

    case LED_BLINK_DUAL:
        if (currentMillis - _lastToggle >= PATTERN_INTERVAL)
        {
            _blinkCount = 0;
            _state = true;
            _lastToggle = currentMillis;
        }
        else if (currentMillis - _lastToggle < 600) 
        {
            unsigned long phase = (currentMillis - _lastToggle) / 150; 
            if (phase != _blinkCount)
            {
                _blinkCount = phase;
                _state = (_blinkCount % 2 == 0);
            }
        }
        else
        {
            _state = false;
        }
        break;

    case LED_BLINK_FAST:
        if (currentMillis - _lastToggle >= 100)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    case LED_BLINK_5_SECONDS:
        if (currentMillis - _lastToggle >= 5000)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    case LED_BLINK_SLOW:
        if (currentMillis - _lastToggle > 500)
        {
            _state = !_state;
            _lastToggle = currentMillis;
        }
        break;

    default:
        _state = false;
        break;
    }

    // 记录状态变化
    if (oldState != _state) {
        LED_DEBUG_PRINTF("普通LED状态变化: %s -> %s (模式: %s)\n", 
                        oldState ? "亮" : "灭", _state ? "亮" : "灭", ledModeToString(_mode));
    }

    // 修改LED控制逻辑：低电平触发（共阳极LED）
    digitalWrite(_pin, _state ? LOW : HIGH);
    
    // 定期输出状态（仅在调试模式下）
    LED_DEBUG_THROTTLED(10000, "普通LED循环状态 - 模式: %s, 状态: %s\n",
                       ledModeToString(_mode), _state ? "亮" : "灭");
}

// 初始化闪烁,最原始的闪烁方式，配合 delay
void LED::initBlink(uint8_t times) {
    LED_DEBUG_PRINTF("开始初始化闪烁，次数: %d\n", times);
    
    for (int i = 0; i < times; i++) {
        LED_DEBUG_PRINTF("初始化闪烁第 %d 次\n", i + 1);
        digitalWrite(_pin, LOW);  // LED亮
        delay(INIT_BLINK_INTERVAL);
        digitalWrite(_pin, HIGH); // LED灭
        delay(INIT_BLINK_INTERVAL);
    }
    
    LED_DEBUG_PRINTLN("初始化闪烁完成");
    Serial.println("初始化闪烁完成");
}

const char* modeToString(LEDMode mode) {
    switch (mode) {
        case LED_OFF: return "关闭";
        case LED_ON: return "常亮";
        case LED_BLINK_SINGLE: return "单闪";
        case LED_BLINK_DUAL: return "双闪";
        case LED_BLINK_FAST: return "快闪";
        case LED_BLINK_5_SECONDS: return "5秒闪烁";
        case LED_BLINK_SLOW: return "慢闪";
        default: return "未知";
    }
}