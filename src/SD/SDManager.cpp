#include "SDManager.h"

#ifdef ENABLE_SDCARD

SDManager sdManager;

SDManager::SDManager() : _initialized(false) {}

SDManager::~SDManager() {
    if (_initialized) {
        end();
    }
}

bool SDManager::begin() {
    if (_initialized) {
        return true;
    }

    debugPrint("正在初始化SD卡...");

    // 4位SDIO模式初始化
    debugPrint("使用4位SDIO模式");
    debugPrint("引脚配置: CLK=" + String(SDCARD_CLK_IO) + ", CMD=" + String(SDCARD_CMD_IO) + 
               ", D0=" + String(SDCARD_D0_IO) + ", D1=" + String(SDCARD_D1_IO) + 
               ", D2=" + String(SDCARD_D2_IO) + ", D3=" + String(SDCARD_D3_IO));
    
    // 使用4位SDIO模式初始化
    if (!SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT, 4)) {
        debugPrint("❌ SD卡4位SDIO模式初始化失败");
        debugPrint("可能的原因：");
        debugPrint("  1. 未插入SD卡");
        debugPrint("  2. SD卡损坏或格式不支持");
        debugPrint("  3. 硬件连接错误");
        debugPrint("  4. SD卡格式不是FAT32");
        debugPrint("  5. 引脚配置错误");
        debugPrint("请检查SD卡并重试");
        return false;
    }
    
    // 设置初始化标志
    _initialized = true;
    
    debugPrint("✅ SD卡4位SDIO模式初始化成功");
    debugPrint("SD卡容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
    debugPrint("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");

    // 创建必要的目录结构
    if (!createDirectoryStructure()) {
        debugPrint("⚠️ 目录结构创建失败，但SD卡可用");
    }
    
    // 保存设备信息
    if (!saveDeviceInfo()) {
        debugPrint("⚠️ 设备信息保存失败，但SD卡可用");
    }

    return true;
}

void SDManager::end() {
    if (!_initialized) {
        return;
    }

    SD_MMC.end();
    _initialized = false;
    debugPrint("SD卡已断开");
}

bool SDManager::isInitialized() {
    return _initialized;
}

uint64_t SDManager::getTotalSpaceMB() {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法获取容量信息");
        return 0;
    }

    try {
        return SD_MMC.totalBytes() / (1024 * 1024);
    } catch (...) {
        debugPrint("⚠️ 获取SD卡容量失败，可能SD卡已移除");
        return 0;
    }
}

uint64_t SDManager::getFreeSpaceMB() {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法获取剩余空间");
        return 0;
    }

    try {
        return (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
    } catch (...) {
        debugPrint("⚠️ 获取SD卡剩余空间失败，可能SD卡已移除");
        return 0;
    }
}

bool SDManager::createDirectoryStructure() {
    if (!_initialized) {
        return false;
    }

    // 创建基本目录结构
    const char* directories[] = {
        SD_DATA_DIR,
        SD_GPS_DATA_DIR,
        SD_SENSOR_DATA_DIR,
        SD_SYSTEM_DATA_DIR,
        SD_CONFIG_DIR,
        SD_UPDATES_DIR,
        SD_VOICE_DIR,
        SD_LOGS_DIR
    };

    for (int i = 0; i < 8; i++) {
        if (!createDirectory(directories[i])) {
            debugPrint("创建目录失败: " + String(directories[i]));
            return false;
        }
    }

    debugPrint("✅ 目录结构创建完成");
    return true;
}

