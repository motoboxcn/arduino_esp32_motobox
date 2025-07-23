#ifndef GPS_LOGGER_H
#define GPS_LOGGER_H

#include "config.h"

#ifdef ENABLE_GPS_LOGGER
#include "SD/SDManager.h"
#include "SDManager.h"
#include <ArduinoJson.h>
#include "Air780EG.h"
#include "Air780EGGNSS.h"


// 使用Air780EG的GPS数据结构
// 不需要重新定义GPSData，直接使用gnss_data_t

class GPSLogger {
private:
    SDManager* sdManager;
    String currentLogFile;
    bool isFirstRecord;
    unsigned long sessionStartTime;
    int recordCount;
    bool debugMode;
    
    String formatTimestamp(unsigned long timestamp);
    String generateFileName(unsigned long timestamp);
    bool writeCSVHeader(const String& filename);
    bool appendCSVRecord(const String& filename, const gnss_data_t& data);
    bool exportToGeoJSON(const String& csvFile, const String& geoJsonFile);
    
public:
    GPSLogger(SDManager* manager);
    
    // 基本功能
    bool begin();
    bool logGPSData(const gnss_data_t& data);
    void setDebug(bool enable) { debugMode = enable; }
    
    // 会话管理
    void startNewSession();
    void endCurrentSession();
    String getCurrentLogFile() { return currentLogFile; }
    int getRecordCount() { return recordCount; }
    
    // 数据导出
    bool exportSessionToGeoJSON();
    bool exportAllToGeoJSON();
    
    // 文件管理
    void listLogFiles();
    bool deleteOldLogs(int daysToKeep = GPS_AUTO_CLEANUP_DAYS);
    void getStorageInfo();
    
    // 串口命令处理
    bool handleSerialCommand(const String& command);
};

#endif // ENABLE_GPS_LOGGER

#endif // GPS_LOGGER_H
