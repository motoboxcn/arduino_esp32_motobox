#include "PowerManager.h"
#include "esp_wifi.h"
#include "driver/adc.h"
#include "driver/gpio.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

#ifdef USE_AIR780EG_GSM
#include "Air780EG.h"
#endif

#ifdef ENABLE_LED
#include "led/PWMLED.h"
#endif

#ifdef ENABLE_SDCARD
#include "SD/SDManager.h"
#endif

// 初始化静态变量
#ifdef ENABLE_SLEEP
RTC_DATA_ATTR bool PowerManager::sleepEnabled = true;
#else
RTC_DATA_ATTR bool PowerManager::sleepEnabled = false;
#endif

PowerManager powerManager;

PowerManager::PowerManager()
{
    powerState = POWER_STATE_NORMAL;
    sleepTimeSec = 300; // 默认5分钟
    lastMotionTime = 0;
}

void PowerManager::begin()
{
    Serial.println("[电源管理] 初始化开始");
    
    // 从存储读取休眠时间
    sleepTimeSec = PreferencesUtils::loadSleepTime();
    if (sleepTimeSec == 0) {
        sleepTimeSec = get_device_state()->sleep_time;
    }
    
    // 处理唤醒事件
    handleWakeup();
    
    Serial.printf("[电源管理] 休眠时间: %lu 秒\n", sleepTimeSec);
    Serial.printf("[电源管理] 休眠功能: %s\n", sleepEnabled ? "启用" : "禁用");
}

void PowerManager::handleWakeup()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            {
                Serial.println("[电源管理] IMU运动唤醒");
                #ifdef ENABLE_IMU
                extern IMU imu;
                imu.restoreFromDeepSleep();
                if (imu.checkWakeOnMotionEvent()) {
                    Serial.println("[电源管理] ✅ 确认运动唤醒事件");
                }
                #endif
                break;
            }
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[电源管理] 定时器唤醒");
            break;
        default:
            Serial.println("[电源管理] 首次启动");
            break;
    }
    
    lastMotionTime = millis();
}

void PowerManager::loop()
{
    #ifndef ENABLE_SLEEP
    return; // 编译时禁用睡眠功能
    #endif
    
    static unsigned long lastCheck = 0;
    unsigned long now = millis();
    
    // 每200ms检查一次
    if (now - lastCheck < 200) {
        return;
    }
    lastCheck = now;
    
    // 检查车辆状态（如果启用）
    handleVehicleStateChange();
    if (isVehicleStarted()) {
        lastMotionTime = now; // 车辆启动时保持活跃
        Serial.println("[电源管理] 车辆启动中，不进入睡眠！");
        return;
    }
    
    // 检查IMU运动
    #ifdef ENABLE_IMU
    if (imu.detectMotion()) {
        lastMotionTime = now;
        Serial.println("[电源管理] 检测到运动，不进入睡眠！");
        return;
    }
    #endif
    
    // 检查是否需要进入睡眠
    if (isDeviceIdle()) {
        Serial.printf("[电源管理] 设备静止超过%lu秒，进入睡眠\n", sleepTimeSec);
        enterLowPowerMode();
    }
}

bool PowerManager::isDeviceIdle()
{
    return (millis() - lastMotionTime) > (sleepTimeSec * 1000);
}

void PowerManager::setSleepTime(unsigned long seconds)
{
    sleepTimeSec = seconds;
    get_device_state()->sleep_time = seconds;
    lastMotionTime = millis(); // 重置计时
    PreferencesUtils::saveSleepTime(sleepTimeSec);
    Serial.printf("[电源管理] 休眠时间已更新: %lu 秒\n", sleepTimeSec);
}

unsigned long PowerManager::getSleepTime() const
{
    return sleepTimeSec;
}

void PowerManager::printWakeupReason()
{
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    switch (wakeup_reason) {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("[系统] IMU运动唤醒");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("[系统] 定时器唤醒");
            break;
        default:
            Serial.printf("[系统] 唤醒原因: %d\n", wakeup_reason);
            break;
    }
}

void PowerManager::enterLowPowerMode()
{
    #ifndef ENABLE_SLEEP
    Serial.println("[电源管理] 休眠功能已禁用");
    return;
    #endif
    
    if (!sleepEnabled) {
        Serial.println("[电源管理] 休眠功能未启用");
        return;
    }
    
    Serial.println("[电源管理] 进入低功耗模式...");
    powerState = POWER_STATE_PREPARING_SLEEP;
    
    // 配置唤醒源
    if (!configureWakeupSources()) {
        Serial.println("[电源管理] 唤醒源配置失败");
        powerState = POWER_STATE_NORMAL;
        return;
    }
    
    // 关闭外设
    disablePeripherals();
    
    // 配置电源域
    configurePowerDomains();
    
    Serial.println("[电源管理] 💤 进入深度睡眠");
    Serial.flush();
    delay(100);
    
    // 进入深度睡眠
    esp_deep_sleep_start();
}

