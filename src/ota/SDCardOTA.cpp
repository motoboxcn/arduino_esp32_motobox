#include "SDCardOTA.h"
#include "config.h"
#include <vector>

// 全局实例
SDCardOTA sdCardOTA;

SDCardOTA::SDCardOTA() 
    : upgrading(false),
      progress(0) {
    
    currentVersion = String(FIRMWARE_VERSION);
}

void SDCardOTA::begin() {
    logMessage("SD卡OTA升级模块初始化");
    logMessage("当前版本: " + currentVersion);
    logMessage("支持的固件文件格式: motobox_v*.bin");
    logMessage("示例: motobox_v4.1.0.bin, motobox_v4.2.0.bin");
}

bool SDCardOTA::checkAndUpgrade() {
    logMessage("开始检查SD卡升级");
    
    // 检查SD卡是否可用
    if (!SD.begin()) {
        lastError = "SD卡未检测到或初始化失败";
        logMessage("❌ " + lastError);
        return false;
    }
    
    // 扫描SD卡中的固件文件
    if (!scanFirmwareFiles()) {
        lastError = "SD卡中未找到有效的固件文件";
        logMessage("❌ " + lastError);
        return false;
    }
    
    // 打印找到的固件列表
    printFirmwareList();
    
    // 获取最新版本的固件
    FirmwareInfo latestFirmware = getLatestFirmware();
    
    if (!latestFirmware.isValid) {
        lastError = "未找到有效的固件文件";
        logMessage("❌ " + lastError);
        return false;
    }
    
    logMessage("选择的固件: " + latestFirmware.fileName + " (版本: " + latestFirmware.version + ")");
    
    // 检查版本是否需要更新
    if (!checkVersionNewer(latestFirmware.version)) {
        lastError = "固件版本不需要更新 (当前: " + currentVersion + ", 最新: " + latestFirmware.version + ")";
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
    
    logMessage("✅ 所有升级条件满足，开始升级到版本: " + latestFirmware.version);
    playUpgradeSound(1); // 开始升级音
    
    // 执行升级
    return performUpgrade(latestFirmware);
}

bool SDCardOTA::scanFirmwareFiles() {
    firmwareList.clear();
    
    File root = SD.open("/");
    if (!root) {
        logMessage("❌ 无法打开SD卡根目录");
        return false;
    }
    
    logMessage("🔍 扫描SD卡中的固件文件...");
    
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        
        if (!file.isDirectory() && isFirmwareFile(fileName)) {
            FirmwareInfo firmware;
            firmware.fileName = fileName;
            firmware.fileSize = file.size();
            firmware.isValid = false;
            
            // 从文件名提取版本号（只支持motobox格式）
            firmware.version = extractVersionFromFileName(fileName);
            
            if (!firmware.version.isEmpty()) {
                firmware.isValid = true;
                firmwareList.push_back(firmware);
                logMessage("✅ 找到固件: " + fileName + " (版本: " + firmware.version + ", 大小: " + String(firmware.fileSize) + " 字节)");
            } else {
                logMessage("⚠️ 跳过固件: " + fileName + " (不是motobox格式或无法确定版本号)");
            }
        }
        
        file = root.openNextFile();
    }
    
    root.close();
    
    logMessage("📊 扫描完成，找到 " + String(firmwareList.size()) + " 个有效固件文件");
    return firmwareList.size() > 0;
}

bool SDCardOTA::isFirmwareFile(String fileName) {
    fileName.toLowerCase();
    
    // 只支持 motobox_v*.bin 格式的固件文件
    return (fileName.endsWith(".bin") && fileName.startsWith("motobox_v"));
}

String SDCardOTA::extractVersionFromFileName(String fileName) {
    // 从 motobox_v4.1.0.bin 格式中提取版本号
    
    String lowerFileName = fileName;
    lowerFileName.toLowerCase();
    
    if (!lowerFileName.startsWith("motobox_v")) {
        return "";
    }
    
    int vIndex = fileName.indexOf("_v");
    if (vIndex >= 0) {
        int startIndex = vIndex + 2; // 跳过 "_v"
        int endIndex = fileName.lastIndexOf(".bin");
        
        if (endIndex > startIndex) {
            String version = fileName.substring(startIndex, endIndex);
            // 添加v前缀（如果没有的话）
            if (!version.startsWith("v")) {
                version = "v" + version;
            }
            return version;
        }
    }
    
    return "";
}

