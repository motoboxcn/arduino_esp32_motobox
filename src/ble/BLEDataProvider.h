#ifndef BLE_DATA_PROVIDER_H
#define BLE_DATA_PROVIDER_H

#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE
#include "BLEManager.h"

// 前向声明
class Air780EG;
class IMU;
class BAT;

/**
 * @brief BLE数据提供者类
 * 负责从现有模块获取数据并转换为BLE格式
 */
class BLEDataProvider {
private:
    static const char* TAG;
    
    // 数据源引用
    Air780EG* air780eg;
    IMU* imu;
    BAT* battery;
    
    // 数据缓存
    BLEGPSData cachedGPSData;
    BLEBatteryData cachedBatteryData;
    BLEIMUData cachedIMUData;
    
    // 更新控制
    unsigned long lastGPSUpdate;
    unsigned long lastBatteryUpdate;
    unsigned long lastIMUUpdate;
    
    // 数据有效性标志
    bool gpsDataValid;
    bool batteryDataValid;
    bool imuDataValid;
    
    // 内部方法
    void updateGPSData();
    void updateBatteryData();
    void updateIMUData();
    void convertGPSData();
    void convertBatteryData();
    void convertIMUData();

public:
    BLEDataProvider();
    ~BLEDataProvider();
    
    // 初始化
    void begin();
    void setDataSources(Air780EG* gps, IMU* imu, BAT* bat);
    
    // 数据更新
    void update();
    void forceUpdate();
    
    // 数据获取
    const BLEGPSData& getGPSData() const { return cachedGPSData; }
    const BLEBatteryData& getBatteryData() const { return cachedBatteryData; }
    const BLEIMUData& getIMUData() const { return cachedIMUData; }
    
    // 状态查询
    bool isGPSDataValid() const { return gpsDataValid; }
    bool isBatteryDataValid() const { return batteryDataValid; }
    bool isIMUDataValid() const { return imuDataValid; }
    
    // 调试接口
    void setDebug(bool enable);
    void printDataStatus();
};

// 全局BLE数据提供者实例
extern BLEDataProvider bleDataProvider;

#endif // ENABLE_BLE

#endif // BLE_DATA_PROVIDER_H