bool SDManager::createDirectory(const char* path) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法创建目录: " + String(path));
        return false;
    }

    // 先检查目录是否已存在
    if (directoryExists(path)) {
        debugPrint("📁 目录已存在: " + String(path));
        return true;
    }

    debugPrint("🔧 正在创建目录: " + String(path));

    bool success = false;
    try {
        success = SD_MMC.mkdir(path);
    } catch (...) {
        debugPrint("❌ 创建目录时发生异常: " + String(path));
        return false;
    }

    if (success) {
        debugPrint("✅ 目录创建成功: " + String(path));
        
        // 验证目录是否真的创建成功
        if (directoryExists(path)) {
            return true;
        } else {
            debugPrint("⚠️ 目录创建报告成功但验证失败: " + String(path));
            return false;
        }
    } else {
        debugPrint("❌ 目录创建失败: " + String(path));
        debugPrint("可能的原因：");
        debugPrint("  1. SD卡空间不足");
        debugPrint("  2. SD卡写保护");
        debugPrint("  3. 文件系统错误");
        debugPrint("  4. 路径格式错误");
        return false;
    }
}

bool SDManager::saveDeviceInfo() {
    if (!_initialized) {
        return false;
    }

    const char* filename = SD_DEVICE_INFO_FILE;
    
    File file = SD_MMC.open(filename, FILE_WRITE);

    if (!file) {
        debugPrint("❌ 无法创建设备信息文件: " + String(filename));
        return false;
    }

    // 创建设备信息JSON
    String deviceInfo = "{\n";
    deviceInfo += "  \"device_id\": \"" + device.get_device_id() + "\",\n";
    deviceInfo += "  \"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\",\n";
    deviceInfo += "  \"hardware_version\": \"v1.0\",\n";
    deviceInfo += "  \"created_at\": \"" + getCurrentTimestamp() + "\",\n";
    deviceInfo += "  \"last_updated\": \"" + getCurrentTimestamp() + "\",\n";
    deviceInfo += "  \"boot_count\": " + String(getBootCount()) + ",\n";
    deviceInfo += "  \"sd_card\": {\n";
    deviceInfo += "    \"total_mb\": " + String((unsigned long)getTotalSpaceMB()) + ",\n";
    deviceInfo += "    \"free_mb\": " + String((unsigned long)getFreeSpaceMB()) + "\n";
    deviceInfo += "  }\n";
    deviceInfo += "}";

    file.print(deviceInfo);
    file.close();

    debugPrint("✅ 设备信息已保存到 " + String(filename));
    return true;
}

