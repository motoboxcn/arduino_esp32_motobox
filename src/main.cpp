/**
 * ESP32 MotoBox - 摩托车数据采集与显示系统
 *
 * 硬件：ESP32-S3
 * 版本：2.0.0
 *
 * 模式：
 * - MODE_ALLINONE: 独立模式，所有功能集成在一个设备上
 * - MODE_SERVER: 服务器模式，采集数据并通过BLE发送给客户端，同时通过MQTT发送到服务器
 * - MODE_CLIENT: 客户端模式，通过BLE接收服务器数据并显示
 */

#include "config.h"
#include "Arduino.h"
#include "config.h"
#include "power/PowerManager.h"
#include "led/LEDManager.h"
#include "device.h"
#include "Air780EG.h"
#include "utils/serialCommand.h"
#include "ota/SDCardOTA.h"
#include "SD/SDManager.h"
#include "SD/GPSLogger.h"

#ifdef BAT_PIN
#include "bat/BAT.h"
#endif

#ifdef RTC_INT_PIN
#include "power/ExternalPower.h"
#endif

#ifdef BTN_PIN
#include "btn/BTN.h"
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

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
#endif

#ifdef ENABLE_GPS_LOGGER
#include "SD/GPSLogger.h"
extern GPSLogger gpsLogger;
#endif

#ifdef ENABLE_AUDIO
#include "audio/AudioManager.h"
#endif

#include "version.h"
#ifdef BLE_CLIENT
#include "ble/ble_client.h"
#endif
#ifdef BLE_SERVER
#include "ble/ble_server.h"
#endif

// 仅在未定义DISABLE_TFT时包含TFT相关头文件
#ifdef ENABLE_TFT
#include "tft/TFT.h"
#endif

#include <SD.h> // SD卡库
#include <FS.h>

//============================= 全局变量 =============================

// RTC内存变量（深度睡眠后保持）
RTC_DATA_ATTR int bootCount = 0;

