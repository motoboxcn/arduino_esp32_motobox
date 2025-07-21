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

#endif // SDMANAGER_H
