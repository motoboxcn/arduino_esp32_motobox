#ifndef SDCARD_OTA_H
#define SDCARD_OTA_H

#include <Arduino.h>
#include <Update.h>
#include <SD.h>
#include "version.h"
#include "bat/BAT.h"
#include "device.h"

class SDCardOTA {
public:
    SDCardOTA();
    
    // 初始化
    void begin();
    
    // 检查并执行SD卡升级
    bool checkAndUpgrade();
    
    // 获取升级状态
    String getLastError() { return lastError; }
    bool isUpgrading() { return upgrading; }
    int getProgress() { return progress; }
    
private:
    // 配置
    String firmwareFileName;
    String versionFileName;
    String currentVersion;
    bool upgrading;
    int progress;
    String lastError;
    
    // 升级条件检查
    bool checkBatteryLevel();
    bool checkVersionNewer(String newVersion);
    bool checkFileExists(String filePath);
    
    // 版本比较
    int compareVersions(String version1, String version2);
    
    // SD卡操作
    String readVersionFromSD();
    bool performUpgrade();
    
    // 语音提示
    void playUpgradeSound(int type);
    
    // 日志
    void logMessage(String message);
};

extern SDCardOTA sdCardOTA;

#endif // SDCARD_OTA_H
