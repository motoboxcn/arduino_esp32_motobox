#ifndef BLE_MANAGER_MODULAR_H
#define BLE_MANAGER_MODULAR_H

#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// 包含设备状态定义
#include "device.h"

/**
 * @brief MotoBox BLE服务器回调类
 * 处理客户端连接和断开事件
 */
class MotoBoxBLEServerCallbacks : public BLEServerCallbacks {
public:
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;
    
    // 友元类，允许访问BLEManager的私有成员
    friend class BLEManager;
};

/**
 * @brief MotoBox BLE特征值回调类
 * 处理客户端订阅和取消订阅事件
 */
class MotoBoxBLECharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    void onRead(BLECharacteristic* pCharacteristic) override;
    void onWrite(BLECharacteristic* pCharacteristic) override;
};

/**
 * @brief 模块化BLE管理器类
 * 使用四个独立特征值分别传输GPS、IMU、罗盘和系统数据
 */
class BLEManager {
private:
    static const char* TAG;
    
    BLEServer* pServer;
    BLEService* pService;
    
    // 模块化特征值
    BLECharacteristic* pGPSCharacteristic;      // GPS位置数据特征值
    BLECharacteristic* pIMUCharacteristic;      // IMU传感器数据特征值
    BLECharacteristic* pCompassCharacteristic;  // 罗盘数据特征值
    BLECharacteristic* pSystemCharacteristic;   // 系统状态数据特征值
    
    // 回调对象
    MotoBoxBLEServerCallbacks* serverCallbacks;
    MotoBoxBLECharacteristicCallbacks* charCallbacks;
    
    // 状态管理
    bool isInitialized;
    
    // 更新时间跟踪
    unsigned long lastGPSUpdateTime;
    unsigned long lastIMUUpdateTime;
    unsigned long lastCompassUpdateTime;
    unsigned long lastSystemUpdateTime;
    
    // 设备名称
    String deviceName;
    
    // 数据缓存（避免重复发送相同数据）
    String lastGPSData;
    String lastIMUData;
    String lastCompassData;
    String lastSystemData;
    
    // 内部方法
    void createService();
    void createCharacteristics();

public:
    BLEManager();
    ~BLEManager();
    
    // 状态管理（供回调类访问）
    bool isConnected;
    bool isAdvertising;
    
    // 初始化和控制
    bool begin();
    bool begin(const String& deviceId);
    void end();
    void update();
    
    // 连接管理
    bool isClientConnected() const { return isConnected; }
    int getConnectedClients() const;
    
    // 广播控制（供回调类访问）
    void startAdvertising();
    void stopAdvertising();
    
    // 模块化数据更新方法
    void updateGPSData();
    void updateIMUData();
    void updateCompassData();
    void updateSystemData();
    
    // 统一数据更新方法（从完整设备状态提取各模块数据）
    void updateAllData();
    
    // 调试和状态
    void setDebug(bool enable);
    void printStatus();
};

// 全局BLE管理器实例
extern BLEManager bleManager;

#endif // ENABLE_BLE

#endif // BLE_MANAGER_MODULAR_H
