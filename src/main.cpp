/**
 * ESP32 MotoBox - 摩托车数据采集与监控系统 (轻量化版本)
 *
 * 硬件：ESP32-S3
 * 版本：Lite 1.0.0
 *
 * 核心功能：
 * - 4G通信 (Air780EG)
 * - GNSS定位 + IMU融合定位
 * - 数据采集与SD卡存储
 * - 低功耗管理
 * - MQTT数据上报
 */

#include "config.h"
#include "Arduino.h"
#include "config.h"
#include "power/PowerManager.h"
#include "power/PowerModeManager.h"
#include "led/LEDManager.h"
#include "device.h"
#include "Air780EG.h"
#include "utils/serialCommand.h"
#include "utils/DataCollector.h"
#include "ota/OTAManager.h"

#ifdef BAT_PIN
#include "bat/BAT.h"
#endif

#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif

#ifdef LED_PIN
#include "led/LED.h"
#endif

#ifdef ENABLE_COMPASS
#include "compass/Compass.h"
#endif

#ifdef ENABLE_FUSION_LOCATION
#include "location/FusionLocationManager.h"
#endif

// GSM模块包含
#ifdef USE_AIR780EG_GSM
// 直接使用新的Air780EG库
#include <Air780EG.h>

#elif defined(USE_ML307_GSM)
#include "net/Ml307AtModem.h"
extern Ml307AtModem ml307_at;
#endif

#ifdef ENABLE_IMU
#include "imu/qmi8658.h"
#endif

#include "version.h"

#include <SD.h> // SD卡库
#include <FS.h>

//============================= 全局变量 =============================

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

/**
 * 系统监控任务
 * 负责电源管理、LED状态等核心功能
 */
void taskSystem(void *parameter)
{
  // 添加任务启动提示
  Serial.println("[系统] 系统监控任务启动");

  while (true)
  {

    // 高频更新：IMU数据读取和融合定位系统更新
#ifdef ENABLE_IMU
    imu.loop();
#endif

#ifdef ENABLE_FUSION_LOCATION
    fusionLocationManager.loop();
#endif

    // 电源管理
    powerManager.loop();
    
    // 功耗模式管理
    #ifdef ENABLE_POWER_MODE_MANAGEMENT
    powerModeManager.loop();
    #endif

    // LED状态更新
    ledManager.loop();

#ifdef BAT_PIN
    bat.loop();
#endif

#ifdef RTC_INT_PIN
    externalPower.loop();
#endif

#ifdef PWM_LED_PIN
    pwmLed.loop();
#endif

    // 串口命令处理
    if (Serial.available())
    {
      handleSerialCommand();
    }

    delay(5);
  }
}

/**
 * 数据处理任务
 * 负责数据采集、发送和存储
 */
void taskDataProcessing(void *parameter)
{
  Serial.println("[系统] 数据处理任务启动");

  // 数据记录相关变量
  unsigned long lastGNSSRecordTime = 0;
  unsigned long lastIMURecordTime = 0;

  for (;;)
  {
    // Air780EG库处理 - 必须在主循环中调用
#ifdef USE_AIR780EG_GSM
    // 调用新库的主循环（处理URC、网络状态更新、GNSS数据更新等）
    air780eg.loop();
#endif

#ifdef ENABLE_COMPASS
    // 罗盘数据处理
    compass.loop();
#endif

    delay(10); // 增加延时，减少CPU占用
  } // for循环结束
}



