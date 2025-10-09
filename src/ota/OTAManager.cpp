#include "OTAManager.h"
#include "config.h"

// 全局实例
OTAManager otaManager;

OTAManager::OTAManager() 
    : currentStatus(OTA_IDLE), 
      upgradeProgress(0),
      mqttPublishCallback(nullptr) {
    
    // 获取设备ID和当前版本
    deviceId = "ESP32_" + String((uint32_t)ESP.getEfuseMac(), HEX);
    currentVersion = String(FIRMWARE_VERSION);
    
    // 设置MQTT主题
    otaTopicCheck = "device/" + deviceId + "/ota/check";
    otaTopicDownload = "device/" + deviceId + "/ota/download";
    otaTopicStatus = "device/" + deviceId + "/ota/status";
}

void OTAManager::begin() {
    logMessage("OTA管理器初始化开始");
    logMessage("设备ID: " + deviceId);
    logMessage("当前版本: " + currentVersion);
    logMessage("最低电池要求: " + String(OTA_BATTERY_MIN_LEVEL) + "%");
    
    // 启动时检查在线升级
    logMessage("OTA管理器已就绪，等待MQTT连接后检查更新");
}

void OTAManager::setupMQTTOTA() {
    logMessage("设置MQTT OTA订阅");
    // 这里需要在MQTT连接后调用订阅函数
    // 实际的订阅操作需要在MQTT客户端中实现
}

void OTAManager::handleMQTTMessage(String topic, String payload) {
    if (topic == otaTopicCheck) {
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            logMessage("MQTT消息解析失败: " + String(error.c_str()));
            return;
        }
        
        String serverVersion = doc["latest_version"];
        String downloadUrl = doc["download_url"];
        bool forceUpdate = doc["force_update"] | false;
        String releaseNotes = doc["release_notes"] | "";
        
        logMessage("收到版本检查响应: " + serverVersion);
        if (!releaseNotes.isEmpty()) {
            logMessage("更新说明: " + releaseNotes);
        }
        
        UpgradeCondition condition = checkUpgradeConditions(serverVersion);
        
        if ((condition.versionNewer || forceUpdate) && condition.batteryOK) {
            logMessage("检测到在线更新，电池电量充足，开始升级");
            startOnlineDownload(downloadUrl, serverVersion);
        } else {
            String message = "在线升级条件不满足: " + condition.message;
            logMessage(message);
            reportStatus("conditions_not_met", 0, condition.message);
        }
    }
}

void OTAManager::checkForUpdates() {
    if (currentStatus != OTA_IDLE) {
        logMessage("升级进行中，跳过检查");
        return;
    }
    
    logMessage("检查在线更新...");
    
    DynamicJsonDocument doc(512);
    doc["device_id"] = deviceId;
    doc["current_version"] = currentVersion;
    doc["hardware_version"] = String(HARDWARE_VERSION);
    doc["timestamp"] = millis();
    
    String message;
    serializeJson(doc, message);
    
    if (mqttPublishCallback) {
        mqttPublishCallback(otaTopicCheck.c_str(), message.c_str());
        logMessage("发送版本检查请求");
    } else {
        logMessage("MQTT回调未设置，无法发送版本检查请求");
    }
}

UpgradeCondition OTAManager::checkUpgradeConditions(String newVersion) {
    UpgradeCondition condition;
    condition.batteryOK = checkBatteryLevel();
    condition.versionNewer = newVersion.isEmpty() ? false : checkVersionNewer(newVersion, currentVersion);
    condition.fileExists = true; // MQTT升级不需要检查本地文件
    condition.spaceAvailable = checkAvailableSpace(1024 * 1024); // 1MB最小空间
    
    String message = "";
    if (!condition.batteryOK) {
        message += "电池电量不足(" + String(OTA_BATTERY_MIN_LEVEL) + "%以下); ";
    }
    if (!condition.versionNewer && !newVersion.isEmpty()) {
        message += "版本不需要更新; ";
    }
    if (!condition.spaceAvailable) {
        message += "存储空间不足; ";
    }
    
    condition.message = message.isEmpty() ? "所有条件满足" : message;
    
    return condition;
}

bool OTAManager::checkBatteryLevel() {
    // 从设备状态获取电池电量百分比
    extern device_state_t device_state;
    int batteryLevel = device_state.battery_percentage;
    
    logMessage("当前电池电量: " + String(batteryLevel) + "%");
    logMessage("充电状态: " + String(device_state.is_charging ? "充电中" : "未充电"));
    
    return batteryLevel >= OTA_BATTERY_MIN_LEVEL;
}

