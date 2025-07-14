#include "SDCardOTA.h"
#include "config.h"

// 全局实例
SDCardOTA sdCardOTA;

SDCardOTA::SDCardOTA() 
    : firmwareFileName("firmware.bin"),
      versionFileName("version.txt"),
      upgrading(false),
      progress(0) {
    
    currentVersion = String(FIRMWARE_VERSION);
}

void SDCardOTA::begin() {
    logMessage("SD卡OTA升级模块初始化");
    logMessage("当前版本: " + currentVersion);
    logMessage("固件文件: " + firmwareFileName);
    logMessage("版本文件: " + versionFileName);
}

bool SDCardOTA::checkAndUpgrade() {
    logMessage("开始检查SD卡升级");
    
    // 检查SD卡是否可用
    if (!SD.begin()) {
        lastError = "SD卡未检测到或初始化失败";
        logMessage("❌ " + lastError);
        return false;
    }
    
    // 检查固件文件是否存在
    if (!checkFileExists("/" + firmwareFileName)) {
        lastError = "SD卡中未找到固件文件: " + firmwareFileName;
        logMessage("❌ " + lastError);
        return false;
    }
    
    // 读取版本信息
    String sdVersion = readVersionFromSD();
    if (sdVersion.isEmpty()) {
        lastError = "无法读取SD卡版本信息";
        logMessage("❌ " + lastError);
        return false;
    }
    
    logMessage("SD卡固件版本: " + sdVersion);
    logMessage("当前固件版本: " + currentVersion);
    
    // 检查版本是否需要更新
    if (!checkVersionNewer(sdVersion)) {
        lastError = "SD卡版本不需要更新 (当前: " + currentVersion + ", SD卡: " + sdVersion + ")";
        logMessage("ℹ️ " + lastError);
        return false;
    }
    
    // 检查电池电量
    if (!checkBatteryLevel()) {
        lastError = "电池电量不足，需要≥90%才能升级";
        logMessage("❌ " + lastError);
        playUpgradeSound(4); // 错误音
        return false;
    }
    
    logMessage("✅ 所有升级条件满足，开始升级");
    playUpgradeSound(1); // 开始升级音
    
    // 执行升级
    return performUpgrade();
}

bool SDCardOTA::checkBatteryLevel() {
    // 从设备状态获取电池电量
    extern device_state_t device_state;
    int batteryLevel = device_state.battery_percentage;
    
    logMessage("当前电池电量: " + String(batteryLevel) + "%");
    logMessage("充电状态: " + String(device_state.is_charging ? "充电中" : "未充电"));
    
    return batteryLevel >= OTA_BATTERY_MIN_LEVEL;
}

bool SDCardOTA::checkVersionNewer(String newVersion) {
    return compareVersions(newVersion, currentVersion) > 0;
}

bool SDCardOTA::checkFileExists(String filePath) {
    return SD.exists(filePath);
}

int SDCardOTA::compareVersions(String version1, String version2) {
    // 移除v前缀和+后缀
    version1.replace("v", "");
    version2.replace("v", "");
    
    int plusIndex1 = version1.indexOf('+');
    if (plusIndex1 > 0) version1 = version1.substring(0, plusIndex1);
    
    int plusIndex2 = version2.indexOf('+');
    if (plusIndex2 > 0) version2 = version2.substring(0, plusIndex2);
    
    // 分割版本号
    int major1 = 0, minor1 = 0, patch1 = 0;
    int major2 = 0, minor2 = 0, patch2 = 0;
    
    sscanf(version1.c_str(), "%d.%d.%d", &major1, &minor1, &patch1);
    sscanf(version2.c_str(), "%d.%d.%d", &major2, &minor2, &patch2);
    
    if (major1 != major2) return major1 - major2;
    if (minor1 != minor2) return minor1 - minor2;
    return patch1 - patch2;
}

String SDCardOTA::readVersionFromSD() {
    File versionFile = SD.open("/" + versionFileName);
    if (versionFile) {
        String version = versionFile.readString();
        versionFile.close();
        version.trim();
        return version;
    }
    return "";
}

bool SDCardOTA::performUpgrade() {
    upgrading = true;
    progress = 0;
    
    logMessage("🔄 开始从SD卡升级固件");
    playUpgradeSound(2); // 升级进行中音
    
    File firmware = SD.open("/" + firmwareFileName);
    if (!firmware) {
        lastError = "无法打开固件文件";
        logMessage("❌ " + lastError);
        playUpgradeSound(4); // 错误音
        upgrading = false;
        return false;
    }
    
    size_t fileSize = firmware.size();
    logMessage("固件文件大小: " + String(fileSize) + " 字节");
    
    // 开始OTA升级
    if (!Update.begin(fileSize)) {
        lastError = "OTA升级初始化失败: " + String(Update.errorString());
        logMessage("❌ " + lastError);
        firmware.close();
        playUpgradeSound(4); // 错误音
        upgrading = false;
        return false;
    }
    
    // 写入固件数据
    size_t written = 0;
    uint8_t buffer[1024];
    
    while (firmware.available()) {
        size_t readBytes = firmware.read(buffer, sizeof(buffer));
        written += Update.write(buffer, readBytes);
        
        // 更新进度
        progress = (written * 100) / fileSize;
        
        // 每20%播放一次进度提示
        if (progress % 20 == 0 && progress > 0) {
            logMessage("升级进度: " + String(progress) + "%");
            playUpgradeSound(5); // 进度音
        }
        
        // 看门狗喂狗
        yield();
    }
    
    firmware.close();
    
    // 完成升级
    if (Update.end(true)) {
        logMessage("✅ 固件升级成功！写入 " + String(written) + " 字节");
        playUpgradeSound(3); // 成功音
        
        delay(2000); // 等待2秒让用户听到成功提示音
        
        logMessage("🔄 设备即将重启...");
        ESP.restart();
        return true;
    } else {
        lastError = "固件升级失败: " + String(Update.errorString());
        logMessage("❌ " + lastError);
        playUpgradeSound(4); // 错误音
        upgrading = false;
        return false;
    }
}

void SDCardOTA::playUpgradeSound(int type) {
#ifdef BUZZER_PIN
    int buzzerPin = BUZZER_PIN;
    
    switch (type) {
        case 1: // 开始升级 - 3声短促
            for (int i = 0; i < 3; i++) {
                tone(buzzerPin, 1000, 200);
                delay(300);
            }
            break;
            
        case 2: // 升级进行中 - 2声中等
            for (int i = 0; i < 2; i++) {
                tone(buzzerPin, 800, 500);
                delay(600);
            }
            break;
            
        case 3: // 升级成功 - 上升音调
            tone(buzzerPin, 800, 200);
            delay(100);
            tone(buzzerPin, 1000, 200);
            delay(100);
            tone(buzzerPin, 1200, 300);
            break;
            
        case 4: // 升级失败 - 长声低音
            tone(buzzerPin, 400, 1000);
            break;
            
        case 5: // 进度提示 - 单声短促
            tone(buzzerPin, 1200, 100);
            break;
            
        default:
            tone(buzzerPin, 1000, 200);
            break;
    }
    
    noTone(buzzerPin);
#endif
}

void SDCardOTA::logMessage(String message) {
    Serial.println("[SDCardOTA] " + message);
}
