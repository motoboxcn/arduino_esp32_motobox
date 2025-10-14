#include "device.h"
#include "utils/DebugUtils.h"
#include "utils/DataCollector.h"
#include "config.h"
#include "imu/qmi8658.h"
// GSM模块包含
#ifdef USE_AIR780EG_GSM
#include "Air780EG.h"
#elif defined(USE_ML307_GSM)
#include "net/Ml307Mqtt.h"
extern Ml307Mqtt ml307Mqtt;
#endif



extern const VersionInfo &getVersionInfo();

device_state_t device_state;
state_changes_t state_changes;
void print_device_info()
{
    // 如果休眠准备中的时候不打印
    if (powerManager.getPowerState() == POWER_STATE_PREPARING_SLEEP)
    {
        return;
    }
    Serial.println("Device Info:");
    Serial.printf("Device ID: %s\n", device_state.device_id.c_str());
    Serial.printf("Sleep Time: %d\n", device_state.sleep_time);
    Serial.printf("Firmware Version: %s\n", device_state.device_firmware_version);
    Serial.printf("Hardware Version: %s\n", device_state.device_hardware_version);
    Serial.printf("WiFi Ready: %d\n", device_state.telemetry.modules.wifi_ready);
    Serial.printf("BLE Ready: %d\n", device_state.telemetry.modules.ble_ready);
    Serial.printf("Battery Voltage: %d\n", device_state.telemetry.system.battery_voltage);
    Serial.printf("Battery Percentage: %d\n", device_state.telemetry.system.battery_percentage);
    Serial.printf("Charging: %d\n", device_state.telemetry.system.is_charging);
    Serial.printf("External Power: %d\n", device_state.telemetry.system.external_power);
    Serial.printf("GSM Ready: %d\n", device_state.telemetry.modules.gsm_ready);
    Serial.printf("GNSS Ready: %d\n", device_state.telemetry.modules.gnss_ready);
    Serial.printf("IMU Ready: %d\n", device_state.telemetry.modules.imu_ready);
    Serial.printf("Compass Ready: %d\n", device_state.telemetry.modules.compass_ready);
    Serial.printf("SD Card Ready: %d\n", device_state.telemetry.modules.sd_ready);
    if (device_state.telemetry.modules.sd_ready)
    {
        Serial.printf("SD Card Size: %llu MB\n", device_state.sdCardSizeMB);
        Serial.printf("SD Card Free: %llu MB\n", device_state.sdCardFreeMB);
    }
    Serial.printf("Audio Ready: %d\n", device_state.telemetry.modules.audio_ready);
    Serial.println("--------------------------------");
}

String Device::get_device_id()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char device_id[13];
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(device_id);
}

device_state_t *get_device_state()
{
    return &device_state;
}

void set_device_state(device_state_t *state)
{
    device_state = *state;
}

// 生成精简版设备状态JSON
// fw: 固件版本, hw: 硬件版本, wifi/ble/gps/imu/compass: 各模块状态, bat_v: 电池电压, bat_pct: 电池百分比, is_charging: 充电状态, ext_power: 外部电源状态, sd: SD卡状态
String device_state_to_json(device_state_t *state)
{
    StaticJsonDocument<256> doc; // 精简后更小即可
    doc["fw"] = device_state.device_firmware_version;
    doc["hw"] = device_state.device_hardware_version;
    doc["wifi"] = device_state.telemetry.modules.wifi_ready;
    doc["ble"] = device_state.telemetry.modules.ble_ready;
    doc["gsm"] = device_state.telemetry.modules.gsm_ready;
    doc["gnss"] = device_state.telemetry.modules.gnss_ready;
    doc["imu"] = device_state.telemetry.modules.imu_ready;
    doc["compass"] = device_state.telemetry.modules.compass_ready;
    doc["bat_v"] = device_state.telemetry.system.battery_voltage;
    doc["bat_pct"] = device_state.telemetry.system.battery_percentage;
    doc["is_charging"] = device_state.telemetry.system.is_charging;
    doc["ext_power"] = device_state.telemetry.system.external_power;
    doc["sd"] = device_state.telemetry.modules.sd_ready;
    if (device_state.telemetry.modules.sd_ready)
    {
        doc["sd_size"] = device_state.sdCardSizeMB;
        doc["sd_free"] = device_state.sdCardFreeMB;
    }
    doc["audio"] = device_state.telemetry.modules.audio_ready;
    return doc.as<String>();
}

