#include "GPSLogger.h"

#ifdef ENABLE_GPS_LOGGER

extern SDManager sdManager;
GPSLogger gpsLogger(&sdManager);

GPSLogger::GPSLogger(SDManager* manager) : 
    sdManager(manager),
    isFirstRecord(true), 
    sessionStartTime(0),
    recordCount(0),
    debugMode(GPS_DEBUG_ENABLED) {
}

bool GPSLogger::begin() {
    if (!sdManager || !sdManager->isInitialized()) {
        if (debugMode) Serial.println("[GPS] SD卡未就绪");
        return false;
    }
    
    // 确保GPS日志目录存在
    if (!sdManager->createDir(GPS_LOG_DIR)) {
        if (debugMode) Serial.println("[GPS] 创建GPS日志目录失败");
        return false;
    }
    
    if (debugMode) Serial.println("[GPS] GPS记录器初始化成功");
    return true;
}

String GPSLogger::formatTimestamp(unsigned long timestamp) {
    // 将时间戳转换为可读格式: YYYYMMDD_HHMMSS
    // 这里简化处理，实际应用中可以使用RTC或NTP时间
    char buffer[20];
    sprintf(buffer, "%lu", timestamp);
    return String(buffer);
}

String GPSLogger::generateFileName(unsigned long timestamp) {
    // 生成文件名: GPS_YYYYMMDD_HHMMSS.csv
    return String(GPS_LOG_DIR) + "/GPS_" + formatTimestamp(timestamp) + ".csv";
}

bool GPSLogger::writeCSVHeader(const String& filename) {
    if (!sdManager->writeFile(filename, "timestamp,latitude,longitude,altitude,speed,course,satellites,valid\n")) {
        if (debugMode) Serial.println("[GPS] 无法创建CSV文件: " + filename);
        return false;
    }
    
    if (debugMode) Serial.println("[GPS] CSV文件创建成功: " + filename);
    return true;
}

bool GPSLogger::appendCSVRecord(const String& filename, const gnss_data_t& data) {
    // 构建CSV记录
    String record = String(millis()) + ",";
    record += String(data.latitude, GPS_COORDINATE_PRECISION) + ",";
    record += String(data.longitude, GPS_COORDINATE_PRECISION) + ",";
    record += String(data.altitude, GPS_ALTITUDE_PRECISION) + ",";
    record += String(data.speed, GPS_SPEED_PRECISION) + ",";
    record += String(data.course, GPS_SPEED_PRECISION) + ",";
    record += String(data.satellites) + ",";
    record += (data.is_fixed ? "1" : "0");
    record += "\n";
    
    if (!sdManager->appendFile(filename, record)) {
        if (debugMode) Serial.println("[GPS] 无法追加GPS数据到文件");
        return false;
    }
    
    return true;
}

bool GPSLogger::logGPSData(const gnss_data_t& data) {
    if (!sdManager || !sdManager->isInitialized()) {
        if (debugMode) Serial.println("[GPS] SD卡不可用");
        return false;
    }
    
    // 如果是第一条记录，创建新的日志文件
    if (isFirstRecord) {
        sessionStartTime = millis();
        currentLogFile = generateFileName(sessionStartTime);
        
        if (!writeCSVHeader(currentLogFile)) {
            return false;
        }
        
        isFirstRecord = false;
        recordCount = 0;
        
        if (debugMode) {
            Serial.println("[GPS] 开始新的GPS记录会话");
            Serial.println("[GPS] 日志文件: " + currentLogFile);
        }
    }
    
    // 追加GPS数据到CSV文件
    if (appendCSVRecord(currentLogFile, data)) {
        recordCount++;
        
        if (debugMode && recordCount % 10 == 0) {
            Serial.printf("[GPS] 已记录 %d 条GPS数据\n", recordCount);
        }
        
        // 每100条记录检查一次存储空间
        if (recordCount % 100 == 0) {
            getStorageInfo();
        }
        
        return true;
    }
    
    return false;
}

void GPSLogger::startNewSession() {
    isFirstRecord = true;
    currentLogFile = "";
    sessionStartTime = 0;
    recordCount = 0;
    
    if (debugMode) Serial.println("[GPS] 准备开始新的GPS记录会话");
}