bool SDManager::recordGPSData(gnss_data_t &gnss_data) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法记录GPS数据");
        return false;
    }

    // 生成当前会话的GPS文件名
    String filename = generateGPSSessionFilename();
    
    // 确保GPS目录存在
    if (!ensureGPSDirectoryExists()) {
        debugPrint("❌ 无法创建GPS目录");
        return false;
    }
    
    // 检查文件是否存在，如果不存在则创建GeoJSON头部
    bool fileExists = false;
    
    try {
        File testFile = SD_MMC.open(filename.c_str(), FILE_READ);
        
        if (testFile) {
            fileExists = true;
            testFile.close();
            debugPrint("📄 使用现有GPS会话文件: " + filename);
        }
    } catch (...) {
        debugPrint("⚠️ 检查GPS文件状态失败，可能SD卡已移除");
        return false;
    }

    // 打开文件进行写入
    File file;
    try {
        file = SD_MMC.open(filename.c_str(), FILE_APPEND);
    } catch (...) {
        debugPrint("⚠️ 打开GPS数据文件失败，可能SD卡已移除");
        return false;
    }

    if (!file) {
        debugPrint("❌ 无法打开GPS数据文件: " + filename);
        debugPrint("可能的原因：");
        debugPrint("  1. SD卡空间不足");
        debugPrint("  2. SD卡已移除");
        debugPrint("  3. 文件系统错误");
        debugPrint("  4. 目录权限问题");
        
        // 尝试重新创建目录
        debugPrint("🔧 尝试重新创建GPS目录...");
        if (createDirectory(SD_DATA_DIR) && createDirectory(SD_GPS_DATA_DIR)) {
            debugPrint("✅ GPS目录重新创建成功，请重试");
        } else {
            debugPrint("❌ GPS目录重新创建失败");
        }
        return false;
    }

    // 如果是新文件，写入GeoJSON头部和会话信息
    if (!fileExists) {
        debugPrint("📁 创建新的GPS会话文件: " + filename);
        
        file.println("{");
        file.println("  \"type\": \"FeatureCollection\",");
        file.println("  \"metadata\": {");
        file.println("    \"device_id\": \"" + device.get_device_id() + "\",");
        file.println("    \"session_start\": \"" + getCurrentTimestamp() + "\",");
        file.println("    \"boot_count\": " + String(getBootCount()) + ",");
        file.println("    \"firmware_version\": \"" + String(FIRMWARE_VERSION) + "\"");
        file.println("  },");
        file.println("  \"features\": [");
    } else {
        // 如果文件已存在，需要在最后一个特征后添加逗号
        file.print(",\n");
    }

    // 写入GPS数据点
    String gpsFeature = "    {\n";
    gpsFeature += "      \"type\": \"Feature\",\n";
    gpsFeature += "      \"geometry\": {\n";
    gpsFeature += "        \"type\": \"Point\",\n";
    gpsFeature += "        \"coordinates\": [" + String(gnss_data.longitude, 6) + ", " + String(gnss_data.latitude, 6) + ", " + String(gnss_data.altitude, 2) + "]\n";
    gpsFeature += "      },\n";
    gpsFeature += "      \"properties\": {\n";
    gpsFeature += "        \"timestamp\": \"" + getCurrentTimestamp() + "\",\n";
    gpsFeature += "        \"runtime_ms\": " + String(millis()) + ",\n";
    gpsFeature += "        \"speed_kmh\": " + String(gnss_data.speed, 2) + ",\n";
    gpsFeature += "        \"satellites\": " + String(gnss_data.satellites) + ",\n";
    gpsFeature += "        \"hdop\": 0.0\n";  // 可以后续添加HDOP数据
    gpsFeature += "      }\n";
    gpsFeature += "    }";

    size_t bytesWritten = file.print(gpsFeature);
    file.flush(); // 确保数据写入
    file.close();

    if (bytesWritten == 0) {
        debugPrint("❌ GPS数据写入失败");
        debugPrint("可能SD卡空间不足或已移除");
        return false;
    }

    debugPrint("📍 GPS数据已记录: " + String(gnss_data.latitude, 6) + "," + String(gnss_data.longitude, 6) + " (卫星:" + String(gnss_data.satellites) + ")");
    return true;
}

String SDManager::getCurrentTimestamp() {
    // 简单的时间戳，实际项目中应该使用RTC或NTP时间
    return String(millis());
}

String SDManager::getCurrentDateString() {
    // 简单的日期字符串，实际项目中应该使用真实日期
    unsigned long days = millis() / (24 * 60 * 60 * 1000);
    return String(days);
}

String SDManager::getCurrentTimeString() {
    // 生成当前时间字符串 HHMMSS
    unsigned long currentTime = millis();
    unsigned long hours = (currentTime % (24 * 60 * 60 * 1000)) / (60 * 60 * 1000);
    unsigned long minutes = (currentTime % (60 * 60 * 1000)) / (60 * 1000);
    unsigned long seconds = (currentTime % (60 * 1000)) / 1000;
    
    String timeStr = "";
    if (hours < 10) timeStr += "0";
    timeStr += String(hours);
    if (minutes < 10) timeStr += "0";
    timeStr += String(minutes);
    if (seconds < 10) timeStr += "0";
    timeStr += String(seconds);
    
    return timeStr;
}

