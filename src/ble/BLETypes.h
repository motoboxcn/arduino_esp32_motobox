#ifndef BLE_TYPES_H
#define BLE_TYPES_H

#include <Arduino.h>

#ifdef ENABLE_BLE

// BLE专用的设备状态结构定义
// 避免与device.h的循环包含问题
typedef struct {
    // 设备基础信息
    String device_id;
    uint32_t timestamp;
    String firmware;
    String hardware;
    int power_mode;
    
    // 位置数据
    struct {
        double lat;
        double lng;
        float altitude;
        float speed;
        float heading;
        uint8_t satellites;
        float hdop;
        bool valid;
        uint32_t timestamp;
    } location;
    
    // 传感器数据
    struct {
        // IMU数据
        struct {
            float accel_x, accel_y, accel_z;
            float gyro_x, gyro_y, gyro_z;
            float roll, pitch, yaw;
            bool valid;
            uint32_t timestamp;
        } imu;
        
        // 罗盘数据
        struct {
            float heading;
            float mag_x, mag_y, mag_z;
            bool valid;
            uint32_t timestamp;
        } compass;
    } sensors;
    
    // 系统状态
    struct {
        int battery_voltage;
        int battery_percentage;
        bool is_charging;
        bool external_power;
        int signal_strength;
        uint32_t uptime;
        uint32_t free_heap;
    } system;
    
    // 模块状态
    struct {
        bool wifi_ready;
        bool ble_ready;
        bool gsm_ready;
        bool gnss_ready;
        bool imu_ready;
        bool compass_ready;
        bool sd_ready;
        bool audio_ready;
    } modules;
    
    // 存储信息
    struct {
        uint64_t size_mb;
        uint64_t free_mb;
    } storage;
} ble_device_state_t;

#endif // ENABLE_BLE

#endif // BLE_TYPES_H
