#ifndef LED_DEBUG_H
#define LED_DEBUG_H

#include <Arduino.h>
#include "../config.h"

// LED调试宏定义
#if LED_DEBUG_ENABLED == true
    #define LED_DEBUG_PRINT(x) Serial.print("[LED_DEBUG] "); Serial.print(x)
    #define LED_DEBUG_PRINTLN(x) Serial.print("[LED_DEBUG] "); Serial.println(x)
    #define LED_DEBUG_PRINTF(fmt, ...) Serial.printf("[LED_DEBUG] " fmt, ##__VA_ARGS__)
    
    // 带时间戳的调试输出
    #define LED_DEBUG_TIMESTAMP_PRINTF(fmt, ...) \
        Serial.printf("[LED_DEBUG][%lu] " fmt, millis(), ##__VA_ARGS__)
    
    // 条件调试输出（避免频繁输出）
    #define LED_DEBUG_THROTTLED(interval, fmt, ...) \
        do { \
            static unsigned long lastDebugTime = 0; \
            if (millis() - lastDebugTime > interval) { \
                Serial.printf("[LED_DEBUG][%lu] " fmt, millis(), ##__VA_ARGS__); \
                lastDebugTime = millis(); \
            } \
        } while(0)
    
    // 函数进入/退出调试
    #define LED_DEBUG_ENTER(func) LED_DEBUG_PRINTF("进入函数: %s\n", func)
    #define LED_DEBUG_EXIT(func) LED_DEBUG_PRINTF("退出函数: %s\n", func)
    
    // 状态变化调试
    #define LED_DEBUG_STATE_CHANGE(old_state, new_state, desc) \
        LED_DEBUG_PRINTF("状态变化 [%s]: %s -> %s\n", desc, old_state, new_state)
    
    // 数值调试
    #define LED_DEBUG_VALUE(name, value) LED_DEBUG_PRINTF("%s = %d\n", name, value)
    #define LED_DEBUG_VALUE_F(name, value) LED_DEBUG_PRINTF("%s = %.2f\n", name, value)
    
    // 错误调试
    #define LED_DEBUG_ERROR(fmt, ...) \
        Serial.printf("[LED_ERROR][%lu] " fmt, millis(), ##__VA_ARGS__)
    
    // 警告调试
    #define LED_DEBUG_WARNING(fmt, ...) \
        Serial.printf("[LED_WARNING][%lu] " fmt, millis(), ##__VA_ARGS__)
    
#else
    // 当LED_DEBUG_ENABLED未定义时，所有调试宏都为空
    #define LED_DEBUG_PRINT(x)
    #define LED_DEBUG_PRINTLN(x)
    #define LED_DEBUG_PRINTF(fmt, ...)
    #define LED_DEBUG_TIMESTAMP_PRINTF(fmt, ...)
    #define LED_DEBUG_THROTTLED(interval, fmt, ...)
    #define LED_DEBUG_ENTER(func)
    #define LED_DEBUG_EXIT(func)
    #define LED_DEBUG_STATE_CHANGE(old_state, new_state, desc)
    #define LED_DEBUG_VALUE(name, value)
    #define LED_DEBUG_VALUE_F(name, value)
    #define LED_DEBUG_ERROR(fmt, ...)
    #define LED_DEBUG_WARNING(fmt, ...)
#endif

// LED模式转换为字符串的辅助函数
inline const char* ledModeToString(int mode) {
    switch (mode) {
        case 0: return "LED_OFF";
        case 1: return "LED_ON";
        case 2: return "LED_BLINK_SINGLE";
        case 3: return "LED_BLINK_DUAL";
        case 4: return "LED_BLINK_FAST";
        case 5: return "LED_BLINK_5_SECONDS";
        case 6: return "LED_BLINK_SLOW";
        case 7: return "LED_BREATH";
        default: return "UNKNOWN";
    }
}

// LED颜色转换为字符串的辅助函数
inline const char* ledColorToString(int color) {
    switch (color) {
        case 0: return "NONE";
        case 1: return "WHITE";
        case 2: return "BLUE";
        case 3: return "YELLOW";
        case 4: return "PURPLE";
        case 5: return "GREEN";
        case 6: return "RED";
        default: return "UNKNOWN";
    }
}

#endif // LED_DEBUG_H
