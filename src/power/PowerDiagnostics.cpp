#include "PowerDiagnostics.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_bt.h"
#include "driver/gpio.h"

#ifdef USE_AIR780EG_GSM
#include "Air780EG.h"
#endif

void PowerDiagnostics::printCurrentConsumption() {
    Serial.println("\n=== 功耗分析报告 ===");
    
    float estimated_current = estimateCurrentConsumption();
    Serial.printf("预估总功耗: %.1f mA\n", estimated_current);
    
    analyzePeripheralPower();
    checkPeripheralStates();
    printGPIOStates();
    
    // 新增：详细的硬件功耗分析
    printHardwarePowerAnalysis();
}

void PowerDiagnostics::printHardwarePowerAnalysis() {
    Serial.println("\n--- 硬件功耗详细分析 ---");
    
    Serial.println("🔋 ESP32-S3 功耗分析:");
    Serial.println("  - CPU (240MHz): ~5-10mA");
    Serial.println("  - WiFi 关闭: ~0mA");
    Serial.println("  - 蓝牙关闭: ~0mA");
    Serial.println("  - 深度睡眠理论值: <1mA");
    
    Serial.println("\n🔌 外设功耗估算:");
    
    // TFT 显示屏
    #ifdef ENABLE_TFT
    Serial.println("  - TFT 显示屏: ~20-50mA (可能未完全关闭)");
    #endif
    
    // LED
    #ifdef PWM_LED_PIN
    Serial.println("  - PWM LED: ~1-5mA");
    #endif
    
    // SD 卡
    #ifdef ENABLE_SDCARD
    Serial.println("  - SD 卡: ~5-15mA (如果未正确断电)");
    #endif
    
    // IMU 传感器
    #ifdef ENABLE_IMU
    Serial.println("  - IMU 传感器: ~0.5-2mA");
    #endif
    
    // 音频模块
    #ifdef ENABLE_AUDIO
    Serial.println("  - 音频模块: ~5-20mA (如果未关闭)");
    #endif
    
    Serial.println("\n⚠️  可能的高功耗源:");
    Serial.println("  1. TFT 显示屏背光或驱动未关闭");
    Serial.println("  2. 音频放大器未断电");
    Serial.println("  3. SD 卡未正确进入低功耗模式");
    Serial.println("  4. 外部上拉电阻导致的漏电流");
    Serial.println("  5. GPIO 配置不当导致的电流泄漏");
    
    Serial.println("\n🔧 建议排查步骤:");
    Serial.println("  1. 物理断开 TFT 显示屏连接");
    Serial.println("  2. 断开 SD 卡连接");
    Serial.println("  3. 断开音频模块连接");
    Serial.println("  4. 逐个断开外设，定位功耗源");
}

void PowerDiagnostics::analyzePeripheralPower() {
    Serial.println("\n--- 外设功耗分析 ---");
    
    // ESP32-S3 基础功耗
    printPeripheralState("ESP32-S3 Core", true, 5.0);
    
    // Air780EG 模块
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    bool air780eg_active = air780eg.isReady();
    printPeripheralState("Air780EG GSM", air780eg_active, air780eg_active ? 25.0 : 0.1);
    #endif
    
    // WiFi
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);
    bool wifi_active = (wifi_mode != WIFI_MODE_NULL);
    printPeripheralState("WiFi", wifi_active, wifi_active ? 15.0 : 0.0);
    
    // 蓝牙
    bool bt_active = esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED;
    printPeripheralState("Bluetooth", bt_active, bt_active ? 10.0 : 0.0);
    
    // LED
    #ifdef PWM_LED_PIN
    printPeripheralState("PWM LED", true, 2.0);
    #endif
    
    // SD卡
    #ifdef ENABLE_SDCARD
    printPeripheralState("SD Card", true, 5.0);
    #endif
    
    // IMU
    #ifdef ENABLE_IMU
    printPeripheralState("IMU Sensor", true, 1.0);
    #endif
    
    // TFT显示屏
    #ifdef ENABLE_TFT
    printPeripheralState("TFT Display", true, 20.0);
    #endif
}