String SDManager::generateGPSSessionFilename() {
    // 生成基于启动会话的GPS文件名
    // 格式: gps_YYYYMMDD_HHMMSS_bootXXX.geojson
    
    // 获取当前日期时间（简化版）
    String dateStr = getCurrentDateString();
    String timeStr = getCurrentTimeString();
    
    // 格式化启动次数
    String bootStr = String(getBootCount());
    while (bootStr.length() < 3) {
        bootStr = "0" + bootStr;
    }
    
    return String(SD_GPS_DATA_DIR) + "/gps_" + dateStr + "_" + timeStr + "_boot" + bootStr + ".geojson";
}

int SDManager::getBootCount() {
    // 从外部获取启动次数
    extern int bootCount;
    return bootCount;
}

bool SDManager::finishGPSSession() {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法结束GPS会话");
        return false;
    }

    // 获取当前会话文件名
    String filename = generateGPSSessionFilename();
    
    // 检查文件是否存在
    File testFile;
    try {
        testFile = SD_MMC.open(filename.c_str(), FILE_READ);
    } catch (...) {
        debugPrint("⚠️ 检查GPS文件失败");
        return false;
    }

    if (!testFile) {
        debugPrint("⚠️ GPS会话文件不存在: " + filename);
        return false;
    }
    testFile.close();

    // 以追加模式打开文件，添加GeoJSON结尾
    File file;
    try {
        file = SD_MMC.open(filename.c_str(), FILE_APPEND);
    } catch (...) {
        debugPrint("⚠️ 打开GPS文件失败");
        return false;
    }

    if (!file) {
        debugPrint("❌ 无法打开GPS会话文件进行结束操作");
        return false;
    }

    // 添加GeoJSON结尾
    file.println("");
    file.println("  ]");
    file.println("}");
    file.close();

    debugPrint("✅ GPS会话已结束: " + filename);
    return true;
}

bool SDManager::ensureGPSDirectoryExists() {
    if (!_initialized) {
        return false;
    }

    // 检查并创建 /data 目录
    if (!directoryExists("/data")) {
        debugPrint("🔧 创建 /data 目录...");
        if (!createDirectory("/data")) {
            debugPrint("❌ 创建 /data 目录失败");
            return false;
        }
        debugPrint("✅ /data 目录创建成功");
    }

    // 检查并创建 GPS 数据目录
    if (!directoryExists(SD_GPS_DATA_DIR)) {
        debugPrint("🔧 创建 GPS 数据目录...");
        if (!createDirectory(SD_GPS_DATA_DIR)) {
            debugPrint("❌ 创建 GPS 数据目录失败");
            return false;
        }
        debugPrint("✅ GPS 数据目录创建成功");
    }

    return true;
}

bool SDManager::directoryExists(const char* path) {
    if (!_initialized) {
        return false;
    }

    File dir;
    try {
        dir = SD_MMC.open(path);
    } catch (...) {
        return false;
    }

    if (!dir) {
        return false;
    }

    bool isDir = dir.isDirectory();
    dir.close();
    return isDir;
}

void SDManager::debugPrint(const String& message) {
    Serial.println("[SDManager] " + message);
}

