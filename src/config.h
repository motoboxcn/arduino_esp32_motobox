#ifndef CONFIG_H
#define CONFIG_H

/*
 * ESP32-S3 MotoBox Air780EG Configuration
 * 专用于Air780EG模块的配置文件
 */

#define ENABLE_GSM
#define USE_AIR780EG_GSM

// GPS功能通过Air780EG内置GNSS提供
#define ENABLE_GPS
#define USE_AIR780EG_GNSS

// SD卡功能
#ifndef ENABLE_SDCARD
#define ENABLE_SDCARD
#endif

// GPS记录功能 (依赖于SD卡功能)
#ifdef ENABLE_SDCARD
#define ENABLE_GPS_LOGGER
#endif

#define ENABLE_LED
#define LED_DEBUG_ENABLED false // LED调试已禁用（优化性能）

// 轻量化版本 - 移除TFT和BLE功能
// #define ENABLE_TFT  // 轻量化版本不需要显示屏
// #define ENABLE_BLE  // 轻量化版本不需要蓝牙



// 版本信息
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "unknown"
#endif

#ifndef HARDWARE_VERSION
#define HARDWARE_VERSION "esp32-air780eg"
#endif

#ifndef BUILD_NUMBER
#define BUILD_NUMBER 0
#endif

// Air780EG配置
#define AIR780EG_BAUD_RATE           115200
#define AIR780EG_LOG_VERBOSE_ENABLED true


// GNSS查询间隔配置（优化串口资源使用）
#define GNSS_UPDATE_INTERVAL_VALID    3000   // GNSS有效时3秒查询一次
#define GNSS_UPDATE_INTERVAL_INVALID  10000  // GNSS无效时10秒查询一次

// MQTT配置
#define MQTT_BROKER                  "222.186.32.152"
#define MQTT_PORT                    32571
#define MQTT_CLIENT_ID_PREFIX        ""
#define MQTT_USERNAME                "box"
#define MQTT_PASSWORD                "box"
#define MQTT_KEEPALIVE               60
#define MQTT_RECONNECT_INTERVAL      30000
#define MQTT_GPS_PUBLISH_INTERVAL     5000 // 5秒上报一次（正常模式默认值）
#define MQTT_DEVICE_STATUS_PUBLISH_INTERVAL 30000 // 30秒上报一次（正常模式默认值）

// 功耗模式管理
#define ENABLE_POWER_MODE_MANAGEMENT
#define POWER_MODE_AUTO_SWITCH_ENABLED    true    // 默认启用自动模式切换
#define POWER_MODE_EVALUATION_INTERVAL    5000    // 模式评估间隔（毫秒）
#define POWER_MODE_MIN_SWITCH_INTERVAL    30000   // 最小模式切换间隔（毫秒）

// GPS配置
#define GPS_UPDATE_INTERVAL          1000
#define GPS_TIMEOUT                  10000

// OTA升级配置
#define OTA_BATTERY_MIN_LEVEL        90   // 升级所需最低电池电量(%)
#define OTA_CHECK_INTERVAL           3600000  // 在线升级检查间隔(1小时)
#define OTA_DOWNLOAD_TIMEOUT         30000    // 下载超时时间(30秒)
#define OTA_MAX_RETRY_COUNT          3        // 最大重试次数

// 默认引脚定义（如果platformio.ini中未定义）
#ifndef BAT_PIN
#define BAT_PIN                      36   // 电池电压检测引脚
#endif

#ifndef CHARGING_STATUS_PIN
#define CHARGING_STATUS_PIN          2    // 充电状态检测引脚
#endif

#ifndef BUZZER_PIN
#define BUZZER_PIN                   25   // 蜂鸣器引脚
#endif

// 基础定位功能（默认启用）
#define ENABLE_GNSS_LOCATION        true    // 启用GNSS定位
#define ENABLE_FALLBACK_LOCATION    true    // 启用WiFi/LBS兜底定位

// 高级融合定位功能（可选）
#define ENABLE_IMU_FUSION           // 启用IMU惯导修正（替代原来的ENABLE_FUSION_LOCATION）

// 融合定位配置
#ifdef ENABLE_IMU_FUSION
#define FUSION_EKF_VEHICLE_ENABLED    true
#define FUSION_LOCATION_UPDATE_INTERVAL  100     // 融合定位更新间隔（毫秒）
#define FUSION_LOCATION_DEBUG_ENABLED    false     // 调试输出
#define FUSION_LOCATION_INITIAL_LAT      39.9042 // 默认初始纬度（北京）
#define FUSION_LOCATION_INITIAL_LNG      116.4074// 默认初始经度（北京）
#define FUSION_LOCATION_PRINT_INTERVAL   5000    // 状态打印间隔（毫秒）

// EKF算法配置
#define FUSION_USE_EKF_DEFAULT           false    // 默认使用EKF算法
#define FUSION_EKF_PROCESS_NOISE_POS     0.8f    // 位置过程噪声（摩托车）
#define FUSION_EKF_PROCESS_NOISE_VEL     3.0f    // 速度过程噪声
#define FUSION_EKF_PROCESS_NOISE_HEADING 0.1f    // 航向过程噪声
#define FUSION_EKF_GPS_NOISE_POS         16.0f   // GPS测量噪声
#define FUSION_EKF_IMU_NOISE_ACCEL       0.3f    // IMU加速度噪声

// 摩托车模型参数
#define MOTO_WHEELBASE                   1.4f    // 轴距（米）
#define MOTO_MAX_ACCELERATION            5.0f    // 最大加速度（m/s²）
#define MOTO_MAX_DECELERATION            12.0f   // 最大减速度（m/s²）
#define MOTO_MAX_STEERING_ANGLE          1.0f    // 最大转向角（弧度）
#endif // ENABLE_IMU_FUSION

// 电池管理优化配置
#define BATTERY_VOLTAGE_FILTER_ENABLED    true    // 启用电压滤波
#define BATTERY_ADC_SAMPLE_COUNT          8       // ADC采样次数
#define BATTERY_ADC_SAMPLE_DELAY          5       // ADC采样间隔（毫秒）
#define BATTERY_VOLTAGE_THRESHOLD         100     // 电压变化阈值（毫伏）
#define BATTERY_CALIBRATION_ENABLED       true    // 启用电压校准
#define BATTERY_DEBUG_ENABLED             false   // 电池调试输出

#endif // CONFIG_H
