#ifndef BAT_H
#define BAT_H

#include <Arduino.h>
#include "device.h"

// 电池管理类
class BAT {
public:
    BAT(int adc_pin, int charging_pin);
    
    void begin();
    void loop();
    void print_voltage();
    
    // 获取电池信息
    int getVoltage() const { return stable_voltage; }
    int getPercentage() const;  // 外部实现
    bool isCharging() const { return _is_charging; }
    
    // 调试控制
    void setDebug(bool debug) { _debug = debug; }
    
private:
    // 引脚配置
    int pin;                    // ADC引脚
    int charging_pin;           // 充电状态检测引脚
    
    // 充电状态管理
    bool _is_charging;          // 当前充电状态
    bool _last_stable_charging; // 上一次稳定的充电状态
    int _charging_state_count;  // 充电状态变化计数器
    static const int CHARGING_DEBOUNCE_COUNT = 5; // 防抖计数阈值
    
    // 电压相关
    int voltage;                // 当前电压 (mV)
    int ema_voltage;            // EMA滤波后的电压
    int stable_voltage;         // 稳定输出电压
    int last_output_voltage;    // 上次输出电压
    
    // 滤波参数
    static const int FILTER_WINDOW_SIZE = 8;  // 滑动窗口大小
    static constexpr float EMA_ALPHA = 0.3f;  // EMA系数
    static const int MAX_VOLTAGE_JUMP = 100;  // 最大电压跳变 (mV)
    static const int OUTPUT_DIVIDER = 4;      // 输出分频
    
    // 校准参数
    int min_voltage;            // 最小电压 (mV)
    int max_voltage;            // 最大电压 (mV)
    int observed_min;           // 观察到的最大电压
    int observed_max;           // 观察到的最大电压
    static const int VOLTAGE_MIN_LIMIT = 2000;  // 校准电压下限 (mV)
    static const int VOLTAGE_MAX_LIMIT = 5000;  // 校准电压上限 (mV)
    static const int CALIBRATION_INTERVAL = 300000; // 校准间隔 (5分钟)
    static const int STABLE_COUNT = 100;       // 稳定计数阈值
    
    // 内部状态
    int voltage_buffer[FILTER_WINDOW_SIZE];  // 电压缓冲区
    int buffer_index;                        // 缓冲区索引
    int output_counter;                      // 输出计数器
    unsigned long last_calibration;          // 上次校准时间
    int stable_count;                        // 稳定计数
    
    // 调试控制
    bool _debug;
    
    // 私有方法
    void loadCalibration();
    void saveCalibration();
    void updateCalibration();
    int calculatePercentage(int voltage) const;  // 添加const修饰符
    
    // 充电状态防抖
    bool updateChargingState();
    
    // 常量定义
    static const int VOLTAGE_LEVELS[];
    static const int PERCENT_LEVELS[];
    static const int LEVEL_COUNT;
    
    // 调试输出
    void debugPrint(const String& message);
};

extern BAT bat;

#endif // BAT_H