// 串口命令处理
bool SDManager::handleSerialCommand(const String& command) {
    if (!_initialized && command != "sd.init") {
        Serial.println("❌ SD卡未初始化，请先使用 'sd.init' 初始化");
        return false;
    }

    // 基本信息查询
    if (command == "sd.info") {
        Serial.println("=== SD卡详细信息 ===");
        Serial.println("初始化状态: " + String(_initialized ? "✅ 已初始化" : "❌ 未初始化"));
        if (_initialized) {
            Serial.println("总容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
            Serial.println("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
            Serial.println("使用率: " + String(100.0 * (getTotalSpaceMB() - getFreeSpaceMB()) / getTotalSpaceMB(), 1) + "%");
        }
        return true;
    }
    
    // 状态检查
    else if (command == "sd.status") {
        Serial.println("=== SD卡状态检查 ===");
        Serial.println("SD卡状态: " + String(_initialized ? "✅ 正常" : "❌ 异常"));
        if (_initialized) {
            Serial.println("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
            
            // 检查关键目录
            Serial.println("--- 目录状态 ---");
            Serial.println(String(SD_DATA_DIR) + ": " + String(directoryExists(SD_DATA_DIR) ? "✅" : "❌"));
            Serial.println(String(SD_GPS_DATA_DIR) + ": " + String(directoryExists(SD_GPS_DATA_DIR) ? "✅" : "❌"));
            Serial.println(String(SD_CONFIG_DIR) + ": " + String(directoryExists(SD_CONFIG_DIR) ? "✅" : "❌"));
            Serial.println(String(SD_UPDATES_DIR) + ": " + String(directoryExists(SD_UPDATES_DIR) ? "✅" : "❌"));
        }
        return true;
    }
    
    // 目录结构查看
    else if (command == "sd.tree") {
        Serial.println("=== SD卡目录结构 ===");
        listDirectoryTree("/", 0, 3); // 最多显示3层
        return true;
    }
    
    // 显示目录结构定义
    else if (command == "sd.structure") {
        printDirectoryStructure();
        return true;
    }
    
    // 格式化说明（不实际格式化，只显示说明）
    else if (command == "sd.fmt") {
        Serial.println("=== SD卡格式化说明 ===");
        Serial.println("⚠️ 注意：ESP32不支持直接格式化SD卡");
        Serial.println("如需格式化SD卡，请：");
        Serial.println("1. 将SD卡取出");
        Serial.println("2. 使用电脑格式化为FAT32格式");
        Serial.println("3. 重新插入SD卡");
        Serial.println("4. 使用 'sd.init' 重新初始化");
        return true;
    }
    
    // 重新初始化
    else if (command == "sd.init") {
        Serial.println("=== 重新初始化SD卡 ===");
        
        if (_initialized) {
            end(); // 先结束当前连接
        }
        
        if (begin()) {
            Serial.println("✅ SD卡初始化成功");
            Serial.println("总容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
            Serial.println("可用空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
        } else {
            Serial.println("❌ SD卡初始化失败");
            Serial.println("请检查：");
            Serial.println("1. SD卡是否正确插入");
            Serial.println("2. SD卡是否损坏");
            Serial.println("3. SD卡格式是否为FAT32");
        }
        return true;
    }
    
    // 帮助信息
    else if (command == "sd.help") {
        Serial.println("=== SD卡简化命令 ===");
        Serial.println("sd.info      - 显示SD卡详细信息");
        Serial.println("sd.status    - 检查SD卡和目录状态");
        Serial.println("sd.tree      - 显示目录树结构");
        Serial.println("sd.structure - 显示目录结构定义");
        Serial.println("sd.fmt       - 显示格式化说明");
        Serial.println("sd.init      - 重新初始化SD卡");
        Serial.println("sd.help      - 显示此帮助信息");
        Serial.println("");
        Serial.println("💡 更多SD卡操作请使用主命令系统的 'help' 查看");
        return true;
    }
    
    Serial.println("❌ 未知SD卡命令: " + command);
    Serial.println("输入 'sd.help' 查看可用命令");
    return false;
}

// 显示SD卡目录结构信息
void SDManager::printDirectoryStructure() {
    if (!_initialized) {
        Serial.println("❌ SD卡未初始化");
        return;
    }
    
    Serial.println("========== SD卡目录结构 ==========");
    Serial.println("📁 根目录文件:");
    Serial.println("  📄 " + String(SD_DEVICE_INFO_FILE) + " - 设备信息");
    
    Serial.println("📁 主要目录:");
    Serial.println("  📂 " + String(SD_DATA_DIR) + " - 数据存储");
    Serial.println("  📂 " + String(SD_CONFIG_DIR) + " - 配置文件");
    Serial.println("  📂 " + String(SD_UPDATES_DIR) + " - 升级包");
    Serial.println("  📂 " + String(SD_VOICE_DIR) + " - 语音文件");
    Serial.println("  📂 " + String(SD_LOGS_DIR) + " - 日志文件");
    
    Serial.println("📁 数据子目录:");
    Serial.println("  📂 " + String(SD_GPS_DATA_DIR) + " - GPS数据");
    Serial.println("  📂 " + String(SD_SENSOR_DATA_DIR) + " - 传感器数据");
    Serial.println("  📂 " + String(SD_SYSTEM_DATA_DIR) + " - 系统数据");
    
    Serial.println("📁 配置文件:");
    Serial.println("  📄 " + String(SD_WIFI_CONFIG_FILE) + " - WiFi配置");
    Serial.println("  📄 " + String(SD_MQTT_CONFIG_FILE) + " - MQTT配置");
    Serial.println("  📄 " + String(SD_DEVICE_CONFIG_FILE) + " - 设备配置");
    
    Serial.println("📁 特殊文件:");
    Serial.println("  📄 " + String(SD_WELCOME_VOICE_FILE) + " - 欢迎语音");
    Serial.println("  📄 " + String(SD_FIRMWARE_FILE) + " - 固件升级包");
    Serial.println("  📄 " + String(SD_UPDATE_INFO_FILE) + " - 升级信息");
    
    Serial.println("📁 日志文件:");
    Serial.println("  📄 " + String(SD_SYSTEM_LOG_FILE) + " - 系统日志");
    Serial.println("  📄 " + String(SD_ERROR_LOG_FILE) + " - 错误日志");
    Serial.println("  📄 " + String(SD_GPS_LOG_FILE) + " - GPS日志");
    
    Serial.println("================================");
}

// ========== 文件操作方法实现 ==========

bool SDManager::writeFile(const String& path, const String& content) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法写入文件: " + path);
        return false;
    }

    File file = SD_MMC.open(path, FILE_WRITE);

    if (!file) {
        debugPrint("❌ 无法创建文件: " + path);
        return false;
    }

    size_t bytesWritten = file.print(content);
    file.close();

    if (bytesWritten != content.length()) {
        debugPrint("⚠️ 文件写入不完整: " + path);
        return false;
    }

    return true;
}

bool SDManager::appendFile(const String& path, const String& content) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法追加文件: " + path);
        return false;
    }

    File file = SD_MMC.open(path, FILE_APPEND);

    if (!file) {
        debugPrint("❌ 无法打开文件进行追加: " + path);
        return false;
    }

    size_t bytesWritten = file.print(content);
    file.close();

    if (bytesWritten != content.length()) {
        debugPrint("⚠️ 文件追加不完整: " + path);
        return false;
    }

    return true;
}

String SDManager::readFile(const String& path) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法读取文件: " + path);
        return "";
    }

    File file = SD_MMC.open(path, FILE_READ);

    if (!file) {
        debugPrint("❌ 无法打开文件: " + path);
        return "";
    }

    String content = "";
    while (file.available()) {
        content += (char)file.read();
    }
    file.close();

    return content;
}

