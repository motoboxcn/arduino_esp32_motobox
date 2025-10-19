#include "OTAManager.h"
#include "config.h"
#include <Preferences.h>

OTAManager otaManager;

OTAManager::OTAManager() 
    : currentStatus(OTA_IDLE), 
      upgradeProgress(0),
      mqttPublishCallback(nullptr),
      air780eg(nullptr) {
    
    deviceId = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    currentVersion = String(FIRMWARE_VERSION);
    
    otaTopicCheck = "device/ota/check";
    otaTopicDownload = "device/" + deviceId + "/ota/download";
    otaTopicStatus = "device/" + deviceId + "/ota/status";
}

void OTAManager::begin(Air780EG* air780eg_instance) {
    air780eg = air780eg_instance;
    logMessage("OTA管理器初始化完成");
    logMessage("设备ID: " + deviceId);
    logMessage("当前版本: " + currentVersion);
}

void OTAManager::handleMQTTMessage(String topic, String payload) {
    if (topic == otaTopicCheck) {
        DynamicJsonDocument doc(1024);
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            String serverVersion = doc["latest_version"];
            String downloadUrl = doc["download_url"];
            
            logMessage("服务端版本: " + serverVersion + ", 当前版本: " + currentVersion);
            
            if (serverVersion != currentVersion) {
                logMessage("🔄 发现新版本，准备升级...");
                
                if (!getAutoUpgrade()) {
                    logMessage("❌ 自动升级已禁用");
                    currentStatus = OTA_IDLE; // 重置状态
                    return;
                }
                
                if (checkUpgradeConditions()) {
                    currentStatus = OTA_DOWNLOADING;
                    
                    if (downloadAndInstall(downloadUrl)) {
                        currentStatus = OTA_SUCCESS;
                        logMessage("✅ 升级成功，重启中...");
                        delay(1000);
                        ESP.restart();
                    } else {
                        currentStatus = OTA_FAILED;
                        logMessage("❌ 升级失败");
                    }
                } else {
                    logMessage("❌ 升级条件不满足");
                    currentStatus = OTA_IDLE; // 重置状态
                }
            } else {
                logMessage("✅ 固件已是最新版本，无需升级");
                currentStatus = OTA_IDLE; // 重置状态
            }
        } else {
            logMessage("❌ 解析服务端消息失败");
            currentStatus = OTA_IDLE; // 重置状态
        }
    }
}

void OTAManager::checkForUpdates() {
    if (!air780eg) return;
    
    if (currentStatus != OTA_IDLE) {
        logMessage("❌ OTA正在进行中，请等待完成后再试");
        return;
    }
    
    // 检查MQTT连接状态
    if (!air780eg->getMQTT().isConnected()) {
        logMessage("❌ MQTT未连接，无法检查更新");
        return;
    }
    
    logMessage("检查更新中...");
    currentStatus = OTA_CHECKING;
    checkStartTime = millis();
    
    logMessage("等待服务端retain消息...");
}

bool OTAManager::checkUpgradeConditions() {
    // 检查电池电量
    if (bat.getPercentage() < 90 && !bat.isCharging()) {
        logMessage("电池电量不足，需要≥90%或正在充电");
        return false;
    }
    
    // 检查可用空间
    size_t freeSpace = ESP.getFreeSketchSpace();
    if (freeSpace < 500000) { // 至少500KB
        logMessage("可用空间不足: " + String(freeSpace));
        return false;
    }
    
    return true;
}

bool OTAManager::downloadAndInstall(String url) {
    if (!air780eg) return false;
    
    logMessage("开始下载固件: " + url);
    
    bool success = air780eg->getHTTP().downloadFile(url, 
        [this](uint8_t* data, size_t size) -> bool {
            // 写入固件数据
            size_t written = Update.write(data, size);
            return written == size;
        },
        [this](int progress) {
            // 进度回调
            upgradeProgress = progress;
            if (progress % 20 == 0) {
                logMessage("下载进度: " + String(progress) + "%");
            }
        }
    );
    
    if (success && Update.end(true)) {
        logMessage("固件下载和安装完成");
        return true;
    } else {
        logMessage("固件安装失败: " + String(Update.errorString()));
        return false;
    }
}



void OTAManager::setMQTTPublishCallback(void (*callback)(const char*, const char*)) {
    mqttPublishCallback = callback;
}

void OTAManager::setAutoUpgrade(bool enabled) {
    Preferences prefs;
    if (!prefs.begin("ota", false)) {
        logMessage("无法打开NVS分区进行写入");
        return;
    }
    prefs.putBool(OTA_AUTO_UPGRADE_KEY, enabled);
    prefs.end();
    logMessage("自动升级设置: " + String(enabled ? "启用" : "禁用"));
}

bool OTAManager::getAutoUpgrade() {
    Preferences prefs;
    if (!prefs.begin("ota", true)) {
        logMessage("NVS分区未初始化，使用默认设置");
        return OTA_DEFAULT_AUTO_UPGRADE;
    }
    bool autoUpgrade = prefs.getBool(OTA_AUTO_UPGRADE_KEY, OTA_DEFAULT_AUTO_UPGRADE);
    prefs.end();
    return autoUpgrade;
}

void OTAManager::logMessage(String message) {
    Serial.println("[OTAManager] " + message);
}

void OTAManager::checkTimeout() {
    if (currentStatus == OTA_CHECKING && millis() - checkStartTime > 10000) {
        logMessage("❌ 检查超时，未收到服务端消息");
        currentStatus = OTA_IDLE;
    }
}
