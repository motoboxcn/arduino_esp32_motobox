#ifndef FUSION_LOCATION_MANAGER_H
#define FUSION_LOCATION_MANAGER_H

#include <Arduino.h>
#include <FusionLocation.h>
#include "config.h"

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

// Air780EG GPS数据
#include "Air780EG.h"

/**
 * @brief 项目专用的IMU数据提供者
 * 适配QMI8658传感器数据到FusionLocation库
 */
class MotoBoxIMUProvider : public IIMUProvider {
private:
    bool debug_enabled;
    unsigned long last_update_time;
    
public:
    MotoBoxIMUProvider();
    
    bool getData(IMUData& data) override;
    bool isAvailable() override;
    
    void setDebug(bool enable) { debug_enabled = enable; }
};

/**
 * @brief 项目专用的GPS数据提供者
 * 适配Air780EG GNSS数据到FusionLocation库
 */
class MotoBoxGPSProvider : public IGPSProvider {
private:
    bool debug_enabled;
    unsigned long last_update_time;
    
public:
    MotoBoxGPSProvider();
    
    bool getData(GPSData& data) override;
    bool isAvailable() override;
    
    void setDebug(bool enable) { debug_enabled = enable; }
};

/**
 * @brief 项目专用的地磁计数据提供者
 * 适配QMC5883L罗盘数据到FusionLocation库
 */
class MotoBoxMagProvider : public IMagProvider {
private:
    bool debug_enabled;
    unsigned long last_update_time;
    
public:
    MotoBoxMagProvider();
    
    bool getData(MagData& data) override;
    bool isAvailable() override;
    
    void setDebug(bool enable) { debug_enabled = enable; }
};

/**
 * @brief 融合定位管理器
 * 整合IMU、GPS、地磁计数据，提供高精度位置信息
 */
class FusionLocationManager {
private:
    static const char* TAG;
    
    // 传感器提供者
    MotoBoxIMUProvider* imuProvider;
    MotoBoxGPSProvider* gpsProvider;
    MotoBoxMagProvider* magProvider;
    
    // 融合定位对象
    FusionLocation* fusionLocation;
    
    // 配置参数
    bool initialized;
    bool debug_enabled;
    unsigned long update_interval;
    unsigned long last_update_time;
    unsigned long last_debug_print_time;
    
    // 初始位置（可配置）
    double initial_latitude;
    double initial_longitude;
    
    // 状态统计
    struct {
        unsigned long total_updates;
        unsigned long gps_updates;
        unsigned long imu_updates;
        unsigned long mag_updates;
        unsigned long fusion_updates;
    } stats;
    
    void debugPrint(const String& message);
    void updateStats(const Position& pos);
    
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
     * @brief 获取融合后的位置信息
     * @return Position结构体，包含融合后的位置、精度、航向等信息
     */
    Position getFusedPosition();
    
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
     * @brief 设置初始位置
     * @param lat 纬度
     * @param lng 经度
     */
    void setInitialPosition(double lat, double lng);
    
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
