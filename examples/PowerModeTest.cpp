/**
 * 功耗模式管理测试示例
 * 
 * 此示例展示如何使用PowerModeManager进行功耗模式管理
 */

#include "power/PowerModeManager.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("=== 功耗模式管理测试 ===");
    
    // 初始化功耗模式管理器
    powerModeManager.begin();
    
    // 显示初始状态
    powerModeManager.printCurrentStatus();
    
    // 测试手动模式切换
    Serial.println("\n--- 测试手动模式切换 ---");
    
    // 切换到运动模式
    Serial.println("切换到运动模式...");
    powerModeManager.setMode(POWER_MODE_SPORT);
    delay(2000);
    
    // 切换到基本模式
    Serial.println("切换到基本模式...");
    powerModeManager.setMode(POWER_MODE_BASIC);
    delay(2000);
    
    // 切换到正常模式
    Serial.println("切换到正常模式...");
    powerModeManager.setMode(POWER_MODE_NORMAL);
    delay(2000);
    
    // 显示所有模式配置
    powerModeManager.printModeConfigs();
    
    // 启用自动模式切换
    Serial.println("启用自动模式切换...");
    powerModeManager.enableAutoModeSwitch(true);
    
    Serial.println("测试完成，进入主循环...");
}

void loop() {
    // 功耗模式管理器主循环
    powerModeManager.loop();
    
    // 每30秒显示一次状态
    static unsigned long lastStatusTime = 0;
    if (millis() - lastStatusTime > 30000) {
        lastStatusTime = millis();
        Serial.println("\n--- 定期状态报告 ---");
        powerModeManager.printCurrentStatus();
    }
    
    // 模拟运动检测变化
    static unsigned long lastMotionSimulation = 0;
    static bool simulateMotion = false;
    
    if (millis() - lastMotionSimulation > 60000) { // 每分钟切换一次
        lastMotionSimulation = millis();
        simulateMotion = !simulateMotion;
        
        if (simulateMotion) {
            Serial.println("模拟运动开始...");
            // 这里可以模拟IMU运动数据
        } else {
            Serial.println("模拟运动停止...");
        }
    }
    
    delay(100);
}

/**
 * 串口命令测试
 * 
 * 在串口监视器中输入以下命令进行测试：
 * 
 * power.status     - 查看当前状态
 * power.sport      - 切换到运动模式
 * power.normal     - 切换到正常模式
 * power.basic      - 切换到基本模式
 * power.config     - 查看所有配置
 * power.auto.on    - 启用自动切换
 * power.auto.off   - 禁用自动切换
 * power.eval       - 手动触发评估
 * power.help       - 查看帮助
 */