/**
 * 系统监控任务
 * 负责电源管理、LED状态、按钮处理
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

#ifdef BLE_SERVER
    bs.loop();
#endif

#ifdef BLE_CLIENT
    bc.loop();
#endif

#ifdef ENABLE_TFT
    tft_loop();
#endif

    // 电源管理
    powerManager.loop();

    // LED状态更新
    ledManager.loop();

#ifdef BAT_PIN
    bat.loop();
#endif

#ifdef BTN_PIN
    button.loop();
    BTN::handleButtonEvents();
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
    // SD卡状态监控
#ifdef ENABLE_SDCARD
    static unsigned long lastSDCheckTime = 0;
    unsigned long currentTime = millis();
    // 每10秒更新一次SD卡状态
    if (currentTime - lastSDCheckTime > 10000)
    {
      lastSDCheckTime = currentTime;
      if (sdManager.isInitialized())
      {
        device_state.sdCardFreeMB = sdManager.getFreeSpaceMB();
      }
    }
#endif

    delay(5);
  }
}

/**
 * 数据处理任务
 * 负责数据采集、发送和显示
 * 可以阻塞的任务，要和主任务区分分配的内核
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

#ifdef ENABLE_SDCARD
    // 数据记录到SD卡
    unsigned long currentTime = millis();

    // 记录GPS数据
    if (device_state.gnssReady && device_state.sdCardReady &&
        currentTime - lastGNSSRecordTime >= GPS_LOG_INTERVAL_MS)
    {
      lastGNSSRecordTime = currentTime;

      // 记录到SD卡（原有方法）
      sdManager.recordGPSData(air780eg.getGNSS().gnss_data);

#ifdef ENABLE_GPS_LOGGER
      // 使用GPS记录器记录数据
      if (air780eg.getGNSS().gnss_data.is_fixed)
      { // 有效GPS数据
        gpsLogger.logGPSData(air780eg.getGNSS().gnss_data);
      }
#endif
    }
#endif


#ifdef ENABLE_TFT
    // 显示屏更新
    tft_loop();
#endif

#ifdef ENABLE_COMPASS
    // 罗盘数据处理
    compass.loop();
#endif

    delay(10); // 增加延时，减少CPU占用
  } // for循环结束
}

#ifdef ENABLE_WIFI
// WiFi处理任务
void taskWiFi(void *parameter)
{
  Serial.println("[系统] WiFi任务启动");
  while (true)
  {
    if (wifiManager.getConfigMode())
    {
      wifiManager.loop();
    }
    delay(1); // 更短的延迟，提高响应性
  }
}
#endif

void setup()
{
  Serial.begin(115200);
  delay(1000);

  PreferencesUtils::init();

  bootCount++;
  Serial.println("[系统] 启动次数: " + String(bootCount));

  powerManager.begin();

  powerManager.printWakeupReason();

  //================ SD卡初始化开始 ================
#ifdef ENABLE_SDCARD
  if (sdManager.begin())
  {
    Serial.println("[SD] SD卡初始化成功");

    // 更新设备状态
    device_state.sdCardReady = true;
    device_state.sdCardSizeMB = sdManager.getTotalSpaceMB();
    device_state.sdCardFreeMB = sdManager.getFreeSpaceMB();

    // 保存设备信息
    sdManager.saveDeviceInfo();

    Serial.println("[SD] 设备信息已保存到SD卡");

    // 初始化SD卡OTA升级功能
    Serial.println("[SD] 初始化SD卡OTA升级");
    sdCardOTA.begin();

    // 检查并执行SD卡升级（如果需要）
    Serial.println("[SD] 检查SD卡升级");
    if (sdCardOTA.checkAndUpgrade())
    {
      Serial.println("[OTA] SD卡升级成功，设备将重启");
      // 如果升级成功，设备会自动重启，不会执行到这里
    }
    else
    {
      String error = sdCardOTA.getLastError();
      if (error.indexOf("未找到") < 0 && error.indexOf("不需要更新") < 0)
      {
        // 只有真正的错误才打印，文件不存在或不需要更新是正常情况
        Serial.println("[OTA] SD卡升级检查: " + error);
      }
    }

#ifdef ENABLE_GPS_LOGGER
    // 初始化GPS记录器
    if (gpsLogger.begin())
    {
      Serial.println("[GPS] GPS记录器初始化成功");
      // 自动开始GPS记录会话
      gpsLogger.startNewSession();
      Serial.println("[GPS] 自动开始GPS记录会话");
    }
    else
    {
      Serial.println("[GPS] GPS记录器初始化失败");
    }
#endif
  }
  else
  {
    Serial.println("[SD] SD卡初始化失败");
    device_state.sdCardReady = false;
  }
#endif
  //================ SD卡初始化结束 ================

  device.begin();

  //================ 融合定位初始化开始 ================
#ifdef ENABLE_FUSION_LOCATION
  Serial.println("[融合定位] 初始化融合定位系统...");
  // 使用EKF车辆模型算法，适合摩托车应用
  if (fusionLocationManager.begin(FUSION_EKF_VEHICLE, FUSION_LOCATION_INITIAL_LAT, FUSION_LOCATION_INITIAL_LNG))
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
    45000,     // WiFi定位间隔 45秒（测试用）
    true       // 优先使用WiFi定位
);

#endif
  //================ 融合定位初始化结束 ================

  // 创建任务
  xTaskCreate(taskSystem, "TaskSystem", 1024 * 15, NULL, 1, NULL);
  xTaskCreate(taskDataProcessing, "TaskData", 1024 * 15, NULL, 2, NULL);
#ifdef ENABLE_WIFI
  xTaskCreate(taskWiFi, "TaskWiFi", 1024 * 15, NULL, 3, NULL);
#endif

#ifdef ENABLE_AUDIO
  Serial.println("音频功能: ✅ 编译时已启用");
#else
  Serial.println("音频功能: ❌ 编译时未启用");
#endif
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

  // 最小延迟，实现高频更新（约1000Hz）
  delay(1);
}