// 添加包装函数
String getDeviceStatusJSON()
{
    return device_state_to_json(&device_state);
}

String getLocationJSON()
{
#ifdef ENABLE_IMU_FUSION
    // 走惯导估算获取位置信息
    extern FusionLocationManager fusionLocationManager;
    return fusionLocationManager.getPositionJSON();
#else
    // 融合定位功能被禁用，使用Air780EG基础定位
    return air780eg.getGNSS().getLocationJSON();
#endif
}

void mqttMessageCallback(const String &topic, const String &payload)
{
#ifndef DISABLE_MQTT
    Serial.println("=== MQTT消息回调触发 ===");
    Serial.printf("收到消息 [%s]: %s\n", topic.c_str(), payload.c_str());
    Serial.printf("主题长度: %d, 负载长度: %d\n", topic.length(), payload.length());

    // 解析JSON
    StaticJsonDocument<256> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.println("❌ 解析JSON失败: " + String(error.c_str()));
        Serial.println("原始负载: " + payload);
        Serial.println("=== MQTT消息回调结束 (JSON解析失败) ===");
        return;
    }

    Serial.println("✅ JSON解析成功");

    // 解析JSON
    // {"cmd": "enter_config"}
    const char *cmd = doc["cmd"];
    if (cmd)
    {
        Serial.printf("收到命令: %s\n", cmd);
        Serial.println("开始执行命令处理...");
#ifdef ENABLE_WIFI

        if (strcmp(cmd, "enter_ap_mode") == 0)
        {
            wifiManager.enterAPMode();
        }
        else if (strcmp(cmd, "exit_ap_mode") == 0)
        {
            wifiManager.exitAPMode();
        }
        else if (strcmp(cmd, "reset_wifi") == 0)
        {
            wifiManager.reset();
        }
#endif
        if (strcmp(cmd, "set_sleep_time") == 0)
        {
            // {"cmd": "set_sleep_time", "sleep_time": 300}
            int sleepTime = doc["sleep_time"].as<int>();
            if (sleepTime > 0)
            {
                powerManager.setSleepTime(sleepTime);
            }
            else
            {
                Serial.println("休眠时间不能小于0");
            }
        }
        // 设置数据采集模式
        else if (strcmp(cmd, "set_data_mode") == 0)
        {
            // {"cmd": "set_data_mode", "mode": "normal"} 或 {"mode": "sport"}
            const char* mode = doc["mode"];
            if (mode != nullptr)
            {
                if (strcmp(mode, "normal") == 0)
                {
                    dataCollector.setMode(MODE_NORMAL);
                    Serial.println("数据采集模式已设置为: 正常模式(5秒)");
                }
                else if (strcmp(mode, "sport") == 0)
                {
                    dataCollector.setMode(MODE_SPORT);
                    Serial.println("数据采集模式已设置为: 运动模式(1秒)");
                }
                else
                {
                    Serial.println("❌ 无效的数据采集模式: " + String(mode));
                }
            }
            else
            {
                Serial.println("❌ 缺少mode参数");
            }
        }
        // 控制数据采集
        else if (strcmp(cmd, "data_collection") == 0)
        {
            // {"cmd": "data_collection", "action": "start"} 或 {"action": "stop"}
            const char* action = doc["action"];
            if (action != nullptr)
            {
                if (strcmp(action, "start") == 0)
                {
                    dataCollector.startCollection();
                    Serial.println("数据采集已启动");
                }
                else if (strcmp(action, "stop") == 0)
                {
                    dataCollector.stopCollection();
                    Serial.println("数据采集已停止");
                }
                else if (strcmp(action, "stats") == 0)
                {
                    dataCollector.printStats();
                }
                else
                {
                    Serial.println("❌ 无效的操作: " + String(action));
                }
            }
            else
            {
                Serial.println("❌ 缺少action参数");
            }
        }
        // reboot
        else if (strcmp(cmd, "reboot") == 0 || strcmp(cmd, "restart") == 0)
        {
            Serial.println("重启设备");
            ESP.restart();
        }
        Serial.println("✅ 命令处理完成");
    }
    else
    {
        Serial.println("❌ 消息中未找到cmd字段");
    }
    Serial.println("=== MQTT消息回调结束 ===");
#endif
}