bool SDManager::deleteFile(const String& path) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法删除文件: " + path);
        return false;
    }

    return SD_MMC.remove(path);
}

bool SDManager::fileExists(const String& path) {
    if (!_initialized) {
        return false;
    }

    return SD_MMC.exists(path);
}

bool SDManager::createDir(const String& path) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法创建目录: " + path);
        return false;
    }

    // 如果目录已存在，返回true
    if (fileExists(path)) {
        return true;
    }

    return SD_MMC.mkdir(path);
}

void SDManager::listDir(const String& path) {
    if (!_initialized) {
        debugPrint("⚠️ SD卡未初始化，无法列出目录: " + path);
        return;
    }

    File root = SD_MMC.open(path);

    if (!root) {
        debugPrint("❌ 无法打开目录: " + path);
        return;
    }

    if (!root.isDirectory()) {
        debugPrint("❌ 路径不是目录: " + path);
        root.close();
        return;
    }

    Serial.println("目录内容: " + path);
    File file = root.openNextFile();
    int fileCount = 0;

    while (file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        String fileType = file.isDirectory() ? "[DIR]" : "[FILE]";
        
        Serial.printf("  %s %s (%d bytes)\n", fileType.c_str(), fileName.c_str(), fileSize);
        
        fileCount++;
        file.close();
        file = root.openNextFile();
    }

    root.close();
    Serial.printf("共找到 %d 个项目\n", fileCount);
}