bool OTAManager::checkVersionNewer(String newVersion, String currentVersion) {
    return compareVersions(newVersion, currentVersion) > 0;
}

bool OTAManager::checkAvailableSpace(size_t requiredSize) {
    // 检查可用的Flash空间
    size_t freeSpace = ESP.getFreeSketchSpace();
    logMessage("可用Flash空间: " + formatBytes(freeSpace));
    return freeSpace >= requiredSize;
}

int OTAManager::compareVersions(String version1, String version2) {
    // 简单的版本比较实现
    // 支持格式: v4.0.0, 4.0.0, v4.0.0+694
    
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

bool OTAManager::verifyFirmwareChecksum(String expectedChecksum) {
    // 这里应该实现MD5或SHA256校验
    // 简化实现，实际项目中需要完整的校验逻辑
    logMessage("校验固件完整性: " + expectedChecksum);
    return true; // 暂时返回true
}

void OTAManager::startOnlineDownload(String url, String version) {
    currentStatus = OTA_DOWNLOADING;
    updateProgress(0);
    
    logMessage("开始下载在线固件，请勿断电");
    reportStatus("downloading", 0, "开始下载固件");
    
    if (downloadFirmware(url)) {
        logMessage("下载完成，开始安装");
        if (installFirmwareFromURL(url)) {
            logMessage("升级成功，即将重启");
            currentStatus = OTA_SUCCESS;
            reportStatus("success", 100, "升级成功");
            delay(2000);
            ESP.restart();
        } else {
            logMessage("安装失败");
            currentStatus = OTA_FAILED;
            reportStatus("failed", 0, "安装失败");
        }
    } else {
        logMessage("下载失败");
        currentStatus = OTA_FAILED;
        reportStatus("failed", 0, "下载失败");
    }
}

bool OTAManager::downloadFirmware(String url) {
    logMessage("开始下载固件: " + url);
    
    HTTPClient http;
    http.begin(url);
    http.setTimeout(OTA_DOWNLOAD_TIMEOUT);
    http.addHeader("User-Agent", "ESP32-MotoBox-OTA/1.0");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        int contentLength = http.getSize();
        logMessage("固件大小: " + formatBytes(contentLength));
        
        WiFiClient* stream = http.getStreamPtr();
        
        if (Update.begin(contentLength)) {
            size_t written = 0;
            uint8_t buffer[1024];
            
            while (http.connected() && (written < contentLength)) {
                size_t available = stream->available();
                if (available) {
                    int readBytes = stream->readBytes(buffer, min(available, sizeof(buffer)));
                    written += Update.write(buffer, readBytes);
                    
                    // 更新进度
                    int progress = (written * 100) / contentLength;
                    updateProgress(progress);
                    
                    if (progress % 20 == 0 && progress > 0) { // 每20%播放一次进度提示
                        logMessage("下载进度: " + String(progress) + "%");
                        reportStatus("downloading", progress, "下载进度: " + String(progress) + "%");
                    }
                    
                    // 看门狗喂狗
                    yield();
                }
                delay(1);
            }
            
            if (written == contentLength) {
                logMessage("固件下载完成: " + formatBytes(written));
                http.end();
                return true;
            } else {
                logMessage("下载不完整: " + formatBytes(written) + "/" + formatBytes(contentLength));
            }
        } else {
            logMessage("OTA升级初始化失败: " + String(Update.errorString()));
        }
    } else {
        logMessage("HTTP请求失败: " + String(httpCode));
    }
    
    http.end();
    return false;
}

bool OTAManager::installFirmwareFromURL(String url) {
    // 这个函数在downloadFirmware中已经处理了安装
    // 这里只是为了接口完整性
    if (Update.end(true)) {
        logMessage("固件安装成功");
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
    
    String jsonMessage;
    serializeJson(doc, jsonMessage);
    
    if (mqttPublishCallback) {
        mqttPublishCallback(otaTopicStatus.c_str(), jsonMessage.c_str());
    }
    
    logMessage("状态报告: " + status + " (" + String(progress) + "%) - " + message);
}

void OTAManager::updateProgress(int progress) {
    upgradeProgress = progress;
}

void OTAManager::setMQTTPublishCallback(void (*callback)(const char*, const char*)) {
    mqttPublishCallback = callback;
}

String OTAManager::formatBytes(size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < 1024 * 1024) return String(bytes / 1024.0, 1) + " KB";
    else return String(bytes / (1024.0 * 1024.0), 1) + " MB";
}

void OTAManager::logMessage(String message) {
    Serial.println("[OTAManager] " + message);
}