void mqttConnectionCallback(bool connected)
{
#ifndef DISABLE_MQTT
    Serial.printf("MQTT连接状态: %s\n", connected ? "已连接" : "断开");
    if (connected)
    {
        // 订阅控制主题
        air780eg.getMQTT().subscribe("vehicle/v1/" + device_state.device_id + "/ctrl/#", 1);
    }
    else
    {
        Serial.println("MQTT连接失败");
        Serial.println("❌ MQTT连接断开，订阅功能不可用");
    }
#endif
}

Device device;

Device::Device()
{
}

void Device::begin()
{
    // 从getVersionInfo()获取版本信息
    const VersionInfo &versionInfo = getVersionInfo();
    device_state.device_id = get_device_id();
    device_state.device_firmware_version = versionInfo.firmware_version;
    device_state.device_hardware_version = versionInfo.hardware_version;

    // 检查是否从深度睡眠唤醒
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    bool isWakeFromDeepSleep = (wakeup_reason != ESP_SLEEP_WAKEUP_UNDEFINED);

    // 打印启动信息
    if (isWakeFromDeepSleep)
    {
        Serial.println("[系统] 从深度睡眠唤醒，重新初始化系统...");
    }
    else
    {
        Serial.printf("[系统] 系统正常启动，硬件版本: %s, 固件版本: %s, 编译时间: %s\n", getVersionInfo().hardware_version,
                      getVersionInfo().firmware_version, getVersionInfo().build_time);
    }

#ifdef BAT_PIN
    // bat.setDebug(true);
    bat.begin();
#endif

#ifdef RTC_INT_PIN
    externalPower.setDebug(true);
    externalPower.begin();
#endif

    // LED初始化
#if defined(PWM_LED_PIN) || defined(LED_PIN)
    ledManager.begin();
#endif

#ifdef GPS_WAKE_PIN
    rtc_gpio_hold_dis((gpio_num_t)GPS_WAKE_PIN);
    Serial.println("[电源管理] GPS_WAKE_PIN 保持已解除");
#endif

#ifdef ENABLE_BLE
    // 初始化BLE管理器（使用设备ID）
    Serial.println("[BLE] 开始初始化BLE系统...");
    if (bleManager.begin(device_state.device_id)) {
        device_state.telemetry.modules.ble_ready = true; // 初始状态为就绪
        Serial.println("[BLE] ✅ BLE系统初始化成功");
    } else {
        device_state.telemetry.modules.ble_ready = false;
        Serial.println("[BLE] ❌ BLE系统初始化失败");
    }
    
    // 初始化BLE数据提供者
    bleDataProvider.begin();
    Serial.println("[BLE] ✅ BLE数据提供者初始化完成");
#endif

    // 统一初始化I2C设备（IMU和Compass共用同一个I2C总线）
    Serial.println("[I2C] 开始初始化I2C设备...");
    Serial.printf("[I2C] 引脚配置 - SDA:%d, SCL:%d\n", IIC_SDA_PIN, IIC_SCL_PIN);

    // 首先初始化共享I2C管理器
    if (!initSharedI2C(IIC_SDA_PIN, IIC_SCL_PIN))
    {
        Serial.println("[I2C] ❌ 共享I2C初始化失败");
    }
    else
    {
        Serial.println("[I2C] ✅ 共享I2C初始化成功");
    }

    // 添加延时，避免电流峰值
    delay(100);

#ifdef ENABLE_IMU
    Serial.println("[IMU] 开始初始化IMU系统...");
    Serial.printf("[IMU] 引脚配置 - INT:%d\n", IMU_INT_PIN);

    try
    {
        imu.begin();
        device_state.telemetry.modules.imu_ready = true; // 设置IMU状态为就绪
        Serial.println("[IMU] ✅ IMU系统初始化成功，状态已设置为就绪");
    }
    catch (...)
    {
        device_state.telemetry.modules.imu_ready = false;
        Serial.println("[IMU] ❌ IMU系统初始化异常");
    }

    // 添加延时，避免电流峰值
    delay(100);
#endif

#ifdef ENABLE_COMPASS
    Serial.println("[Compass] 开始初始化指南针系统...");
    try
    {
        compass.begin();
        Serial.println("[Compass] ✅ 指南针系统初始化成功");
    }
    catch (...)
    {
        Serial.println("[Compass] ❌ 指南针系统初始化异常");
    }

    // 添加延时，避免电流峰值
    delay(100);
#endif

    // 如果是从深度睡眠唤醒，检查唤醒原因
    if (isWakeFromDeepSleep)
    {
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[系统] IMU运动唤醒检测到，记录运动事件");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[系统] 定时器唤醒，检查系统状态");
            break;
        default:
            break;
        }
    }

