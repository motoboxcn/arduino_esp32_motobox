#ifndef SDCARD_OTA_H
#define SDCARD_OTA_H

#include <Arduino.h>
#include <Update.h>
#include <SD.h>
#include "version.h"
#include "bat/BAT.h"
#include "device.h"

// 固件信息结构体
struct FirmwareInfo {
    String fileName;
    String version;
    size_t fileSize;
    bool isValid;
};

class SDCardOTA {
public:
    SDCardOTA();
    
    // 初始化
    void begin();
    
    // 检查并执行SD卡升级
    bool checkAndUpgrade();
    
    // 扫描SD卡中的所有固件文件
    bool scanFirmwareFiles();
    
    // 获取最新版本的固件信息
    FirmwareInfo getLatestFirmware();
    
    // 获取升级状态
    String getLastError() { return lastError; }
    bool isUpgrading() { return upgrading; }
    int getProgress() { return progress; }
    
    // 获取扫描到的固件列表
    void printFirmwareList();
    
private:
    // 配置
    String currentVersion;
    bool upgrading;
    int progress;
    String lastError;
    
    // 固件文件列表
    std::vector<FirmwareInfo> firmwareList;
    
    // 支持的固件文件名模式
    bool isFirmwareFile(String fileName);
    String extractVersionFromFileName(String fileName);
    String extractVersionFromFile(String filePath);
    
    // 升级条件检查
    bool checkBatteryLevel();
    bool checkVersionNewer(String newVersion);
    
    // 版本比较
    int compareVersions(String version1, String version2);
    
    // 升级执行
    bool performUpgrade(const FirmwareInfo& firmware);
    
    // 语音提示
    void playUpgradeSound(int type);
    
    // 日志
    void logMessage(String message);
};

extern SDCardOTA sdCardOTA;

#endif // SDCARD_OTA_H
