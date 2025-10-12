#ifndef FUSION_LOCATION_MANAGER_H
#define FUSION_LOCATION_MANAGER_H

#include <Arduino.h>
#include <MadgwickAHRS.h>
#include "config.h"

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

// Air780EG GPS数据
#include "Air780EG.h"

// 融合算法类型 - 现在只使用MadgwickAHRS
enum FusionAlgorithm {
    FUSION_MADGWICK_AHRS     // MadgwickAHRS算法
};

// 摩托车轨迹点数据结构
struct MotorcycleTrajectoryPoint {
    double lat, lng;        // 经纬度
    float altitude;         // 高度
    float speed;            // 速度 (m/s)
    float heading;          // 航向角 (度)
    float pitch;            // 俯仰角 (度)
    float roll;             // 横滚角 (度)
    float leanAngle;        // 倾斜角 (度)
    uint32_t timestamp;     // 时间戳
    bool valid;             // 数据有效性
    
    MotorcycleTrajectoryPoint() : lat(0), lng(0), altitude(0), speed(0), 
                                 heading(0), pitch(0), roll(0), leanAngle(0), 
                                 timestamp(0), valid(false) {}
};

// 摩托车运动状态
struct MotorcycleMotionState {
    bool isAccelerating;    // 是否加速
    bool isBraking;         // 是否制动
    bool isLeaning;         // 是否倾斜
    bool isWheelie;         // 是否翘头
    bool isStoppie;         // 是否翘尾
    bool isDrifting;        // 是否漂移
    float leanAngle;        // 倾斜角
    float leanRate;         // 倾斜角速度
    float forwardAccel;     // 前进加速度
    float lateralAccel;     // 侧向加速度
    uint32_t timestamp;     // 时间戳
    
    MotorcycleMotionState() : isAccelerating(false), isBraking(false), 
                             isLeaning(false), isWheelie(false), isStoppie(false), 
                             isDrifting(false), leanAngle(0), leanRate(0), 
                             forwardAccel(0), lateralAccel(0), timestamp(0) {}
};

/**
 * @brief 融合定位管理器
 * 基于MadgwickAHRS的摩托车惯导系统
 */
class FusionLocationManager {
private:
    static const char* TAG;
    
    // Madgwick AHRS算法
    MadgwickAHRS ahrs;
    
    // 配置参数
    bool initialized;
    bool debug_enabled;
    unsigned long update_interval;
    unsigned long last_update_time;
    unsigned long last_debug_print_time;
    unsigned long last_motion_update_time;
    
    // 初始位置（可配置）
    double initial_latitude;
    double initial_longitude;
    double originLat, originLng;
    bool hasOrigin;
    
    // 当前位置和状态
    MotorcycleTrajectoryPoint currentPosition;
    MotorcycleMotionState currentMotionState;
    
    // 积分状态
    float velocity[3];  // 速度积分 [x, y, z]
    float position[3];  // 位置积分 [x, y, z]
    float lastVelocity[3];  // 上次速度，用于加速度计算
    
    // 零速检测
    bool isStationary;
    uint32_t stationaryStartTime;
    static const uint32_t STATIONARY_THRESHOLD = 1000; // 1秒
    
    // 轨迹记录
    bool isRecording;
    float totalDistance;
    float maxSpeed;
    float maxLeanAngle;
    
    // 运动检测阈值
    float accelerationThreshold;
    float brakingThreshold;
    float leanThreshold;
    float wheelieThreshold;
    float stoppieThreshold;
    float driftThreshold;
    
    // LBS/WiFi兜底定位配置
    struct FallbackLocationConfig {
        bool enabled;                    // 是否启用兜底定位
        unsigned long gnss_timeout;     // GNSS信号丢失超时时间(ms)
        unsigned long lbs_interval;     // LBS定位间隔(ms)
        unsigned long wifi_interval;    // WiFi定位间隔(ms)
        bool prefer_wifi_over_lbs;      // 是否优先使用WiFi定位
        unsigned long last_lbs_time;    // 上次LBS定位时间
        unsigned long last_wifi_time;   // 上次WiFi定位时间
        bool lbs_in_progress;           // LBS定位进行中标志
        bool wifi_in_progress;          // WiFi定位进行中标志
    } fallbackConfig;
    
    // 状态统计
    struct {
        unsigned long total_updates;
        unsigned long gps_updates;
        unsigned long imu_updates;
        unsigned long mag_updates;
        unsigned long fusion_updates;
        unsigned long lbs_updates;      // LBS定位次数
        unsigned long wifi_updates;     // WiFi定位次数
    } stats;
    
    void debugPrint(const String& message);
    
    // MadgwickAHRS相关方法
    void updateAHRS();
    void updatePosition();
    void updateMotionState();
    void applyZUPT();  // 零速更新
    void calculateRelativePosition();
    bool detectStationary();
    
    // 运动检测
    bool detectAcceleration();
    bool detectBraking();
    bool detectLeaning();
    bool detectWheelie();
    bool detectStoppie();
    bool detectDrifting();
    