#ifndef ENABLE_IMU
    device_state.imuReady = false;
    Serial.println("[IMU] IMU功能未启用 (ENABLE_IMU未定义)");
#endif

#ifdef ENABLE_GSM
#ifdef USE_AIR780EG_GSM
    // Air780EG模块初始化已在main.cpp中完成
    Serial.println("[Device] 使用Air780EG模块");
#elif defined(USE_ML307_GSM)
    // ML307模块初始化
    Serial.println("[Device] 使用ML307模块");
    ml307_at.setDebug(true);
    ml307_at.begin(115200);
#endif
    initializeGSM();
#endif
    Serial.println("GPS初始化已延迟到任务中!");

#ifdef ENABLE_BLE
    // 设置BLE数据源（在所有模块初始化完成后）
    Serial.println("[BLE] 设置数据源...");
    bleDataProvider.setDataSources(&air780eg, &imu, &bat);
    Serial.println("[BLE] ✅ 数据源设置完成");
#endif
}

// 通知特定状态变化
void notify_state_change(const char *state_name, const char *old_value, const char *new_value)
{
    Serial.printf("[状态变化] %s: %s -> %s\n", state_name, old_value, new_value);
}

// 更新设备状态并检查变化
void update_device_state()
{
    static device_state_t last_state;

    // 检查电池状态变化
    if (device_state.telemetry.system.battery_percentage != last_state.telemetry.system.battery_percentage)
    {
        notify_state_change("电池电量",
                            String(last_state.telemetry.system.battery_percentage).c_str(),
                            String(device_state.telemetry.system.battery_percentage).c_str());
        state_changes.battery_changed = true;

        // LED显示现在由LEDManager根据充电状态自动处理
        // 只有在电池电量极低时才显示红色警告
        if (device_state.telemetry.system.battery_percentage <= 10)
        {
            ledManager.setLEDState(LED_BLINK_FAST, LED_COLOR_RED, 20);
        }
        // 其他情况让LEDManager自动处理充电状态显示
    }

    // 检查外部电源状态变化
    if (device_state.telemetry.system.external_power != last_state.telemetry.system.external_power)
    {
        notify_state_change("外部电源",
                            last_state.telemetry.system.external_power ? "已连接" : "未连接",
                            device_state.telemetry.system.external_power ? "已连接" : "未连接");
        state_changes.external_power_changed = true;
    }

    // 检查网络状态变化 - 根据模式区分
#ifdef ENABLE_WIFI
    if (device_state.telemetry.modules.wifi_ready != last_state.telemetry.modules.wifi_ready)
    {
        notify_state_change("WiFi连接",
                            last_state.telemetry.modules.wifi_ready ? "已连接" : "未连接",
                            device_state.telemetry.modules.wifi_ready ? "已连接" : "未连接");
        state_changes.wifi_changed = true;
    }
#endif

    // 检查BLE状态变化
    if (device_state.telemetry.modules.ble_ready != last_state.telemetry.modules.ble_ready)
    {
        notify_state_change("BLE连接",
                            last_state.telemetry.modules.ble_ready ? "已连接" : "未连接",
                            device_state.telemetry.modules.ble_ready ? "已连接" : "未连接");
        state_changes.ble_changed = true;
    }

#ifdef ENABLE_GSM
    if (device_state.telemetry.modules.gsm_ready != last_state.telemetry.modules.gsm_ready)
    {
        notify_state_change("GSM状态",
                            last_state.telemetry.modules.gsm_ready ? "就绪" : "未就绪",
                            device_state.telemetry.modules.gsm_ready ? "就绪" : "未就绪");
        state_changes.gsm_changed = true;
    }
#endif

    // 检查IMU状态变化
    if (device_state.telemetry.modules.imu_ready != last_state.telemetry.modules.imu_ready)
    {
        notify_state_change("IMU状态",
                            last_state.telemetry.modules.imu_ready ? "就绪" : "未就绪",
                            device_state.telemetry.modules.imu_ready ? "就绪" : "未就绪");
        state_changes.imu_changed = true;
    }

    // 检查罗盘状态变化
    if (device_state.telemetry.modules.compass_ready != last_state.telemetry.modules.compass_ready)
    {
        notify_state_change("罗盘状态",
                            last_state.telemetry.modules.compass_ready ? "就绪" : "未就绪",
                            device_state.telemetry.modules.compass_ready ? "就绪" : "未就绪");
        state_changes.compass_changed = true;
    }

    // 检查休眠时间变化
    if (device_state.sleep_time != last_state.sleep_time)
    {
        notify_state_change("休眠时间",
                            String(last_state.sleep_time).c_str(),
                            String(device_state.sleep_time).c_str());
        state_changes.sleep_time_changed = true;
    }

    // 检查LED模式变化
    if (device_state.led_mode != last_state.led_mode)
    {
        notify_state_change("LED模式",
                            String(last_state.led_mode).c_str(),
                            String(device_state.led_mode).c_str());
        state_changes.led_mode_changed = true;
    }

    // 检查SD卡状态变化
    if (device_state.telemetry.modules.sd_ready != last_state.telemetry.modules.sd_ready)
    {
        notify_state_change("SD卡状态",
                            last_state.telemetry.modules.sd_ready ? "就绪" : "未就绪",
                            device_state.telemetry.modules.sd_ready ? "就绪" : "未就绪");
        state_changes.sdcard_changed = true;
    }

    // 检查音频状态变化
    if (device_state.telemetry.modules.audio_ready != last_state.telemetry.modules.audio_ready)
    {
        notify_state_change("音频状态",
                            last_state.telemetry.modules.audio_ready ? "就绪" : "未就绪",
                            device_state.telemetry.modules.audio_ready ? "就绪" : "未就绪");
        state_changes.audio_changed = true;
    }

    // 更新上一次状态
    last_state = device_state;

    // 重置状态变化标志
    state_changes = {0};
}

