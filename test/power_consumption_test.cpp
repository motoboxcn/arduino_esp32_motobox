/*
 * 功耗排查测试脚本
 * 用于逐步关闭外设，定位高功耗源
 */

#include <Arduino.h>
#include "power/PowerDiagnostics.h"
#include "power/PowerManager.h"

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("=== ESP32-S3 MotoBox 功耗排查测试 ===");
    Serial.println("当前休眠功耗: ~20mA");
    Serial.println("目标功耗: <5mA");
    Serial.println();
    
    // 打印详细的功耗分析
    PowerDiagnostics::printCurrentConsumption();
    
    Serial.println("\n=== 逐步排查建议 ===");
    Serial.println("请按以下步骤物理断开外设连接，每次测量功耗：");
    Serial.println();
    
    Serial.println("步骤 1: 断开 TFT 显示屏");
    Serial.println("  - 断开 TFT 的 VCC 和背光连接");
    Serial.println("  - 预期功耗降低: 20-50mA");
    Serial.println();
    
    Serial.println("步骤 2: 断开 SD 卡");
    Serial.println("  - 移除 SD 卡或断开 SD 卡模块电源");
    Serial.println("  - 预期功耗降低: 5-15mA");
    Serial.println();
    
    Serial.println("步骤 3: 断开音频模块");
    Serial.println("  - 断开音频放大器和扬声器");
    Serial.println("  - 预期功耗降低: 5-20mA");
    Serial.println();
    
    Serial.println("步骤 4: 断开 LED");
    Serial.println("  - 断开所有 LED 连接");
    Serial.println("  - 预期功耗降低: 1-5mA");
    Serial.println();
    
    Serial.println("步骤 5: 检查外部上拉电阻");
    Serial.println("  - 检查 I2C、SPI 等总线的上拉电阻");
    Serial.println("  - 强上拉可能导致额外功耗");
    Serial.println();
    
    Serial.println("=== 软件排查 ===");
    Serial.println("如果硬件排查后功耗仍高，检查：");
    Serial.println("1. GPIO 配置是否正确");
    Serial.println("2. 时钟是否完全关闭");
    Serial.println("3. 外设驱动是否有后台任务");
    Serial.println();
    
    Serial.println("30秒后自动进入休眠测试...");
    delay(30000);
    
    Serial.println("进入休眠模式，请测量功耗...");
    powerManager.testSafeEnterSleep();
}

void loop() {
    // 不会执行到这里，因为设备会进入深度睡眠
}

/*
 * 编译和使用说明：
 * 
 * 1. 将此文件重命名为 main.cpp 替换原文件
 * 2. 编译并烧录到设备
 * 3. 打开串口监视器查看分析报告
 * 4. 按照建议逐步断开外设
 * 5. 每次断开后测量功耗变化
 * 
 * 常见高功耗源：
 * - TFT 显示屏背光: 20-100mA
 * - 音频放大器: 10-50mA  
 * - SD 卡: 5-20mA
 * - 强上拉电阻: 1-10mA
 * - GPIO 漏电流: 0.1-5mA
 */