void GPSLogger::endCurrentSession() {
    if (!currentLogFile.isEmpty() && recordCount > 0) {
        if (debugMode) {
            Serial.printf("[GPS] 会话结束，共记录 %d 条数据\n", recordCount);
            Serial.println("[GPS] 文件: " + currentLogFile);
        }
        
        // 可选：自动导出为GeoJSON
        if (GPS_AUTO_EXPORT_GEOJSON) {
            exportSessionToGeoJSON();
        }
    }
    
    startNewSession();
}

bool GPSLogger::exportToGeoJSON(const String& csvFile, const String& geoJsonFile) {
    String csvContent = sdManager->readFile(csvFile);
    if (csvContent.isEmpty()) {
        if (debugMode) Serial.println("[GPS] 无法读取CSV文件: " + csvFile);
        return false;
    }
    
    // 构建GeoJSON内容
    String geoJson = "{\n";
    geoJson += "  \"type\": \"FeatureCollection\",\n";
    geoJson += "  \"features\": [\n";
    
    // 解析CSV内容
    int lineStart = csvContent.indexOf('\n') + 1; // 跳过头部
    bool firstFeature = true;
    int exportCount = 0;
    
    while (lineStart < csvContent.length()) {
        int lineEnd = csvContent.indexOf('\n', lineStart);
        if (lineEnd == -1) lineEnd = csvContent.length();
        
        String line = csvContent.substring(lineStart, lineEnd);
        line.trim();
        
        if (line.length() == 0) {
            lineStart = lineEnd + 1;
            continue;
        }
        
        // 解析CSV行
        int commaIndex[7];
        int commaCount = 0;
        for (int i = 0; i < line.length() && commaCount < 7; i++) {
            if (line.charAt(i) == ',') {
                commaIndex[commaCount++] = i;
            }
        }
        
        if (commaCount < 7) {
            lineStart = lineEnd + 1;
            continue;
        }
        
        // 提取数据
        String timestamp = line.substring(0, commaIndex[0]);
        String lat = line.substring(commaIndex[0] + 1, commaIndex[1]);
        String lng = line.substring(commaIndex[1] + 1, commaIndex[2]);
        String alt = line.substring(commaIndex[2] + 1, commaIndex[3]);
        String speed = line.substring(commaIndex[3] + 1, commaIndex[4]);
        String course = line.substring(commaIndex[4] + 1, commaIndex[5]);
        String satellites = line.substring(commaIndex[5] + 1, commaIndex[6]);
        String valid = line.substring(commaIndex[6] + 1);
        
        // 只导出有效的GPS数据
        if (valid == "1") {
            if (!firstFeature) {
                geoJson += ",\n";
            }
            
            // 写入GeoJSON特征
            geoJson += "    {\n";
            geoJson += "      \"type\": \"Feature\",\n";
            geoJson += "      \"geometry\": {\n";
            geoJson += "        \"type\": \"Point\",\n";
            geoJson += "        \"coordinates\": [" + lng + ", " + lat + ", " + alt + "]\n";
            geoJson += "      },\n";
            geoJson += "      \"properties\": {\n";
            geoJson += "        \"timestamp\": " + timestamp + ",\n";
            geoJson += "        \"speed\": " + speed + ",\n";
            geoJson += "        \"course\": " + course + ",\n";
            geoJson += "        \"satellites\": " + satellites + "\n";
            geoJson += "      }\n";
            geoJson += "    }";
            
            firstFeature = false;
            exportCount++;
        }
        
        lineStart = lineEnd + 1;
    }
    
    // 写入GeoJSON尾部
    geoJson += "\n  ]\n";
    geoJson += "}\n";
    
    if (!sdManager->writeFile(geoJsonFile, geoJson)) {
        if (debugMode) Serial.println("[GPS] 无法写入GeoJSON文件: " + geoJsonFile);
        return false;
    }
    
    if (debugMode) {
        Serial.printf("[GPS] GeoJSON导出完成: %d 个有效点\n", exportCount);
        Serial.println("[GPS] 文件: " + geoJsonFile);
    }
    
    return true;
}

