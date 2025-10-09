#ifndef DATA_COLLECTOR_H
#define DATA_COLLECTOR_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "device.h"
#include "config.h"

#ifdef USE_AIR780EG_GSM
#include "Air780EG.h"
#endif

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

#include "location/FusionLocationManager.h"

// 数据采集模式
enum DataCollectionMode {
    MODE_NORMAL = 0,    // 正常模式：5秒一次
    MODE_SPORT = 1      // 运动模式：1秒一次
};

// IMU数据统计结构
typedef struct {
    // 加速度极值 (g)
    float accel_x_min, accel_x_max;
    float accel_y_min, accel_y_max;
    float accel_z_min, accel_z_max;
    
    // 陀螺仪极值 (°/s)
    float gyro_x_min, gyro_x_max;
    float gyro_y_min, gyro_y_max;
    float gyro_z_min, gyro_z_max;
    
    // 姿态角极值 (度)
    float roll_min, roll_max;
    float pitch_min, pitch_max;
    float yaw_min, yaw_max;
    
    // 统计时间戳
    unsigned long start_time;
    unsigned long last_update;
    
    // 数据有效性
    bool valid;
} imu_stats_t;

// 综合传感器数据结构
typedef struct {
    // GPS数据
    struct {
        double latitude;
        double longitude;
        float altitude;
        float speed;
        float course;
        int satellites;
        float hdop;
        bool valid;
        unsigned long timestamp;
    } gps;
    
    // IMU统计数据
    imu_stats_t imu_stats;
    
    // 罗盘数据
    struct {
        float heading;
        float x, y, z;
        bool valid;
        unsigned long timestamp;
    } compass;
    
    // 系统状态
    struct {
        float battery_voltage;
        int signal_strength;
        DataCollectionMode mode;
        unsigned long uptime;
    } system;
    
} sensor_data_t;

class DataCollector {
public:
    DataCollector();
    
    void begin();
    void loop();
    
    // 模式控制
    void setMode(DataCollectionMode mode);
    DataCollectionMode getMode() const { return current_mode; }
    
    // 数据采集控制
    void startCollection();
    void stopCollection();
    bool isCollecting() const { return collecting; }
    
    // 数据获取
    sensor_data_t getCurrentData();
    String getDataJSON();
    String getIMUStatsJSON();
    String getGPSJSON();
    String getCompassJSON();
    
    // 统计重置
    void resetIMUStats();
    
    // 传输控制
    void enableTransmission(bool enable) { transmission_enabled = enable; }
    bool isTransmissionEnabled() const { return transmission_enabled; }
    
    // 调试
    void setDebug(bool debug) { debug_enabled = debug; }
    void printStats();

private:
    DataCollectionMode current_mode;
    bool collecting;
    bool transmission_enabled;
    bool debug_enabled;
    
    // 定时器
    unsigned long last_collection_time;
    unsigned long last_transmission_time;
    unsigned long collection_interval;  // 采集间隔(ms)
    unsigned long transmission_interval; // 传输间隔(ms)
    
    // 数据缓存
    sensor_data_t current_sensor_data;
    imu_stats_t imu_statistics;
    
    // 内部方法
    void updateCollectionInterval();
    void collectGPSData();
    void collectIMUData();
    void collectCompassData();
    void collectSystemData();
    void updateIMUStatistics();
    bool transmitData();
    
    // MQTT传输
    bool publishToMQTT(const String& topic, const String& payload);
    String generateMQTTTopic(const String& data_type);
    
    // 调试输出
    void debugPrint(const String& message);
};

extern DataCollector dataCollector;

#endif // DATA_COLLECTOR_H