bool PowerManager::configureWakeupSources()
{
    Serial.println("[电源管理] 配置唤醒源...");
    
    // 禁用所有唤醒源
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    
    // 配置IMU运动唤醒
    #if defined(ENABLE_IMU) && defined(IMU_INT_PIN)
    if (IMU_INT_PIN >= 0 && rtc_gpio_is_valid_gpio((gpio_num_t)IMU_INT_PIN)) {
        // 初始化RTC GPIO
        rtc_gpio_init((gpio_num_t)IMU_INT_PIN);
        rtc_gpio_set_direction((gpio_num_t)IMU_INT_PIN, RTC_GPIO_MODE_INPUT_ONLY);
        
        // 配置上拉（GPIO39除外）
        if (IMU_INT_PIN != 39) {
            rtc_gpio_pullup_en((gpio_num_t)IMU_INT_PIN);
            rtc_gpio_pulldown_dis((gpio_num_t)IMU_INT_PIN);
        }
        
        // 配置EXT0唤醒
        esp_err_t ret = esp_sleep_enable_ext0_wakeup((gpio_num_t)IMU_INT_PIN, 0);
        if (ret != ESP_OK) {
            Serial.printf("[电源管理] EXT0配置失败: %s\n", esp_err_to_name(ret));
            return false;
        }
        
        // 配置IMU为深度睡眠模式
        extern IMU imu;
        if (!imu.configureForDeepSleep()) {
            Serial.println("[电源管理] IMU深度睡眠配置失败");
            return false;
        }
        
        Serial.printf("[电源管理] ✅ IMU唤醒配置成功 (GPIO%d)\n", IMU_INT_PIN);
    }
    #endif
    
    // 配置定时器唤醒（1小时备用）
    const uint64_t BACKUP_WAKEUP_TIME = 60 * 60 * 1000000ULL;
    esp_sleep_enable_timer_wakeup(BACKUP_WAKEUP_TIME);
    Serial.println("[电源管理] ✅ 定时器唤醒配置成功");
    
    return true;
}

void PowerManager::disablePeripherals()
{
    Serial.println("[电源管理] 关闭外设...");
    
    // 1. 关闭 Air780EG 模块（最大功耗外设）
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    Serial.println("[电源管理] 关闭 Air780EG 模块...");
    air780eg.powerOff();
    delay(1000); // 等待模块完全关闭
    #endif
    
    // 2. 关闭WiFi和蓝牙
    Serial.println("[电源管理] 关闭 WiFi 和蓝牙...");
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    esp_wifi_deinit();
    btStop();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    
    // 3. 关闭 LED 和 PWM
    Serial.println("[电源管理] 关闭 LED...");
    #ifdef ENABLE_LED
    extern PWMLED pwmLed;
    pwmLed.setBrightness(0);
    pwmLed.deinit(); // 完全关闭PWM
    #endif
    
    // 4. 关闭 SD 卡
    #ifdef ENABLE_SDCARD
    Serial.println("[电源管理] 关闭 SD 卡...");
    extern SDManager sdManager;
    sdManager.end();
    #endif
    
    // 5. 关闭 TFT 显示屏
    #ifdef ENABLE_TFT
    Serial.println("[电源管理] 关闭 TFT 显示屏...");
    // 添加 TFT 关闭代码
    #endif
    
    // 6. 关闭串口（除了调试串口）
    #ifdef GPS_RX_PIN
    Serial2.end();
    #endif
    
    // 7. 关闭 ADC
    Serial.println("[电源管理] 关闭 ADC...");
    adc_power_release();
    
    // 8. 关闭不必要的 GPIO 上拉
    Serial.println("[电源管理] 配置 GPIO 低功耗模式...");
    configureGPIOForSleep();
    
    Serial.println("[电源管理] ✅ 所有外设已关闭");
    Serial.flush();
    delay(100);
}

void PowerManager::configurePowerDomains()
{
    Serial.println("[电源管理] 配置电源域...");
    // 配置电源域以最大化省电
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_ON);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_OFF);
    
    // 关闭不必要的电源域
    esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
    Serial.println("[电源管理] ✅ 电源域配置完成");
}

void PowerManager::configureGPIOForSleep()
{
    Serial.println("[电源管理] 配置 GPIO 低功耗模式...");
    
    // 配置所有未使用的 GPIO 为输入模式，禁用上拉下拉
    for (int gpio = 0; gpio <= 48; gpio++) {
        // 跳过特殊引脚
        if (gpio == 1 || gpio == 3) continue;  // UART0 TX/RX
        if (gpio == 19 || gpio == 20) continue; // USB D-/D+
        if (gpio == 43 || gpio == 44) continue; // UART0 TX/RX (ESP32-S3)
        
        // 跳过正在使用的引脚
        #ifdef IMU_INT_PIN
        if (gpio == IMU_INT_PIN) continue;
        #endif
        #ifdef RTC_INT_PIN
        if (gpio == RTC_INT_PIN) continue;
        #endif
        #ifdef BAT_PIN
        if (gpio == BAT_PIN) continue;
        #endif
        #ifdef CHARGING_STATUS_PIN
        if (gpio == CHARGING_STATUS_PIN) continue;
        #endif
        
        // 检查是否为有效的 GPIO
        if (GPIO_IS_VALID_GPIO(gpio)) {
            gpio_config_t io_conf = {};
            io_conf.intr_type = GPIO_INTR_DISABLE;
            io_conf.mode = GPIO_MODE_INPUT;
            io_conf.pin_bit_mask = (1ULL << gpio);
            io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
            io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
            gpio_config(&io_conf);
        }
    }
    
    Serial.println("[电源管理] ✅ GPIO 低功耗配置完成");
}

#ifdef RTC_INT_PIN
bool PowerManager::isVehicleStarted()
{
    return (digitalRead(RTC_INT_PIN) == LOW);
}

void PowerManager::handleVehicleStateChange()
{
    static bool lastVehicleState = false;
    static bool firstCheck = true;
    bool currentVehicleState = isVehicleStarted();
    
    if (firstCheck) {
        lastVehicleState = currentVehicleState;
        firstCheck = false;
        return;
    }
    
    if (currentVehicleState != lastVehicleState) {
        if (currentVehicleState) {
            Serial.println("[电源管理] 🚗 车辆启动");
            lastMotionTime = millis();
        } else {
            Serial.println("[电源管理] 🚗 车辆关闭");
        }
        lastVehicleState = currentVehicleState;
    }
}
#endif