void device_loop()
{
    // Implementation of device_loop function
}

void Device::initializeGSM()
{
//================ GSM模块初始化开始 ================
#ifdef USE_AIR780EG_GSM
    Serial.println("[GSM] 初始化Air780EG模块...");
    Serial.printf("[GSM] 引脚配置 - RX:%d, TX:%d, EN:%d\n", GSM_RX_PIN, GSM_TX_PIN, GSM_EN);
    // 设置日志级别 (可选)
#if AIR780EG_LOG_VERBOSE_ENABLED == true
    Air780EG::setLogLevel(AIR780EG_LOG_VERBOSE);
#else
    Air780EG::setLogLevel(AIR780EG_LOG_INFO);
#endif
    // 配置Air780EG功能
    Air780EGConfig config;
    config.enableGSM = true;
    config.enableMQTT = true;
    config.enableGNSS = ENABLE_GNSS_LOCATION;
    config.enableFallbackLocation = ENABLE_FALLBACK_LOCATION;
    
    while (!air780eg.begin(&Serial1, 115200, GSM_RX_PIN, GSM_TX_PIN, GSM_EN, config))
    {
        Serial.println("[GSM] ❌ Air780EG基础初始化失败");
        device_state.telemetry.modules.gsm_ready = false;
        delay(1000);
    }
    Serial.println("[GSM] ✅ Air780EG基础初始化成功");
    device_state.telemetry.modules.gsm_ready = true;

#ifdef DISABLE_MQTT
    Serial.println("MQTT功能已禁用");
#else
    initializeMQTT();
#endif

#endif
    //================ GSM模块初始化结束 ================
}

