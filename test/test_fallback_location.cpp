/**
 * @file test_fallback_location.cpp
 * @brief FusionLocationManager 兜底定位功能测试
 * 
 * 测试场景：
 * 1. 正常GNSS定位
 * 2. GNSS信号丢失时的LBS兜底定位
 * 3. GNSS信号丢失时的WiFi兜底定位
 * 4. 手动触发兜底定位
 */

#include <Arduino.h>
#include "location/FusionLocationManager.h"
#include "Air780EG.h"

// 测试配置
const unsigned long TEST_DURATION = 300000;  // 5分钟测试
const unsigned long PRINT_INTERVAL = 10000;  // 10秒打印一次状态

unsigned long testStartTime = 0;
unsigned long lastPrintTime = 0;
int testPhase = 0;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== FusionLocationManager 兜底定位功能测试 ===");
    
    // 初始化Air780EG模块
    if (!air780eg.begin(&Serial2, 115200, 16, 17, 4)) {
        Serial.println("❌ Air780EG初始化失败");
        return;
    }
    
    // 等待网络连接
    Serial.println("等待网络连接...");
    while (!air780eg.getNetwork().isNetworkRegistered()) {
        air780eg.loop();
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\n✅ 网络连接成功");
    
    // 初始化融合定位系统
    if (!fusionLocationManager.begin(FUSION_EKF_VEHICLE, 39.9042, 116.4074)) {
        Serial.println("❌ 融合定位系统初始化失败");
        return;
    }
    
    // 配置兜底定位 - 测试用的短间隔
    fusionLocationManager.configureFallbackLocation(
        true,      // 启用兜底定位
        15000,     // GNSS信号丢失超时时间 15秒（测试用）
        60000,     // LBS定位间隔 1分钟（测试用）
        45000,     // WiFi定位间隔 45秒（测试用）
        true       // 优先使用WiFi定位
    );
    
    // 启用调试输出
    fusionLocationManager.setDebug(true);
    
    testStartTime = millis();
    lastPrintTime = millis();
    
    Serial.println("✅ 测试初始化完成");
    Serial.println("测试阶段：");
    Serial.println("  阶段0: 正常GNSS定位测试 (0-60秒)");
    Serial.println("  阶段1: 手动LBS定位测试 (60-120秒)");
    Serial.println("  阶段2: 手动WiFi定位测试 (120-180秒)");
    Serial.println("  阶段3: 兜底定位自动测试 (180-300秒)");
    Serial.println();
}

void loop() {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - testStartTime;
    
    // 更新系统
    air780eg.loop();
    fusionLocationManager.loop();
    
    // 测试阶段控制
    int newPhase = elapsedTime / 60000;  // 每60秒一个阶段
    if (newPhase != testPhase && newPhase < 5) {
        testPhase = newPhase;
        Serial.printf("\n=== 进入测试阶段 %d ===\n", testPhase);
        
        switch (testPhase) {
            case 0:
                Serial.println("测试正常GNSS定位...");
                break;
                
            case 1:
                Serial.println("测试手动LBS定位...");
                if (fusionLocationManager.requestLBSLocation()) {
                    Serial.println("✅ LBS定位请求已发送");
                } else {
                    Serial.println("❌ LBS定位请求失败");
                }
                break;
                
            case 2:
                Serial.println("测试手动WiFi定位...");
                if (fusionLocationManager.requestWiFiLocation()) {
                    Serial.println("✅ WiFi定位请求已发送");
                } else {
                    Serial.println("❌ WiFi定位请求失败");
                }
                break;
                
            case 3:
                Serial.println("测试兜底定位自动触发...");
                Serial.println("注意：此阶段会在GNSS信号丢失时自动触发兜底定位");
                break;
                
            case 4:
                Serial.println("测试完成，显示最终统计...");
                fusionLocationManager.printStats();
                break;
        }
    }
    
    // 定期打印状态
    if (currentTime - lastPrintTime >= PRINT_INTERVAL) {
        lastPrintTime = currentTime;
        
        Position pos = fusionLocationManager.getFusedPosition();
        String locationSource = fusionLocationManager.getLocationSource();
        
        Serial.printf("\n--- 状态报告 (阶段%d, %lu秒) ---\n", testPhase, elapsedTime/1000);
        
        if (pos.valid) {
            Serial.printf("位置: %.6f, %.6f\n", pos.lat, pos.lng);
            Serial.printf("精度: %.1fm | 来源: %s\n", pos.accuracy, locationSource.c_str());
            Serial.printf("航向: %.1f° | 速度: %.1fm/s\n", pos.heading, pos.speed);
        } else {
            Serial.println("位置: 无效");
        }
        
        // 显示GNSS状态
        gnss_data_t& gnss = air780eg.getGNSS().gnss_data;
        Serial.printf("GNSS: %s | 卫星: %d | 类型: %s\n", 
                     gnss.data_valid ? "有效" : "无效",
                     gnss.satellites,
                     gnss.location_type.c_str());
        
        // 显示网络状态
        Serial.printf("网络: %s | 信号: %ddBm\n",
                     air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接",
                     air780eg.getNetwork().getSignalStrength());
        
        Serial.println("---");
    }
    
    // 测试结束
    if (elapsedTime >= TEST_DURATION) {
        Serial.println("\n=== 测试完成 ===");
        fusionLocationManager.printStats();
        
        // 停止测试
        while (true) {
            delay(1000);
        }
    }
    
    delay(100);
}

/**
 * 测试预期结果：
 * 
 * 1. 阶段0：应该看到GNSS定位数据，location_type为"GNSS"
 * 2. 阶段1：手动LBS定位应该成功，可能需要30秒时间
 * 3. 阶段2：手动WiFi定位应该成功，可能需要30秒时间
 * 4. 阶段3：如果GNSS信号丢失，应该自动触发兜底定位
 * 5. 统计信息应该显示各种定位方式的使用次数
 * 
 * 注意事项：
 * - 确保SIM卡有流量，LBS和WiFi定位需要网络连接
 * - 在室外测试以获得更好的GNSS信号
 * - 在WiFi热点附近测试WiFi定位功能
 * - 观察串口输出的调试信息
 */
