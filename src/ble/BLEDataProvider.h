#ifndef BLE_DATA_PROVIDER_SIMPLE_H
#define BLE_DATA_PROVIDER_SIMPLE_H

#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE

// 前向声明
struct device_state_t;

/**
 * @brief 简化的BLE数据提供者类
 * 直接使用device_state提供统一的遥测数据
 */
class BLEDataProvider {
private:
    static const char* TAG;
    
    // 数据源
    const device_state_t* deviceState;
    
    // 更新控制
    unsigned long lastUpdateTime;
    bool dataValid;

public:
    BLEDataProvider();
    ~BLEDataProvider();
    
    // 初始化和控制
    void begin();
    void update();
    
    // 数据源设置
    void setDeviceState(const device_state_t* state);
    
    // 数据获取
    const device_state_t* getDeviceState() const { return deviceState; }
    bool isDataValid() const { return dataValid; }
    
    // 调试
    void setDebug(bool enable);
    void printDataStatus();
};

// 全局BLE数据提供者实例
extern BLEDataProvider bleDataProvider;

#endif // ENABLE_BLE

#endif // BLE_DATA_PROVIDER_SIMPLE_H