String SDCardOTA::extractVersionFromFile(String filePath) {
    File versionFile = SD.open(filePath);
    if (versionFile) {
        String version = versionFile.readString();
        versionFile.close();
        version.trim();
        return version;
    }
    return "";
}

FirmwareInfo SDCardOTA::getLatestFirmware() {
    FirmwareInfo latest;
    latest.isValid = false;
    
    for (const auto& firmware : firmwareList) {
        if (!firmware.isValid) continue;
        
        if (!latest.isValid || compareVersions(firmware.version, latest.version) > 0) {
            latest = firmware;
        }
    }
    
    return latest;
}

void SDCardOTA::printFirmwareList() {
    if (firmwareList.empty()) {
        logMessage("📋 未找到固件文件");
        return;
    }
    
    logMessage("📋 找到的固件文件列表:");
    for (size_t i = 0; i < firmwareList.size(); i++) {
        const auto& firmware = firmwareList[i];
        String sizeStr = String(firmware.fileSize / 1024.0, 1) + " KB";
        if (firmware.fileSize >= 1024 * 1024) {
            sizeStr = String(firmware.fileSize / (1024.0 * 1024.0), 1) + " MB";
        }
        
        logMessage("  " + String(i + 1) + ". " + firmware.fileName + 
                  " (版本: " + firmware.version + ", 大小: " + sizeStr + ")");
    }
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

bool SDCardOTA::performUpgrade(const FirmwareInfo& firmware) {
    upgrading = true;
    progress = 0;
    
    logMessage("🔄 开始升级固件: " + firmware.fileName);
    logMessage("📦 固件版本: " + firmware.version);
    logMessage("📏 文件大小: " + String(firmware.fileSize) + " 字节");
    
    playUpgradeSound(2); // 升级进行中音
    
    File firmwareFile = SD.open("/" + firmware.fileName);
    if (!firmwareFile) {
        lastError = "无法打开固件文件: " + firmware.fileName;
        logMessage("❌ " + lastError);
        playUpgradeSound(4); // 错误音
        upgrading = false;
        return false;
    }
    
    // 开始OTA升级
    if (!Update.begin(firmware.fileSize)) {
        lastError = "OTA升级初始化失败: " + String(Update.errorString());
        logMessage("❌ " + lastError);
        firmwareFile.close();
        playUpgradeSound(4); // 错误音
        upgrading = false;
        return false;
    }
    
    // 写入固件数据
    size_t written = 0;
    uint8_t buffer[1024];
    
    while (firmwareFile.available()) {
        size_t readBytes = firmwareFile.read(buffer, sizeof(buffer));
        written += Update.write(buffer, readBytes);
        
        // 更新进度
        progress = (written * 100) / firmware.fileSize;
        
        // 每20%播放一次进度提示
        if (progress % 20 == 0 && progress > 0) {
            logMessage("升级进度: " + String(progress) + "% (" + String(written) + "/" + String(firmware.fileSize) + " 字节)");
            playUpgradeSound(5); // 进度音
        }
        
        // 看门狗喂狗
        yield();
    }
    
    firmwareFile.close();
    
    // 完成升级
    if (Update.end(true)) {
        logMessage("✅ 固件升级成功！");
        logMessage("📊 升级统计:");
        logMessage("  - 源文件: " + firmware.fileName);
        logMessage("  - 版本: " + currentVersion + " → " + firmware.version);
        logMessage("  - 写入字节: " + String(written) + "/" + String(firmware.fileSize));
        
        playUpgradeSound(3); // 成功音
        
        delay(2000); // 等待2秒让用户听到成功提示音
        
        logMessage("🔄 设备即将重启到新版本...");
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
