#include <Arduino.h>
#include "location/FusionLocationManager.h"
#include "config.h"

#ifdef ENABLE_FUSION_LOCATION

// 测试融合定位功能
void test_fusion_location() {
    Serial.println("=== 融合定位系统测试 ===");
    
    // 初始化融合定位系统
    if (!fusionLocationManager.begin()) {
        Serial.println("❌ 融合定位系统初始化失败");
        return;
    }
    
    // 启用调试输出
    fusionLocationManager.setDebug(true);
    
    // 设置更新间隔
    fusionLocationManager.setUpdateInterval(FUSION_LOCATION_UPDATE_INTERVAL);
    
    Serial.println("✅ 融合定位系统初始化成功");
    Serial.println("开始数据融合测试...");
    
    unsigned long testStartTime = millis();
    unsigned long lastPrintTime = 0;
    const unsigned long testDuration = 60000; // 测试60秒
    
    while (millis() - testStartTime < testDuration) {
        // 更新融合定位
        fusionLocationManager.loop();
        
        // 每5秒打印一次结果
        if (millis() - lastPrintTime > 5000) {
            lastPrintTime = millis();
            
            Position pos = fusionLocationManager.getFusedPosition();
            
            Serial.println("\n--- 融合定位结果 ---");
            if (pos.valid) {
                Serial.printf("位置: %.6f, %.6f\n", pos.lat, pos.lng);
                Serial.printf("精度: %.1fm | 航向: %.1f° | 速度: %.1fm/s\n", 
                             pos.accuracy, pos.heading, pos.speed);
                Serial.printf("高度: %.1fm | 时间戳: %lu\n", pos.altitude, pos.timestamp);
                
                // 显示数据源
                Serial.print("数据源: ");
                if (pos.sources.hasGPS) Serial.print("GPS ");
                if (pos.sources.hasIMU) Serial.print("IMU ");
                if (pos.sources.hasMag) Serial.print("MAG ");
                Serial.println();
                
                // 显示JSON格式
                Serial.println("JSON: " + fusionLocationManager.getPositionJSON());
            } else {
                Serial.println("位置数据无效");
            }
            
            // 显示数据源状态
            auto status = fusionLocationManager.getDataSourceStatus();
            Serial.printf("传感器状态 - IMU:%s GPS:%s MAG:%s\n",
                         status.imu_available ? "✅" : "❌",
                         status.gps_available ? "✅" : "❌", 
                         status.mag_available ? "✅" : "❌");
        }
        
        delay(10);
    }
    
    // 打印最终统计
    Serial.println("\n=== 测试完成 ===");
    fusionLocationManager.printStats();
}

// 测试单个传感器数据
void test_individual_sensors() {
    Serial.println("\n=== 单个传感器测试 ===");
    
    // 测试IMU
    Serial.println("--- IMU测试 ---");
    MotoBoxIMUProvider imuProvider;
    IMUData imuData;
    if (imuProvider.isAvailable() && imuProvider.getData(imuData)) {
        Serial.printf("IMU数据: 加速度(%.2f,%.2f,%.2f) 陀螺仪(%.3f,%.3f,%.3f)\n",
                     imuData.accel[0], imuData.accel[1], imuData.accel[2],
                     imuData.gyro[0], imuData.gyro[1], imuData.gyro[2]);
    } else {
        Serial.println("IMU数据不可用");
    }
    
    // 测试GPS
    Serial.println("--- GPS测试 ---");
    MotoBoxGPSProvider gpsProvider;
    GPSData gpsData;
    if (gpsProvider.isAvailable() && gpsProvider.getData(gpsData)) {
        Serial.printf("GPS数据: 位置(%.6f,%.6f) 高度%.1fm 精度%.1fm\n",
                     gpsData.lat, gpsData.lng, gpsData.altitude, gpsData.accuracy);
    } else {
        Serial.println("GPS数据不可用");
    }
    
    // 测试地磁计
    Serial.println("--- 地磁计测试 ---");
    MotoBoxMagProvider magProvider;
    MagData magData;
    if (magProvider.isAvailable() && magProvider.getData(magData)) {
        Serial.printf("磁场数据: (%.1f,%.1f,%.1f)μT\n",
                     magData.mag[0], magData.mag[1], magData.mag[2]);
    } else {
        Serial.println("地磁计数据不可用");
    }
}

// 主测试函数
void run_fusion_location_tests() {
    Serial.println("开始融合定位测试...");
    
    // 等待传感器初始化
    delay(2000);
    
    // 测试单个传感器
    test_individual_sensors();
    
    delay(1000);
    
    // 测试融合定位
    test_fusion_location();
    
    Serial.println("所有测试完成！");
}

#endif // ENABLE_FUSION_LOCATION
