#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "version.h"
#include "bat/BAT.h"
#include "Air780EG.h"

enum OTAStatus {
    OTA_IDLE,
    OTA_CHECKING,
    OTA_DOWNLOADING,
    OTA_INSTALLING,
    OTA_SUCCESS,
    OTA_FAILED
};

class OTAManager {
public:
    OTAManager();
    
    void begin(Air780EG* air780eg_instance);
    void handleMQTTMessage(String topic, String payload);
    void checkForUpdates();
    void checkTimeout();
    
    // 自动升级开关
    void setAutoUpgrade(bool enabled);
    bool getAutoUpgrade();
    
    OTAStatus getStatus() { return currentStatus; }
    int getProgress() { return upgradeProgress; }
    
    void setMQTTPublishCallback(void (*callback)(const char*, const char*));
    
private:
    String deviceId;
    String currentVersion;
    OTAStatus currentStatus;
    int upgradeProgress;
    unsigned long checkStartTime;
    
    Air780EG* air780eg;
    void (*mqttPublishCallback)(const char*, const char*);
    
    String otaTopicCheck;
    String otaTopicDownload;
    String otaTopicStatus;
    
    bool checkUpgradeConditions();
    bool downloadAndInstall(String url);
    void logMessage(String message);
};

extern OTAManager otaManager;

#endif