bool Device::initializeMQTT()
{

#if (defined(ENABLE_WIFI) || defined(ENABLE_GSM))
    Serial.println("🔄 开始MQTT初始化...");

#ifdef USE_AIR780EG_GSM

    // 配置MQTT连接参数
    Air780EGMQTTConfig config;
    config.server = MQTT_BROKER;
    config.port = MQTT_PORT;
    config.client_id = MQTT_CLIENT_ID_PREFIX + device_state.device_hardware_version + "_" + device_state.device_id;
    config.username = MQTT_USERNAME;
    config.password = MQTT_PASSWORD;
    config.keepalive = 60;
    config.clean_session = true;
    // 初始化MQTT模块
    if (!air780eg.getMQTT().begin(config))
    {
        Serial.println("Failed to initialize MQTT module!");
        return false;
    }

    // 设置消息回调函数
    air780eg.getMQTT().setMessageCallback(mqttMessageCallback);

    // 设置连接状态回调
    air780eg.getMQTT().setConnectionCallback(mqttConnectionCallback);

    // 添加统一遥测任务
    air780eg.getMQTT().addScheduledTask("telemetry", "vehicle/v1/" + device_state.device_id + "/telemetry", 
        []() { return device.getCombinedTelemetryJSON(); }, MQTT_GPS_PUBLISH_INTERVAL, 0, false);

    // // 连接到MQTT服务器
    // if (!air780eg.getMQTT().connect())
    //     Serial.println("Failed to start MQTT connection, will retry later!");

#elif defined(USE_ML307_GSM)
    Serial.println("连接方式: ML307 4G网络");
#elif defined(ENABLE_WIFI)
    Serial.println("连接方式: WiFi网络");
#else
    Serial.println("连接方式: 未定义");
    return false;
#endif

#endif
    return false;
}

// 数据更新方法实现
void Device::updateLocationData(double lat, double lng, float alt, float speed, 
                               float heading, uint8_t sats, float hdop) {
    device_state.telemetry.location.lat = lat;
    device_state.telemetry.location.lng = lng;
    device_state.telemetry.location.altitude = alt;
    device_state.telemetry.location.speed = speed;
    device_state.telemetry.location.heading = heading;
    device_state.telemetry.location.satellites = sats;
    device_state.telemetry.location.hdop = hdop;
    device_state.telemetry.location.valid = true;
    device_state.telemetry.location.timestamp = millis();
}

void Device::updateIMUData(float ax, float ay, float az, float gx, float gy, float gz,
                          float roll, float pitch, float yaw) {
    device_state.telemetry.sensors.imu.accel_x = ax;
    device_state.telemetry.sensors.imu.accel_y = ay;
    device_state.telemetry.sensors.imu.accel_z = az;
    device_state.telemetry.sensors.imu.gyro_x = gx;
    device_state.telemetry.sensors.imu.gyro_y = gy;
    device_state.telemetry.sensors.imu.gyro_z = gz;
    device_state.telemetry.sensors.imu.roll = roll;
    device_state.telemetry.sensors.imu.pitch = pitch;
    device_state.telemetry.sensors.imu.yaw = yaw;
    device_state.telemetry.sensors.imu.valid = true;
    device_state.telemetry.sensors.imu.timestamp = millis();
}

void Device::updateCompassData(float heading, float mx, float my, float mz) {
    device_state.telemetry.sensors.compass.heading = heading;
    device_state.telemetry.sensors.compass.mag_x = mx;
    device_state.telemetry.sensors.compass.mag_y = my;
    device_state.telemetry.sensors.compass.mag_z = mz;
    device_state.telemetry.sensors.compass.valid = true;
    device_state.telemetry.sensors.compass.timestamp = millis();
}

void Device::updateBatteryData(int voltage, int percentage, bool charging, bool ext_power) {
    device_state.telemetry.system.battery_voltage = voltage;
    device_state.telemetry.system.battery_percentage = percentage;
    device_state.telemetry.system.is_charging = charging;
    device_state.telemetry.system.external_power = ext_power;
}

void Device::updateSystemData(int signal, uint32_t uptime, uint32_t free_heap) {
    device_state.telemetry.system.signal_strength = signal;
    device_state.telemetry.system.uptime = uptime;
    device_state.telemetry.system.free_heap = free_heap;
}

void Device::updateModuleStatus(const char* module, bool ready) {
    if (strcmp(module, "wifi") == 0) {
        device_state.telemetry.modules.wifi_ready = ready;
    } else if (strcmp(module, "ble") == 0) {
        device_state.telemetry.modules.ble_ready = ready;
    } else if (strcmp(module, "gsm") == 0) {
        device_state.telemetry.modules.gsm_ready = ready;
    } else if (strcmp(module, "gnss") == 0) {
        device_state.telemetry.modules.gnss_ready = ready;
    } else if (strcmp(module, "imu") == 0) {
        device_state.telemetry.modules.imu_ready = ready;
    } else if (strcmp(module, "compass") == 0) {
        device_state.telemetry.modules.compass_ready = ready;
    } else if (strcmp(module, "sd") == 0) {
        device_state.telemetry.modules.sd_ready = ready;
    } else if (strcmp(module, "audio") == 0) {
        device_state.telemetry.modules.audio_ready = ready;
    }
}