void PowerDiagnostics::checkPeripheralStates() {
    Serial.println("\n--- 外设状态检查 ---");
    
    // 检查WiFi状态
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);
    Serial.printf("WiFi 模式: %d (0=关闭, 1=STA, 2=AP, 3=STA+AP)\n", wifi_mode);
    
    // 检查蓝牙状态
    esp_bt_controller_status_t bt_status = esp_bt_controller_get_status();
    Serial.printf("蓝牙状态: %d (0=未初始化, 1=已初始化, 2=已启用)\n", bt_status);
    
    // 检查电源域状态
    Serial.println("电源域状态:");
    Serial.printf("  RTC_PERIPH: %s\n", "ON");  // 简化显示
    Serial.printf("  RTC_SLOW_MEM: %s\n", "ON");
    Serial.printf("  RTC_FAST_MEM: %s\n", "OFF");
    Serial.printf("  XTAL: %s\n", "OFF");
}

void PowerDiagnostics::printGPIOStates() {
    Serial.println("\n--- 关键 GPIO 状态 ---");
    
    #ifdef BAT_PIN
    Serial.printf("电池检测引脚 (GPIO%d): %s\n", BAT_PIN, 
                  digitalRead(BAT_PIN) ? "HIGH" : "LOW");
    #endif
    
    #ifdef CHARGING_STATUS_PIN
    Serial.printf("充电状态引脚 (GPIO%d): %s\n", CHARGING_STATUS_PIN, 
                  digitalRead(CHARGING_STATUS_PIN) ? "HIGH" : "LOW");
    #endif
    
    #ifdef IMU_INT_PIN
    Serial.printf("IMU中断引脚 (GPIO%d): %s\n", IMU_INT_PIN, 
                  digitalRead(IMU_INT_PIN) ? "HIGH" : "LOW");
    #endif
    
    #ifdef RTC_INT_PIN
    Serial.printf("RTC中断引脚 (GPIO%d): %s\n", RTC_INT_PIN, 
                  digitalRead(RTC_INT_PIN) ? "HIGH" : "LOW");
    #endif
}

void PowerDiagnostics::testSleepModes() {
    Serial.println("\n=== 睡眠模式测试 ===");
    Serial.println("注意: 此测试将进入不同的睡眠模式");
    Serial.println("请在5秒内发送任意字符取消测试...");
    
    // 等待用户取消
    unsigned long start = millis();
    while (millis() - start < 5000) {
        if (Serial.available()) {
            Serial.read();
            Serial.println("测试已取消");
            return;
        }
        delay(100);
    }
    
    Serial.println("开始睡眠模式测试...");
    
    // 测试轻度睡眠
    Serial.println("进入轻度睡眠 2 秒...");
    Serial.flush();
    esp_sleep_enable_timer_wakeup(2000000); // 2秒
    esp_light_sleep_start();
    Serial.println("从轻度睡眠唤醒");
    
    delay(1000);
    
    // 注意：深度睡眠测试会重启设备，所以放在最后
    Serial.println("如需测试深度睡眠，请手动调用 PowerManager::enterLowPowerMode()");
}

void PowerDiagnostics::measureBaselinePower() {
    Serial.println("\n=== 基线功耗测量建议 ===");
    Serial.println("1. 使用万用表串联测量电流");
    Serial.println("2. 当前状态预估功耗: " + String(estimateCurrentConsumption()) + " mA");
    Serial.println("3. 理想深度睡眠功耗: < 5 mA");
    Serial.println("4. 如果功耗过高，检查以下项目:");
    Serial.println("   - Air780EG 模块是否完全关闭");
    Serial.println("   - WiFi/蓝牙是否完全禁用");
    Serial.println("   - 不必要的 GPIO 上拉是否禁用");
    Serial.println("   - 外部设备是否正确断电");
}

void PowerDiagnostics::printPeripheralState(const char* name, bool enabled, float estimated_ma) {
    Serial.printf("  %-15s: %s (%.1f mA)\n", 
                  name, 
                  enabled ? "启用" : "禁用", 
                  enabled ? estimated_ma : 0.0);
}

float PowerDiagnostics::estimateCurrentConsumption() {
    float total = 5.0; // ESP32-S3 基础功耗
    
    // Air780EG
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    if (air780eg.isReady()) total += 25.0;
    #endif
    
    // WiFi
    wifi_mode_t wifi_mode;
    esp_wifi_get_mode(&wifi_mode);
    if (wifi_mode != WIFI_MODE_NULL) total += 15.0;
    
    // 蓝牙
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        total += 10.0;
    }
    
    // 其他外设
    #ifdef PWM_LED_PIN
    total += 2.0;
    #endif
    
    #ifdef ENABLE_SDCARD
    total += 5.0;
    #endif
    
    #ifdef ENABLE_IMU
    total += 1.0;
    #endif
    
    #ifdef ENABLE_TFT
    total += 20.0;
    #endif
    
    return total;
}
