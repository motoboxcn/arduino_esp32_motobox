#ifndef SDMANAGER_H
#define SDMANAGER_H

#include <Arduino.h>
#include "Air780EG.h"
#include "Air780EGGNSS.h"

#ifdef ENABLE_SDCARD

#include <SPI.h>
#include "device.h"

#ifdef SD_MODE_SPI
#include <SD.h>
#else
#include <SD_MMC.h>
// ESP32 4位SDIO模式固定引脚配置
#define SDCARD_CLK_IO  14  // 时钟线（固定）
#define SDCARD_CMD_IO  15  // 命令线（固定）
#define SDCARD_D0_IO   2   // 数据线0（固定）
#define SDCARD_D1_IO   4   // 数据线1（固定）
#define SDCARD_D2_IO   12  // 数据线2（固定）
#define SDCARD_D3_IO   13  // 数据线3（固定）
#endif

#include <esp_system.h>

#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "2.3.0"
#endif

// ========== SD卡目录结构定义 ==========
// 根目录文件
#define SD_DEVICE_INFO_FILE     "/deviceinfo.txt"

// 主要目录
#define SD_UPDATES_DIR          "/updates"
#define SD_DATA_DIR             "/data"
#define SD_VOICE_DIR            "/voice"
#define SD_LOGS_DIR             "/logs"
#define SD_CONFIG_DIR           "/config"

// 数据子目录
#define SD_GPS_DATA_DIR         "/data/gps"
#define SD_SENSOR_DATA_DIR      "/data/sensor"
#define SD_SYSTEM_DATA_DIR      "/data/system"

// 配置文件
#define SD_WIFI_CONFIG_FILE     "/config/wifi.json"
#define SD_MQTT_CONFIG_FILE     "/config/mqtt.json"
#define SD_DEVICE_CONFIG_FILE   "/config/device.json"

// 语音文件
#define SD_WELCOME_VOICE_FILE   "/voice/welcome.wav"

// 日志文件
#define SD_SYSTEM_LOG_FILE      "/logs/system.log"
#define SD_ERROR_LOG_FILE       "/logs/error.log"
#define SD_GPS_LOG_FILE         "/logs/gps.log"

// 升级相关
#define SD_FIRMWARE_FILE        "/updates/firmware.bin"
#define SD_UPDATE_INFO_FILE     "/updates/update_info.json"

#endif // ENABLE_SDCARD

class SDManager {
public:
    SDManager();
    ~SDManager();

    // 基本操作
    bool begin();
    void end();
    bool isInitialized();

    // 空间信息
    uint64_t getTotalSpaceMB();
    uint64_t getFreeSpaceMB();

    // 核心功能
    bool saveDeviceInfo();
    bool recordGPSData(gnss_data_t &gnss_data);
    bool finishGPSSession();

    // 文件操作方法
    bool writeFile(const String& path, const String& content);
    bool appendFile(const String& path, const String& content);
    String readFile(const String& path);
    bool deleteFile(const String& path);
    bool fileExists(const String& path);
    bool createDir(const String& path);
    void listDir(const String& path);
    
    // 新增的文件系统操作方法
    bool listDirectory(const String& path);
    bool listDirectoryTree(const String& path, int depth, int maxDepth);
    bool createDirectory(const String& path);
    bool displayFileContent(const String& path);
    bool removeFile(const String& path);
    bool removeDirectory(const String& path);

    // 串口命令处理
    bool handleSerialCommand(const String& command);

    // 显示SD卡目录结构信息
    void printDirectoryStructure();

    // ========== 简单语音文件支持 ==========
    /**
     * @brief 检查SD卡上是否存在自定义欢迎语音文件
     * @return 是否存在 /voice/welcome.wav
     */
    bool hasCustomWelcomeVoice();
    /**
     * @brief 获取自定义欢迎语音文件路径
     * @return 文件路径 "/voice/welcome.wav"
     */
    String getCustomWelcomeVoicePath();
    /**
     * @brief 检查语音文件是否有效（WAV格式）
     * @return 是否有效
     */
    bool isValidWelcomeVoiceFile();

#ifdef ENABLE_SDCARD
private:
    bool _initialized;

    // 内部方法
    bool createDirectoryStructure();
    bool createDirectory(const char* path);
    bool ensureGPSDirectoryExists();
    bool directoryExists(const char* path);
    
    // 工具方法
    String getCurrentTimestamp();
    String getCurrentDateString();
    String getCurrentTimeString();
    String generateGPSSessionFilename();
    int getBootCount();
    void debugPrint(const String& message);
    String formatFileSize(size_t bytes);
#endif // ENABLE_SDCARD
};

extern SDManager sdManager;

#endif // SDMANAGER_H
