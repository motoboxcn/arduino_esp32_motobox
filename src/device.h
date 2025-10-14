#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "esp_system.h"
#include <ArduinoJson.h>
#include "config.h"
#include "version.h"  // 包含版本信息头文件
#include "compass/Compass.h"
#include "led/PWMLED.h"
#include "led/LED.h"
#include "imu/qmi8658.h"
#include "power/PowerManager.h"
#include "bat/BAT.h"
#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif
#include "led/LEDManager.h"


#ifdef ENABLE_FUSION_LOCATION
#include "location/FusionLocationManager.h"
#endif

#ifdef ENABLE_BLE
#include "ble/BLEManager.h"
#include "ble/BLEDataProvider.h"
#endif


typedef struct
{
    // 设备基础信息
    String device_id; // 设备ID
    String device_firmware_version; // 固件版本
    String device_hardware_version; // 硬件版本
    int sleep_time; // 休眠时间 单位：秒
    int led_mode; // LED模式 0:关闭 1:常亮 2:单闪 3:双闪 4:慢闪 5:快闪 6:呼吸 7:5秒闪烁
    
    // 遥测数据 - 嵌套结构
    struct {
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
    } telemetry;
    
    // SD卡信息（低频更新）
    uint64_t sdCardSizeMB; // SD卡大小(MB)
    uint64_t sdCardFreeMB; // SD卡剩余空间(MB)
    
    // 功耗模式
    int power_mode; // 功耗模式 0:休眠 1:基本 2:正常 3:运动
} device_state_t;

// 添加状态变化跟踪
typedef struct {
        bool battery_changed;      // 电池状态变化
        bool external_power_changed; // 外部电源状态变化
        bool wifi_changed;         // WiFi连接状态变化
        bool ble_changed;          // BLE连接状态变化
        bool gps_changed;          // GPS状态变化
        bool imu_changed;          // IMU状态变化
        bool compass_changed;      // 罗盘状态变化
        bool gsm_changed;          // GSM状态变化
        bool sleep_time_changed;   // 休眠时间变化
        bool led_mode_changed;     // LED模式变化
        bool sdcard_changed;       // SD卡状态变化
        bool audio_changed;        // 音频状态变化
} state_changes_t;

void update_device_state();

extern device_state_t device_state;
extern state_changes_t state_changes;

String device_state_to_json(device_state_t *state);

device_state_t *get_device_state();
void set_device_state(device_state_t *state);
void print_device_info();


class Device
{
public:
    Device();
    String get_device_id();

    // 硬件初始化
    void begin();
    
    // MQTT初始化，需要在网络就绪后调用
    bool initializeMQTT();
    
    // GSM初始化
    void initializeGSM();
    
    // 欢迎语音配置
    void setWelcomeVoiceType(int voiceType);
    void playWelcomeVoice();
    String getWelcomeVoiceInfo();
    
    // 数据更新接口
    void updateLocationData(double lat, double lng, float alt, float speed, 
                           float heading, uint8_t sats, float hdop);
    void updateIMUData(float ax, float ay, float az, float gx, float gy, float gz,
                      float roll, float pitch, float yaw);
    void updateCompassData(float heading, float mx, float my, float mz);
    void updateBatteryData(int voltage, int percentage, bool charging, bool ext_power);
    void updateSystemData(int signal, uint32_t uptime, uint32_t free_heap);
    void updateModuleStatus(const char* module, bool ready);
    
    // 统一MQTT数据获取
    String getCombinedTelemetryJSON();
    
private:
};

extern Device device;

#endif