    // 坐标转换
    void latLngToXY(double lat, double lng, float& x, float& y);
    void xyToLatLng(float x, float y, double& lat, double& lng);
    
    // 轨迹统计
    void updateTrajectoryStatistics();
    
    // 兜底定位相关方法
    void handleFallbackLocation();
    bool isGNSSSignalLost();
    bool tryLBSLocation();
    bool tryWiFiLocation();
    
public:
    FusionLocationManager();
    ~FusionLocationManager();
    
    /**
     * @brief 初始化融合定位系统
     * @param initLat 初始纬度（默认使用北京坐标）
     * @param initLng 初始经度
     * @return 是否初始化成功
     */
    bool begin(double initLat = 39.9042, double initLng = 116.4074);
    
    /**
     * @brief 主循环更新
     * 需要在主循环中定期调用
     */
    void loop();
    
    /**
     * @brief 获取当前轨迹点
     * @return MotorcycleTrajectoryPoint结构体，包含位置、姿态、运动信息
     */
    MotorcycleTrajectoryPoint getCurrentPosition();
    
    /**
     * @brief 获取当前运动状态
     * @return MotorcycleMotionState结构体，包含各种运动检测结果
     */
    MotorcycleMotionState getMotionState();
    
    /**
     * @brief 开始记录轨迹
     */
    void startRecording();
    
    /**
     * @brief 停止记录轨迹
     */
    void stopRecording();
    
    /**
     * @brief 清除轨迹记录
     */
    void clearTrajectory();
    
    /**
     * @brief 获取位置精度估计
     * @return 位置精度（米）
     */
    float getPositionAccuracy();
    
    /**
     * @brief 获取当前航向
     * @return 航向角（度，0-360）
     */
    float getHeading();
    
    /**
     * @brief 获取当前速度
     * @return 速度（m/s）
     */
    float getSpeed();
    
    /**
     * @brief 检查系统是否已初始化
     */
    bool isInitialized() const { return initialized; }
    
    /**
     * @brief 检查位置数据是否有效
     */
    bool isPositionValid();
    
    /**
     * @brief 设置起始点
     * @param lat 纬度
     * @param lng 经度
     */
    void setOrigin(double lat, double lng);
    
    /**
     * @brief 重置起始点为当前位置
     */
    void resetOrigin();
    
    /**
     * @brief 设置更新间隔
     * @param interval_ms 更新间隔（毫秒）
     */
    void setUpdateInterval(unsigned long interval_ms);
    
    /**
     * @brief 启用/禁用调试输出
     */
    void setDebug(bool enable);
    
    /**
     * @brief 打印系统状态
     */
    void printStatus();
    
    /**
     * @brief 打印统计信息
     */
    void printStats();
    
    /**
     * @brief 获取位置信息的JSON字符串
     */
    String getPositionJSON();
    
    /**
     * @brief 配置LBS/WiFi兜底定位
     * @param enable 是否启用兜底定位
     * @param gnss_timeout GNSS信号丢失超时时间(ms)，默认30秒
     * @param lbs_interval LBS定位间隔(ms)，默认5分钟
     * @param wifi_interval WiFi定位间隔(ms)，默认3分钟
     * @param prefer_wifi 是否优先使用WiFi定位，默认true
     */
    void configureFallbackLocation(bool enable = true, 
                                 unsigned long gnss_timeout = 30000,
                                 unsigned long lbs_interval = 300000,
                                 unsigned long wifi_interval = 180000,
                                 bool prefer_wifi = true);
    
    /**
     * @brief 手动触发LBS定位
     * @return 是否成功启动定位请求
     */
    bool requestLBSLocation();
    
    /**
     * @brief 手动触发WiFi定位
     * @return 是否成功启动定位请求
     */
    bool requestWiFiLocation();
    
    /**
     * @brief 获取当前定位来源
     * @return 定位来源字符串 ("GNSS", "WiFi", "LBS", "Unknown")
     */
    String getLocationSource();
    
    /**
     * @brief 获取总距离
     * @return 总距离（米）
     */
    float getTotalDistance();
    
    /**
     * @brief 获取最大速度
     * @return 最大速度（m/s）
     */
    float getMaxSpeed();
    
    /**
     * @brief 获取最大倾斜角
     * @return 最大倾斜角（度）
     */
    float getMaxLeanAngle();
    
    /**
     * @brief 重置统计信息
     */
    void resetStats();
    
    /**
     * @brief 获取数据源状态
     */
    struct DataSourceStatus {
        bool imu_available;
        bool gps_available;
        bool mag_available;
        bool fusion_valid;
        unsigned long last_imu_time;
        unsigned long last_gps_time;
        unsigned long last_mag_time;
        unsigned long last_fusion_time;
    };
    
    DataSourceStatus getDataSourceStatus();
};

// 全局融合定位管理器实例
#ifdef ENABLE_FUSION_LOCATION
extern FusionLocationManager fusionLocationManager;
#endif

#endif // FUSION_LOCATION_MANAGER_H
