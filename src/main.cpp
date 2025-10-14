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

#ifdef ENABLE_IMU_FUSION
#include "location/FusionLocationManager.h"
#endif

#ifdef ENABLE_BLE
#include "ble/BLEManager.h"
#include "ble/BLEDataProvider.h"
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

#ifdef ENABLE_IMU_FUSION
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
    
    // 获取GPS数据并更新到融合定位系统和统一数据管理
    if (air780eg.getGNSS().isDataValid()) {
        double lat = air780eg.getGNSS().getLatitude();
        double lng = air780eg.getGNSS().getLongitude();
        float alt = air780eg.getGNSS().getAltitude();
        float speed = air780eg.getGNSS().getSpeed() / 3.6f; // 转换为m/s
        float heading = air780eg.getGNSS().getCourse();
        uint8_t sats = air780eg.getGNSS().getSatelliteCount();
        float hdop = air780eg.getGNSS().getHDOP();
        
        // 更新到统一数据管理
        device.updateLocationData(lat, lng, alt, speed, heading, sats, hdop);
        
#ifdef ENABLE_IMU_FUSION
        // 更新到融合定位系统
        fusionLocationManager.updateWithGPS(lat, lng, speed, true);
#endif
    }
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

  //================ MadgwickAHRS融合定位初始化开始 ================
#ifdef ENABLE_IMU_FUSION
  Serial.println("[融合定位] 初始化MadgwickAHRS融合定位系统...");
  
  if (fusionLocationManager.begin(FUSION_LOCATION_INITIAL_LAT, FUSION_LOCATION_INITIAL_LNG)) {
    Serial.println("[融合定位] ✅ MadgwickAHRS融合定位系统初始化成功");
    fusionLocationManager.setDebug(FUSION_LOCATION_DEBUG_ENABLED);
    fusionLocationManager.setUpdateInterval(FUSION_LOCATION_UPDATE_INTERVAL);
    
    // 开始记录轨迹
    fusionLocationManager.startRecording();
    
    Serial.println("[融合定位] MadgwickAHRS摩托车惯导系统已启动");
  }
  else {
    Serial.println("[融合定位] ❌ MadgwickAHRS融合定位系统初始化失败");
  }
  
  // 配置兜底定位 - 测试用的短间隔
  fusionLocationManager.configureFallbackLocation(
    true,      // 启用兜底定位
    15000,     // GNSS信号丢失超时时间 15秒（测试用）
    60000,     // LBS定位间隔 1分钟（测试用）
    120000,    // WiFi定位间隔 120秒
    true       // 优先使用WiFi定位
  );
#endif
  //================ MadgwickAHRS融合定位初始化结束 ================

#ifdef ENABLE_BLE
  // BLE数据提供者设置融合定位管理器
  #ifdef ENABLE_IMU_FUSION
    bleDataProvider.setFusionManager(&fusionLocationManager);
    Serial.println("[BLE] 融合定位管理器已设置到BLE数据提供者");
  #endif
#endif

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
#ifdef ENABLE_IMU_FUSION
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
  
  // 更新系统状态到统一数据管理
  static unsigned long lastSystemUpdate = 0;
  if (millis() - lastSystemUpdate > 5000) { // 每5秒更新一次系统状态
    int signal_strength = 0;
#ifdef USE_AIR780EG_GSM
    if (air780eg.isInitialized()) {
      signal_strength = air780eg.getNetwork().getSignalStrength();
    }
#endif
    device.updateSystemData(signal_strength, millis()/1000, ESP.getFreeHeap());
    lastSystemUpdate = millis();
  }

#ifdef ENABLE_BLE
  // BLE数据更新
  bleDataProvider.update();
  
  // 如果有客户端连接，更新BLE特征值
  if (bleManager.isClientConnected()) {
    bleManager.updateGPSData(bleDataProvider.getGPSData());
    bleManager.updateBatteryData(bleDataProvider.getBatteryData());
    bleManager.updateIMUData(bleDataProvider.getIMUData());
    
    // 更新融合定位数据
    if (bleDataProvider.isFusionDataValid()) {
      bleManager.updateFusionData(bleDataProvider.getFusionData());
    }
    
    // 更新系统状态数据
    if (bleDataProvider.isSystemStatusValid()) {
      bleManager.updateSystemStatus(bleDataProvider.getSystemStatus());
    }
  }
  
  // BLE管理器更新
  bleManager.update();
#endif

  // 最小延迟，实现高频更新（约1000Hz）
  delay(1);
}