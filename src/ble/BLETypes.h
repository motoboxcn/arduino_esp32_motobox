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
        float vdop;
        float pdop;
        uint8_t fix_type;
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
            float temperature;
            bool valid;
            uint32_t timestamp;
        } imu;
        
        // 罗盘数据
        struct {
            float heading;
            float mag_x, mag_y, mag_z;
            float declination;
            float inclination;
            float field_strength;
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
        int cpu_usage;
        float temperature;
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
        uint64_t total_mb;
        uint64_t free_mb;
        int used_percentage;
    } storage;
    
    // 网络信息
    struct {
        bool wifi_connected;
        bool gsm_connected;
        String ip_address;
        String operator_name;
    } network;
} ble_device_state_t;

// 模块化数据结构定义
typedef struct {
    String device_id;
    uint32_t timestamp;
    struct {
        double lat;
        double lng;
        float altitude;
        float speed;
        float heading;
        uint8_t satellites;
        float hdop;
        float vdop;
        float pdop;
        uint8_t fix_type;
        bool valid;
        uint32_t timestamp;
    } location;
    struct {
        bool gnss_ready;
        String fix_quality;
        uint32_t last_fix_age;
    } status;
} ble_gps_data_t;

typedef struct {
    String device_id;
    uint32_t timestamp;
    struct {
        struct {
            float x, y, z;
        } accel;
        struct {
            float x, y, z;
        } gyro;
        struct {
            float roll, pitch, yaw;
        } attitude;
        float temperature;
        bool valid;
        uint32_t timestamp;
    } imu;
    struct {
        bool imu_ready;
        bool calibrated;
        bool motion_detected;
        float vibration_level;
    } status;
} ble_imu_data_t;

typedef struct {
    String device_id;
    uint32_t timestamp;
    struct {
        float heading;
        struct {
            float x, y, z;
        } magnetic;
        float declination;
        float inclination;
        float field_strength;
        bool valid;
        uint32_t timestamp;
    } compass;
    struct {
        bool compass_ready;
        bool calibrated;
        bool interference;
        int calibration_quality;
    } status;
} ble_compass_data_t;

typedef struct {
    String device_id;
    uint32_t timestamp;
    String firmware;
    String hardware;
    int power_mode;
    struct {
        int battery_voltage;
        int battery_percentage;
        bool is_charging;
        bool external_power;
        int signal_strength;
        uint32_t uptime;
        uint32_t free_heap;
        int cpu_usage;
        float temperature;
    } system;
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
    struct {
        uint64_t total_mb;
        uint64_t free_mb;
        int used_percentage;
    } storage;
    struct {
        bool wifi_connected;
        bool gsm_connected;
        String ip_address;
        String operator_name;
    } network;
} ble_system_data_t;

#endif // ENABLE_BLE

#endif // BLE_TYPES_H
