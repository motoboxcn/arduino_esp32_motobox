#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "version.h"
#include "bat/BAT.h"

// OTA升级状态枚举
enum OTAStatus {
    OTA_IDLE,
    OTA_CHECKING,
    OTA_DOWNLOADING,
    OTA_INSTALLING,
    OTA_SUCCESS,
    OTA_FAILED
};

// 升级条件检查结果
struct UpgradeCondition {
    bool batteryOK;
    bool versionNewer;
    bool fileExists;
    bool spaceAvailable;
    String message;
};

class OTAManager {
public:
    OTAManager();
    
    // 初始化
    void begin();
    
    
    // MQTT在线升级相关
    void setupMQTTOTA();
    void handleMQTTMessage(String topic, String payload);
    void checkForUpdates();
    
    // 升级条件检查
    UpgradeCondition checkUpgradeConditions(String newVersion = "");
    
    // 状态管理
    OTAStatus getStatus() { return currentStatus; }
    int getProgress() { return upgradeProgress; }
    
    // 回调函数设置
    void setMQTTPublishCallback(void (*callback)(const char*, const char*));
    
private:
    // 基本属性
    String deviceId;
    String currentVersion;
    OTAStatus currentStatus;
    int upgradeProgress;
    
    // MQTT相关
    String otaTopicCheck;
    String otaTopicDownload;
    String otaTopicStatus;
    void (*mqttPublishCallback)(const char*, const char*);
    
    
    // 升级条件检查
    bool checkBatteryLevel();
    bool checkVersionNewer(String newVersion, String currentVersion);
    bool checkAvailableSpace(size_t requiredSize);
    
    // 版本比较
    int compareVersions(String version1, String version2);
    
    // 固件验证
    bool verifyFirmwareChecksum(String expectedChecksum);
    
    // 在线升级操作
    void startOnlineDownload(String url, String version);
    bool downloadFirmware(String url);
    
    // 升级执行
    bool installFirmwareFromURL(String url);
    
    // 状态报告
    void reportStatus(String status, int progress, String message = "");
    void updateProgress(int progress);
    
    // 工具函数
    String formatBytes(size_t bytes);
    void logMessage(String message);
};

extern OTAManager otaManager;

#endif // OTA_MANAGER_H