// ========== 新增的文件系统操作方法实现 ==========

bool SDManager::listDirectory(const String& path) {
    if (!_initialized) {
        Serial.println("❌ SD卡未初始化");
        return false;
    }

    Serial.println("🔍 正在打开目录: " + path);

    File root = SD_MMC.open(path);

    if (!root) {
        Serial.println("❌ 无法打开目录: " + path);
        return false;
    }

    if (!root.isDirectory()) {
        Serial.println("❌ 路径不是目录: " + path);
        root.close();
        return false;
    }

    Serial.println("✅ 目录打开成功，开始读取内容...");

    File file = root.openNextFile();
    int fileCount = 0;
    int dirCount = 0;
    size_t totalSize = 0;

    while (file) {
        String fileName = file.name();
        size_t fileSize = file.size();
        
        Serial.println("📋 发现项目: " + fileName + " (大小: " + String(fileSize) + ")");
        
        if (file.isDirectory()) {
            Serial.printf("  [DIR]  %s/\n", fileName.c_str());
            dirCount++;
        } else {
            Serial.printf("  [FILE] %s (%s)\n", fileName.c_str(), formatFileSize(fileSize).c_str());
            fileCount++;
            totalSize += fileSize;
        }
        
        file.close();
        file = root.openNextFile();
    }

    root.close();
    
    Serial.println("--- 统计信息 ---");
    Serial.printf("目录: %d 个, 文件: %d 个\n", dirCount, fileCount);
    if (fileCount > 0) {
        Serial.printf("总大小: %s\n", formatFileSize(totalSize).c_str());
    }
    
    if (fileCount == 0 && dirCount == 0) {
        Serial.println("ℹ️ 目录为空或无法读取内容");
        
        // 额外的诊断信息
        Serial.println("🔧 诊断信息:");
        Serial.println("  - SD卡初始化状态: " + String(_initialized ? "已初始化" : "未初始化"));
        Serial.println("  - 剩余空间: " + String((unsigned long)getFreeSpaceMB()) + " MB");
        Serial.println("  - 总容量: " + String((unsigned long)getTotalSpaceMB()) + " MB");
    }
    
    return true;
}

bool SDManager::listDirectoryTree(const String& path, int depth, int maxDepth) {
    if (!_initialized || depth > maxDepth) {
        return false;
    }

    File root = SD_MMC.open(path);

    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return false;
    }

    // 打印当前目录
    String indent = "";
    for (int i = 0; i < depth; i++) {
        indent += "  ";
    }
    
    if (depth == 0) {
        Serial.println("📁 " + path);
    }

    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        String fullPath = path;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += fileName;
        
        if (file.isDirectory()) {
            Serial.println(indent + "├── 📁 " + fileName + "/");
            // 递归列出子目录
            listDirectoryTree(fullPath, depth + 1, maxDepth);
        } else {
            size_t fileSize = file.size();
            Serial.println(indent + "├── 📄 " + fileName + " (" + formatFileSize(fileSize) + ")");
        }
        
        file.close();
        file = root.openNextFile();
    }

    root.close();
    return true;
}

bool SDManager::createDirectory(const String& path) {
    if (!_initialized) {
        return false;
    }

    // 检查目录是否已存在
    if (directoryExists(path.c_str())) {
        Serial.println("⚠️ 目录已存在: " + path);
        return true;
    }

    // 创建父目录（如果需要）
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash > 0) {
        String parentPath = path.substring(0, lastSlash);
        if (!directoryExists(parentPath.c_str())) {
            if (!createDirectory(parentPath)) {
                return false;
            }
        }
    }

    bool result = SD_MMC.mkdir(path);
    return result;
}

