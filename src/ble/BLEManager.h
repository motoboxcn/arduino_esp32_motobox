#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include <Arduino.h>
#include "config.h"

#ifdef ENABLE_BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// BLE数据结构定义
typedef struct {
    double latitude;      // 纬度
    double longitude;     // 经度
    float altitude;       // 海拔
    float speed;          // 速度 (km/h)
    float course;         // 航向 (度)
    int satellites;       // 卫星数量
    bool valid;           // 数据有效性
} BLEGPSData;

typedef struct {
    int voltage;          // 电池电压 (mV)
    int percentage;       // 电池百分比 (0-100)
    bool is_charging;     // 充电状态
    bool external_power;  // 外部电源状态
} BLEBatteryData;

typedef struct {
    float pitch;          // 俯仰角 (度)
    float roll;           // 横滚角 (度)
    float yaw;            // 偏航角 (度)
    float accel_x;        // X轴加速度 (m/s²)
    float accel_y;        // Y轴加速度 (m/s²)
    float accel_z;        // Z轴加速度 (m/s²)
    float gyro_x;         // X轴角速度 (rad/s)
    float gyro_y;         // Y轴角速度 (rad/s)
    float gyro_z;         // Z轴角速度 (rad/s)
    bool valid;           // 数据有效性
} BLEIMUData;

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
 * @brief BLE管理器类
 * 负责BLE服务器初始化、数据广播和连接管理
 */
class BLEManager {
private:
    static const char* TAG;
    
    BLEServer* pServer;
    BLEService* pService;
    
    // BLE特征值
    BLECharacteristic* pGPSCharacteristic;
    BLECharacteristic* pBatteryCharacteristic;
    BLECharacteristic* pIMUCharacteristic;
    
    // 回调对象
    MotoBoxBLEServerCallbacks* serverCallbacks;
    MotoBoxBLECharacteristicCallbacks* charCallbacks;
    
    // 状态管理
    bool isInitialized;
    unsigned long lastUpdateTime;
    
    // 设备名称
    String deviceName;
    
    // 数据缓存
    BLEGPSData lastGPSData;
    BLEBatteryData lastBatteryData;
    BLEIMUData lastIMUData;
    
    // 内部方法
    void createService();
    void createCharacteristics();
    String gpsDataToJSON(const BLEGPSData& data);
    String batteryDataToJSON(const BLEBatteryData& data);
    String imuDataToJSON(const BLEIMUData& data);

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
    
    // 数据更新接口
    void updateGPSData(const BLEGPSData& data);
    void updateBatteryData(const BLEBatteryData& data);
    void updateIMUData(const BLEIMUData& data);
    
    // 状态查询
    bool isReady() const { return isInitialized; }
    bool isAdvertisingActive() const { return isAdvertising; }
    
    // 调试接口
    void setDebug(bool enable);
    void printStatus();
};

// 全局BLE管理器实例
extern BLEManager bleManager;

#endif // ENABLE_BLE

#endif // BLE_MANAGER_H
