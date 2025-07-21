#include "SDManager.h"

#ifndef ENABLE_SDCARD

// 空实现，当ENABLE_SDCARD未定义时使用

SDManager::SDManager() {}
SDManager::~SDManager() {}

bool SDManager::begin() { return false; }
void SDManager::end() {}
bool SDManager::isInitialized() { return false; }

uint64_t SDManager::getTotalSpaceMB() { return 0; }
uint64_t SDManager::getFreeSpaceMB() { return 0; }

bool SDManager::saveDeviceInfo() { return false; }
bool SDManager::recordGPSData(gnss_data_t &gnss_data) { return false; }
bool SDManager::finishGPSSession() { return false; }

bool SDManager::writeFile(const String& path, const String& content) { return false; }
bool SDManager::appendFile(const String& path, const String& content) { return false; }
String SDManager::readFile(const String& path) { return ""; }
bool SDManager::deleteFile(const String& path) { return false; }
bool SDManager::fileExists(const String& path) { return false; }
bool SDManager::createDir(const String& path) { return false; }
void SDManager::listDir(const String& path) {}

bool SDManager::listDirectory(const String& path) { return false; }
bool SDManager::listDirectoryTree(const String& path, int depth, int maxDepth) { return false; }
bool SDManager::createDirectory(const String& path) { return false; }
bool SDManager::displayFileContent(const String& path) { return false; }
bool SDManager::removeFile(const String& path) { return false; }
bool SDManager::removeDirectory(const String& path) { return false; }

bool SDManager::handleSerialCommand(const String& command) { 
    Serial.println("SD卡功能未启用 (ENABLE_SDCARD 未定义)");
    return false; 
}

bool SDManager::hasCustomWelcomeVoice() { return false; }
String SDManager::getCustomWelcomeVoicePath() { return ""; }
bool SDManager::isValidWelcomeVoiceFile() { return false; }

#endif // ENABLE_SDCARD