String Device::getCombinedTelemetryJSON() {
    DynamicJsonDocument doc(2048);
    
    // 设备信息
    doc["device_id"] = device_state.device_id;
    doc["timestamp"] = millis();
    doc["firmware"] = device_state.device_firmware_version;
    doc["hardware"] = device_state.device_hardware_version;
    doc["power_mode"] = device_state.power_mode;
    
    // 位置数据
    if (device_state.telemetry.location.valid) {
        JsonObject location = doc.createNestedObject("location");
        location["lat"] = device_state.telemetry.location.lat;
        location["lng"] = device_state.telemetry.location.lng;
        location["alt"] = device_state.telemetry.location.altitude;
        location["speed"] = device_state.telemetry.location.speed;
        location["course"] = device_state.telemetry.location.heading;
        location["satellites"] = device_state.telemetry.location.satellites;
        location["hdop"] = device_state.telemetry.location.hdop;
        location["timestamp"] = device_state.telemetry.location.timestamp;
    }
    
    // 传感器数据
    JsonObject sensors = doc.createNestedObject("sensors");
    
    // IMU数据
    if (device_state.telemetry.sensors.imu.valid) {
        JsonObject imu = sensors.createNestedObject("imu");
        imu["accel_x"] = device_state.telemetry.sensors.imu.accel_x;
        imu["accel_y"] = device_state.telemetry.sensors.imu.accel_y;
        imu["accel_z"] = device_state.telemetry.sensors.imu.accel_z;
        imu["gyro_x"] = device_state.telemetry.sensors.imu.gyro_x;
        imu["gyro_y"] = device_state.telemetry.sensors.imu.gyro_y;
        imu["gyro_z"] = device_state.telemetry.sensors.imu.gyro_z;
        imu["roll"] = device_state.telemetry.sensors.imu.roll;
        imu["pitch"] = device_state.telemetry.sensors.imu.pitch;
        imu["yaw"] = device_state.telemetry.sensors.imu.yaw;
        imu["timestamp"] = device_state.telemetry.sensors.imu.timestamp;
    }
    
    // 罗盘数据
    if (device_state.telemetry.sensors.compass.valid) {
        JsonObject compass = sensors.createNestedObject("compass");
        compass["heading"] = device_state.telemetry.sensors.compass.heading;
        compass["mag_x"] = device_state.telemetry.sensors.compass.mag_x;
        compass["mag_y"] = device_state.telemetry.sensors.compass.mag_y;
        compass["mag_z"] = device_state.telemetry.sensors.compass.mag_z;
        compass["timestamp"] = device_state.telemetry.sensors.compass.timestamp;
    }
    
    // 系统状态
    JsonObject system = doc.createNestedObject("system");
    system["battery"] = device_state.telemetry.system.battery_voltage;
    system["battery_pct"] = device_state.telemetry.system.battery_percentage;
    system["charging"] = device_state.telemetry.system.is_charging;
    system["external_power"] = device_state.telemetry.system.external_power;
    system["signal"] = device_state.telemetry.system.signal_strength;
    system["uptime"] = device_state.telemetry.system.uptime;
    system["free_heap"] = device_state.telemetry.system.free_heap;
    
    // 模块状态
    JsonObject modules = doc.createNestedObject("modules");
    modules["wifi"] = device_state.telemetry.modules.wifi_ready;
    modules["ble"] = device_state.telemetry.modules.ble_ready;
    modules["gsm"] = device_state.telemetry.modules.gsm_ready;
    modules["gnss"] = device_state.telemetry.modules.gnss_ready;
    modules["imu"] = device_state.telemetry.modules.imu_ready;
    modules["compass"] = device_state.telemetry.modules.compass_ready;
    modules["sd"] = device_state.telemetry.modules.sd_ready;
    modules["audio"] = device_state.telemetry.modules.audio_ready;
    
    // SD卡信息
    if (device_state.telemetry.modules.sd_ready) {
        JsonObject storage = doc.createNestedObject("storage");
        storage["size_mb"] = device_state.sdCardSizeMB;
        storage["free_mb"] = device_state.sdCardFreeMB;
    }
    
    String json;
    serializeJson(doc, json);
    return json;
}