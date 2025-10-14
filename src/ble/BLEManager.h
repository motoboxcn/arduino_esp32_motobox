#ifndef BLE_MANAGER_SIMPLE_H
#define BLE_MANAGER_SIMPLE_H

#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// 包含BLE类型定义
#include "BLETypes.h"

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
 * @brief 简化的BLE管理器类
 * 使用单一特征值传输完整的遥测数据
 */
class BLEManager {
private:
    static const char* TAG;
    
    BLEServer* pServer;
    BLEService* pService;
    
    // 单一遥测特征值
    BLECharacteristic* pTelemetryCharacteristic;
    
    // 回调对象
    MotoBoxBLEServerCallbacks* serverCallbacks;
    MotoBoxBLECharacteristicCallbacks* charCallbacks;
    
    // 状态管理
    bool isInitialized;
    unsigned long lastUpdateTime;
    
    // 设备名称
    String deviceName;
    
    // 数据缓存
    String lastTelemetryData;
    
    // 内部方法
    void createService();
    void createCharacteristics();
    String telemetryDataToJSON(const ble_device_state_t& deviceState);

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
    
    // 数据更新
    void updateTelemetryData(const ble_device_state_t& deviceState);
    
    // 调试和状态
    void setDebug(bool enable);
    void printStatus();
};

// 全局BLE管理器实例
extern BLEManager bleManager;

#endif // ENABLE_BLE

#endif // BLE_MANAGER_SIMPLE_H