bool GPSLogger::exportSessionToGeoJSON() {
    if (currentLogFile.isEmpty()) {
        if (debugMode) Serial.println("[GPS] 没有当前会话可导出");
        return false;
    }
    
    String geoJsonFile = currentLogFile;
    geoJsonFile.replace(".csv", ".geojson");
    
    return exportToGeoJSON(currentLogFile, geoJsonFile);
}

bool GPSLogger::exportAllToGeoJSON() {
    // 使用SDManager的listDir功能
    if (debugMode) Serial.println("[GPS] 开始批量导出GeoJSON...");
    
    // 这里需要实现目录遍历，暂时简化
    if (debugMode) Serial.println("[GPS] 批量导出功能需要进一步实现");
    
    return false;
}

void GPSLogger::listLogFiles() {
    if (debugMode) Serial.println("[GPS] GPS日志文件列表:");
    
    // 使用SDManager的listDir功能
    sdManager->listDir(GPS_LOG_DIR);
}

bool GPSLogger::deleteOldLogs(int daysToKeep) {
    // 简化实现：删除超过指定数量的文件
    // 实际应用中应该基于文件创建时间判断
    
    if (debugMode) Serial.println("[GPS] 清理旧日志文件功能需要进一步实现");
    
    return true;
}

void GPSLogger::getStorageInfo() {
    if (!sdManager) return;
    
    uint64_t totalMB = sdManager->getTotalSpaceMB();
    uint64_t freeMB = sdManager->getFreeSpaceMB();
    uint64_t usedMB = totalMB - freeMB;
    
    if (debugMode) {
        Serial.printf("[GPS] 存储空间 - 总计: %llu MB, 已用: %llu MB, 可用: %llu MB\n", 
                     totalMB, usedMB, freeMB);
    }
    
    // 如果可用空间少于设定值，发出警告
    if (freeMB < GPS_MIN_FREE_SPACE_MB) {
        if (debugMode) Serial.println("[GPS] 警告: 存储空间不足!");
    }
}

bool GPSLogger::handleSerialCommand(const String& command) {
    String cmd = command;
    cmd.toLowerCase();
    cmd.trim();
    
    if (cmd == "gps_start" || cmd == "gs") {
        startNewSession();
        Serial.println("[GPS] 开始新的GPS记录会话");
        return true;
        
    } else if (cmd == "gps_stop" || cmd == "gt") {
        endCurrentSession();
        Serial.println("[GPS] GPS记录会话已结束");
        return true;
        
    } else if (cmd == "gps_status" || cmd == "gst") {
        Serial.println("=== GPS系统状态 ===");
        Serial.printf("当前会话: %s\n", currentLogFile.c_str());
        Serial.printf("记录数量: %d\n", recordCount);
        Serial.printf("会话开始时间: %lu\n", sessionStartTime);
        return true;
        
    } else if (cmd == "gps_export" || cmd == "ge") {
        Serial.println("[GPS] 导出当前会话为GeoJSON...");
        if (exportSessionToGeoJSON()) {
            Serial.println("[GPS] 导出成功");
        } else {
            Serial.println("[GPS] 导出失败");
        }
        return true;
        
    } else if (cmd == "gps_list" || cmd == "gl") {
        listLogFiles();
        return true;
        
    } else if (cmd == "gps_info" || cmd == "gi") {
        getStorageInfo();
        return true;
        
    } else if (cmd == "gps_help" || cmd == "gh") {
        Serial.println("=== GPS命令帮助 ===");
        Serial.println("gps_start (gs)  - 开始GPS记录");
        Serial.println("gps_stop (gt)   - 停止GPS记录");
        Serial.println("gps_status (gst)- 显示GPS状态");
        Serial.println("gps_export (ge) - 导出GeoJSON");
        Serial.println("gps_list (gl)   - 列出日志文件");
        Serial.println("gps_info (gi)   - 显示存储信息");
        Serial.println("gps_help (gh)   - 显示此帮助");
        return true;
    }
    
    return false;  // 不是GPS命令
}

#endif // ENABLE_GPS_LOGGER
