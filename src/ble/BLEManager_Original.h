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

// 融合定位数据结构（包含详细debug信息）
typedef struct {
    // 融合位置
    double lat, lng;           // 融合后的经纬度
    float altitude;            // 海拔 (m)
    
    // 运动状态
    float speed;               // 速度 (m/s)
    float heading;             // 航向角 (度, 0-360)
    
    // 姿态角（Madgwick原始输出）
    float raw_roll, raw_pitch, raw_yaw;
    
    // 姿态角（卡尔曼滤波后）
    float kalman_roll, kalman_pitch, kalman_yaw;
    
    // 摩托车特定数据
    float lean_angle;          // 倾斜角 (度)
    float lean_rate;           // 倾斜角速度 (度/s)
    float forward_accel;       // 前进加速度 (m/s²)
    float lateral_accel;       // 侧向加速度 (m/s²)
    
    // 位置积分数据（debug用）
    float pos_x, pos_y, pos_z; // 相对位置积分 (m)
    float vel_x, vel_y, vel_z; // 速度积分 (m/s)
    
    // 状态标志
    uint8_t source;            // 定位来源 (0=GPS, 1=GPS+IMU, 2=惯导)
    bool kalman_enabled;       // 卡尔曼滤波是否启用
    bool valid;                // 数据有效性
    uint32_t timestamp;        // 时间戳 (ms)
} BLEFusionData;

// 系统状态数据结构
typedef struct {
    // 运行状态
    uint8_t mode;              // 运行模式 (0=正常, 1=调试, 2=校准)
    uint32_t uptime;           // 运行时间 (秒)
    uint32_t loop_count;       // 主循环计数
    
    // 模块状态
    uint8_t gps_status;        // GPS状态 (0=无效, 1=有效, 2=丢失)
    uint8_t imu_status;        // IMU状态 (0=无效, 1=有效, 2=校准中)
    uint8_t battery_status;    // 电池状态 (0=低电量, 1=正常, 2=充电中)
    uint8_t fusion_status;     // 融合状态 (0=未初始化, 1=正常, 2=GPS丢失)
    
    // 资源使用
    uint32_t free_heap;        // 空闲堆内存 (bytes)
    uint16_t free_heap_kb;     // 空闲堆内存 (KB)
    
    // 统计数据
    float total_distance;      // 总行驶距离 (m)
    float max_speed;           // 最大速度 (m/s)
    float max_lean_angle;      // 最大倾斜角 (度)
    uint32_t gps_updates;      // GPS更新次数
    uint32_t imu_updates;      // IMU更新次数
    uint32_t fusion_updates;   // 融合更新次数
    
    // 错误信息
    uint8_t error_code;        // 错误代码 (0=无错误)
    char error_msg[32];        // 错误消息
    
    uint32_t timestamp;        // 时间戳 (ms)
} BLESystemStatus;

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
    BLECharacteristic* pFusionCharacteristic;
    BLECharacteristic* pSystemCharacteristic;
    
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
    BLEFusionData lastFusionData;
    BLESystemStatus lastSystemStatus;
    
    // 内部方法
    void createService();
    void createCharacteristics();
    String gpsDataToJSON(const BLEGPSData& data);
    String batteryDataToJSON(const BLEBatteryData& data);
    String imuDataToJSON(const BLEIMUData& data);
    String fusionDataToJSON(const BLEFusionData& data);
    String systemStatusToJSON(const BLESystemStatus& data);

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
    void updateFusionData(const BLEFusionData& data);
    void updateSystemStatus(const BLESystemStatus& data);
    
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