void setup()
{
  Serial.begin(115200);
  delay(1000);

  PreferencesUtils::init();

  bootCount++;
  Serial.println("[系统] 启动次数: " + String(bootCount));

  powerManager.begin();

  powerManager.printWakeupReason();
  
  // 初始化功耗模式管理器
  #ifdef ENABLE_POWER_MODE_MANAGEMENT
    powerModeManager.begin();
  #endif

  device.begin();

  //================ 融合定位初始化开始 ================
#ifdef ENABLE_FUSION_LOCATION
  Serial.println("[融合定位] 初始化融合定位系统...");
  // 使用EKF车辆模型算法，适合摩托车应用 FUSION_EKF_VEHICLE FUSION_SIMPLE_KALMAN
  #if FUSION_EKF_VEHICLE_ENABLED == true
  if (fusionLocationManager.begin(FUSION_EKF_VEHICLE, FUSION_LOCATION_INITIAL_LAT, FUSION_LOCATION_INITIAL_LNG))
  #else
  if (fusionLocationManager.begin(FUSION_SIMPLE_KALMAN, FUSION_LOCATION_INITIAL_LAT, FUSION_LOCATION_INITIAL_LNG))
  #endif
  {
    Serial.println("[融合定位] ✅ EKF融合定位系统初始化成功");
    fusionLocationManager.setDebug(FUSION_LOCATION_DEBUG_ENABLED);
    fusionLocationManager.setUpdateInterval(FUSION_LOCATION_UPDATE_INTERVAL);

    // 设置摩托车专用参数
    EKFConfig motoConfig;
    motoConfig.processNoisePos = 0.8f;     // 摩托车位置变化较快
    motoConfig.processNoiseVel = 3.0f;     // 速度变化较大
    motoConfig.processNoiseHeading = 0.1f; // 航向变化频繁
    motoConfig.gpsNoisePos = 16.0f;        // GPS精度约4m
    motoConfig.imuNoiseAccel = 0.3f;       // 摩托车振动较大
    fusionLocationManager.setEKFConfig(motoConfig);

    VehicleModel motoModel;
    motoModel.wheelbase = 1.4f;        // 摩托车轴距
    motoModel.maxAcceleration = 5.0f;  // 摩托车加速性能好
    motoModel.maxDeceleration = 12.0f; // 制动性能强
    motoModel.maxSteeringAngle = 1.0f; // 转向角度大
    fusionLocationManager.setVehicleModel(motoModel);

    Serial.println("[融合定位] 摩托车专用参数已设置");
  }
  else
  {
    Serial.println("[融合定位] ❌ 融合定位系统初始化失败");
  }
  // 配置兜底定位 - 测试用的短间隔
  fusionLocationManager.configureFallbackLocation(
    true,      // 启用兜底定位
    15000,     // GNSS信号丢失超时时间 15秒（测试用）
    60000,     // LBS定位间隔 1分钟（测试用）
    120000,     // WiFi定位间隔 120秒
    true       // 优先使用WiFi定位
);

#endif
  //================ 融合定位初始化结束 ================

  // 创建任务
  xTaskCreate(taskSystem, "TaskSystem", 1024 * 15, NULL, 1, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 15, NULL, 2, NULL);
  
  // 初始化数据采集器
  Serial.println("=== 数据采集器初始化 ===");
  dataCollector.setDebug(true);
  dataCollector.begin();
  
  Serial.println("=== 系统初始化完成 ===");
}

void loop()
{
  static unsigned long loopCount = 0;
  static unsigned long lastLoopReport = 0;
  static unsigned long lastSlowUpdate = 0;
  loopCount++;

  // 状态报告（每10秒一次）
  if (millis() - lastLoopReport > 10000)
  {
    lastLoopReport = millis();
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t minFreeHeap = ESP.getMinFreeHeap();
    Serial.printf("[主循环] 运行正常，循环计数: %lu, 空闲内存: %d 字节\n", loopCount, freeHeap);
    if (freeHeap < 50000)
    {
      Serial.printf("[警告] 内存不足: %d 字节，最小值: %d 字节\n", freeHeap, minFreeHeap);
    }
    if (freeHeap < 20000)
    {
      Serial.println("[严重] 内存严重不足，即将重启系统...");
      delay(1000);
      ESP.restart();
    }

#ifdef ENABLE_COMPASS
    printCompassData();
#endif

    // 打印融合定位状态
#ifdef ENABLE_FUSION_LOCATION
    if (fusionLocationManager.isInitialized())
    {
      Position pos = fusionLocationManager.getFusedPosition();
      if (pos.valid)
      {
        fusionLocationManager.printStats();
      }
    }
#endif
  }

  // 数据采集器处理
  dataCollector.loop();

  // 最小延迟，实现高频更新（约1000Hz）
  delay(1);
}