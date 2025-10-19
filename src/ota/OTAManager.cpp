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
    
    otaTopicCheck = "device/" + deviceId + "/ota/check";
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
            
            if (serverVersion != currentVersion) {
                logMessage("发现新版本: " + serverVersion);
                
                if (!getAutoUpgrade()) {
                    logMessage("自动升级已禁用，跳过升级");
                    reportStatus("auto_disabled", 0, "自动升级已禁用");
                    return;
                }
                
                if (checkUpgradeConditions()) {
                    currentStatus = OTA_DOWNLOADING;
                    reportStatus("downloading", 0, "开始下载固件");
                    
                    if (downloadAndInstall(downloadUrl)) {
                        currentStatus = OTA_SUCCESS;
                        reportStatus("success", 100, "升级成功，即将重启");
                        delay(2000);
                        ESP.restart();
                    } else {
                        currentStatus = OTA_FAILED;
                        reportStatus("failed", 0, "升级失败");
                    }
                } else {
                    reportStatus("conditions_not_met", 0, "升级条件不满足");
                }
            } else {
                reportStatus("up_to_date", 100, "已是最新版本");
            }
        }
    }
}

void OTAManager::checkForUpdates() {
    if (!air780eg || currentStatus != OTA_IDLE) return;
    
    currentStatus = OTA_CHECKING;
    
    DynamicJsonDocument doc(512);
    doc["device_id"] = deviceId;
    doc["current_version"] = currentVersion;
    doc["hardware_version"] = "esp32-air780eg";
    doc["timestamp"] = millis();
    
    String payload;
    serializeJson(doc, payload);
    
    if (mqttPublishCallback) {
        mqttPublishCallback(otaTopicCheck.c_str(), payload.c_str());
    }
    
    logMessage("发送版本检查请求");
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
                reportStatus("downloading", progress, "下载进度: " + String(progress) + "%");
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

void OTAManager::reportStatus(String status, int progress, String message) {
    DynamicJsonDocument doc(512);
    doc["device_id"] = deviceId;
    doc["status"] = status;
    doc["progress"] = progress;
    doc["message"] = message;
    doc["timestamp"] = millis();
    doc["version"] = currentVersion;
    
    String payload;
    serializeJson(doc, payload);
    
    if (mqttPublishCallback) {
        mqttPublishCallback(otaTopicStatus.c_str(), payload.c_str());
    }
}

void OTAManager::setMQTTPublishCallback(void (*callback)(const char*, const char*)) {
    mqttPublishCallback = callback;
}

void OTAManager::setAutoUpgrade(bool enabled) {
    Preferences prefs;
    prefs.begin("ota", false);
    prefs.putBool(OTA_AUTO_UPGRADE_KEY, enabled);
    prefs.end();
    logMessage("自动升级设置: " + String(enabled ? "启用" : "禁用"));
}

bool OTAManager::getAutoUpgrade() {
    Preferences prefs;
    prefs.begin("ota", true);
    bool autoUpgrade = prefs.getBool(OTA_AUTO_UPGRADE_KEY, OTA_DEFAULT_AUTO_UPGRADE);
    prefs.end();
    return autoUpgrade;
}

void OTAManager::logMessage(String message) {
    Serial.println("[OTAManager] " + message);
}
