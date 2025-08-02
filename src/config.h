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
#define LED_DEBUG_ENABLED true // LED调试已启用

// #define ENABLE_TFT  // 暂时禁用TFT
#define ENABLE_BLE

// BLE配置
#define BLE_NAME                      "ESP32-MotoBox"
#define SERVICE_UUID        "4FAFC201-1FB5-459E-8FCC-C5C9C331914B"
#define DEVICE_CHAR_UUID    "BEB5483A-36E1-4688-B7F5-EA07361B26A8"
#define GPS_CHAR_UUID       "BEB5483E-36E1-4688-B7F5-EA07361B26A8"
#define IMU_CHAR_UUID       "BEB5483F-36E1-4688-B7F5-EA07361B26A8"



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
#define AIR780EG_TIMEOUT             5000
#define AIR780EG_GNSS_UPDATE_RATE    1000  // 1Hz
#define AIR780EG_NETWORK_CHECK_INTERVAL 5000  // 5秒
#define AIR780EG_LOG_VERBOSE_ENABLED false

// MQTT配置
#define MQTT_BROKER                  "222.186.32.152"
#define MQTT_PORT                    32571
#define MQTT_CLIENT_ID_PREFIX        ""
#define MQTT_USERNAME                "box"
#define MQTT_PASSWORD                "box"
#define MQTT_KEEPALIVE               60
#define MQTT_RECONNECT_INTERVAL      30000
#define MQTT_GPS_PUBLISH_INTERVAL     5000 // 5秒上报一次
#define MQTT_DEVICE_STATUS_PUBLISH_INTERVAL 30000 // 30秒上报一次

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

// GPS记录器配置
#ifdef ENABLE_GPS_LOGGER
#define GPS_LOG_INTERVAL_MS          5000    // GPS数据记录间隔（毫秒）
#define GPS_MIN_SATELLITES           4       // 最小卫星数量（有效GPS数据）
#define GPS_MAX_LOG_FILES            100     // 最大日志文件数量
#define GPS_AUTO_EXPORT_GEOJSON      false   // 是否自动导出GeoJSON
#define GPS_STORAGE_CHECK_INTERVAL   600000  // 存储空间检查间隔（10分钟）
#define GPS_LOG_DIR                  "/logs/gps"
#define GPS_CONFIG_FILE              "/config/gps.json"
#define GPS_COORDINATE_PRECISION     8       // 坐标精度（小数位数）
#define GPS_ALTITUDE_PRECISION       2       // 高度精度
#define GPS_SPEED_PRECISION          2       // 速度精度
#define GPS_MIN_FREE_SPACE_MB        50      // 最小可用空间（MB）
#define GPS_AUTO_CLEANUP_DAYS        30      // 自动清理天数
#define GPS_LOGGER_DEBUG_ENABLED      false
#endif

// 融合定位功能
#define ENABLE_FUSION_LOCATION
#define FUSION_EKF_VEHICLE_ENABLED    true


// #define AT_DEBUG_ENABLED

// 融合定位配置
#ifdef ENABLE_FUSION_LOCATION
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
#endif // ENABLE_FUSION_LOCATION

#endif // CONFIG_H