bool SDManager::displayFileContent(const String& path) {
    if (!_initialized) {
        return false;
    }

    File file = SD_MMC.open(path);

    if (!file) {
        Serial.println("❌ 无法打开文件: " + path);
        return false;
    }

    if (file.isDirectory()) {
        Serial.println("❌ 路径是目录，不是文件: " + path);
        file.close();
        return false;
    }

    size_t fileSize = file.size();
    Serial.println("文件大小: " + formatFileSize(fileSize));
    
    if (fileSize > 10240) { // 10KB限制
        Serial.println("⚠️ 文件过大 (>10KB)，只显示前1024字节");
        char buffer[1025];
        size_t bytesRead = file.readBytes(buffer, 1024);
        buffer[bytesRead] = '\0';
        Serial.println("--- 文件内容开始 ---");
        Serial.print(buffer);
        Serial.println("\n--- 文件内容结束 (截断) ---");
    } else {
        Serial.println("--- 文件内容开始 ---");
        while (file.available()) {
            Serial.write(file.read());
        }
        Serial.println("\n--- 文件内容结束 ---");
    }

    file.close();
    return true;
}

bool SDManager::removeFile(const String& path) {
    if (!_initialized) {
        return false;
    }

    return SD_MMC.remove(path);
}

bool SDManager::removeDirectory(const String& path) {
    if (!_initialized) {
        return false;
    }

    File root = SD_MMC.open(path);

    if (!root || !root.isDirectory()) {
        if (root) root.close();
        return false;
    }

    // 先删除目录中的所有文件和子目录
    File file = root.openNextFile();
    while (file) {
        String fileName = file.name();
        String fullPath = path;
        if (!fullPath.endsWith("/")) fullPath += "/";
        fullPath += fileName;
        
        if (file.isDirectory()) {
            file.close();
            removeDirectory(fullPath); // 递归删除子目录
        } else {
            file.close();
            removeFile(fullPath); // 删除文件
        }
        
        file = root.openNextFile();
    }

    root.close();

    // 删除空目录
    return SD_MMC.rmdir(path);
}

// 辅助方法：格式化文件大小显示
String SDManager::formatFileSize(size_t bytes) {
    if (bytes < 1024) {
        return String(bytes) + " B";
    } else if (bytes < 1024 * 1024) {
        return String(bytes / 1024.0, 1) + " KB";
    } else {
        return String(bytes / (1024.0 * 1024.0), 1) + " MB";
    }
}

// ========== 语音文件支持函数实现 ==========
bool SDManager::hasCustomWelcomeVoice() {
    if (!_initialized) {
        return false;
    }
    return fileExists(SD_WELCOME_VOICE_FILE);
}

String SDManager::getCustomWelcomeVoicePath() {
    return String(SD_WELCOME_VOICE_FILE);
}

bool SDManager::isValidWelcomeVoiceFile() {
    if (!_initialized || !hasCustomWelcomeVoice()) {
        return false;
    }
    
    File file = SD_MMC.open(SD_WELCOME_VOICE_FILE, FILE_READ);
    if (!file) {
        return false;
    }
    
    // 检查文件大小（至少要有WAV文件头的44字节）
    if (file.size() < 44) {
        file.close();
        return false;
    }
    
    // 读取WAV文件头进行简单验证
    char header[12];
    file.read((uint8_t*)header, 12);
    file.close();
    
    // 检查RIFF和WAVE标识
    if (strncmp(header, "RIFF", 4) == 0 && strncmp(header + 8, "WAVE", 4) == 0) {
        return true;
    }
    
    return false;
}

#endif // ENABLE_SDCARD
