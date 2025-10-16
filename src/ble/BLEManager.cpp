#include "BLEManager.h"

#ifdef ENABLE_BLE

#include "utils/DebugUtils.h"

const char* BLEManager::TAG = "BLEManager";

// 全局BLE管理器实例
BLEManager bleManager;

// ==================== MotoBoxBLEServerCallbacks 实现 ====================

void MotoBoxBLEServerCallbacks::onConnect(BLEServer* pServer) {
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] 客户端已连接");
    #endif
    
    // 更新连接状态
    bleManager.isConnected = true;
    
    // 停止广播以节省功耗
    bleManager.stopAdvertising();
}

void MotoBoxBLEServerCallbacks::onDisconnect(BLEServer* pServer) {
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] 客户端已断开连接");
    #endif
    
    // 更新连接状态
    bleManager.isConnected = false;
    
    // 延迟后重新开始广播
    delay(500);
    bleManager.startAdvertising();
}

// ==================== MotoBoxBLECharacteristicCallbacks 实现 ====================

void MotoBoxBLECharacteristicCallbacks::onRead(BLECharacteristic* pCharacteristic) {
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 客户端读取特征值: %s\n", pCharacteristic->getUUID().toString().c_str());
    #endif
}

void MotoBoxBLECharacteristicCallbacks::onWrite(BLECharacteristic* pCharacteristic) {
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 客户端写入特征值: %s\n", pCharacteristic->getUUID().toString().c_str());
    #endif
}

// ==================== BLEManager 实现 ====================

BLEManager::BLEManager() 
    : pServer(nullptr)
    , pService(nullptr)
    , pGPSCharacteristic(nullptr)
    , pIMUCharacteristic(nullptr)
    , pCompassCharacteristic(nullptr)
    , pSystemCharacteristic(nullptr)
    , serverCallbacks(nullptr)
    , charCallbacks(nullptr)
    , isInitialized(false)
    , lastGPSUpdateTime(0)
    , lastIMUUpdateTime(0)
    , lastCompassUpdateTime(0)
    , lastSystemUpdateTime(0)
    , deviceName("")
    , isConnected(false)
    , isAdvertising(false)
    , lastGPSData("")
    , lastIMUData("")
    , lastCompassData("")
    , lastSystemData("")
{
}

BLEManager::~BLEManager() {
    end();
}

bool BLEManager::begin() {
    // 使用默认设备名称
    return begin("");
}

bool BLEManager::begin(const String& deviceId) {
    if (isInitialized) {
        return true;
    }
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] 开始初始化BLE管理器...");
    #endif
    
    try {
        // 生成设备名称
        if (deviceId.length() > 0) {
            deviceName = String(BLE_DEVICE_NAME_PREFIX) + deviceId;
        } else {
            // 如果没有提供设备ID，使用默认名称
            deviceName = "MotoBox";
        }
        
        #ifdef BLE_DEBUG_ENABLED
        Serial.printf("[BLE] 设备名称: %s\n", deviceName.c_str());
        #endif
        
        // 初始化BLE设备
        BLEDevice::init(deviceName.c_str());
        
        // 创建BLE服务器
        pServer = BLEDevice::createServer();
        if (!pServer) {
            Serial.println("[BLE] ❌ 创建BLE服务器失败");
            return false;
        }
        
        // 设置服务器回调
        serverCallbacks = new MotoBoxBLEServerCallbacks();
        pServer->setCallbacks(serverCallbacks);
        
        // 创建服务和特征值
        createService();
        createCharacteristics();
        
        // 开始广播
        startAdvertising();
        
        isInitialized = true;
        
        #ifdef BLE_DEBUG_ENABLED
        Serial.println("[BLE] ✅ BLE管理器初始化成功");
        Serial.printf("[BLE] 设备名称: %s\n", deviceName.c_str());
        Serial.printf("[BLE] 服务UUID: %s\n", BLE_SERVICE_UUID);
        #endif
        
        return true;
        
    } catch (const std::exception& e) {
        Serial.printf("[BLE] ❌ BLE初始化异常: %s\n", e.what());
        return false;
    }
}

void BLEManager::end() {
    if (!isInitialized) {
        return;
    }
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] 关闭BLE管理器...");
    #endif
    
    stopAdvertising();
    
    if (pServer) {
        pServer->getAdvertising()->stop();
    }
    
    // 清理资源
    delete serverCallbacks;
    delete charCallbacks;
    
    isInitialized = false;
    isConnected = false;
    isAdvertising = false;
}

void BLEManager::createService() {
    // 创建主服务
    pService = pServer->createService(BLE_SERVICE_UUID);
    if (!pService) {
        Serial.println("[BLE] ❌ 创建BLE服务失败");
        return;
    }
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] ✅ BLE服务创建成功");
    #endif
}

void BLEManager::createCharacteristics() {
    if (!pService) {
        return;
    }
    
    // 创建特征值回调对象
    charCallbacks = new MotoBoxBLECharacteristicCallbacks();
    
    // 创建GPS位置数据特征值
    pGPSCharacteristic = pService->createCharacteristic(
        BLE_CHAR_GPS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pGPSCharacteristic->setCallbacks(charCallbacks);
    pGPSCharacteristic->addDescriptor(new BLE2902());
    
    // 创建IMU传感器数据特征值
    pIMUCharacteristic = pService->createCharacteristic(
        BLE_CHAR_IMU_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pIMUCharacteristic->setCallbacks(charCallbacks);
    pIMUCharacteristic->addDescriptor(new BLE2902());
    
    // 创建罗盘数据特征值
    pCompassCharacteristic = pService->createCharacteristic(
        BLE_CHAR_COMPASS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pCompassCharacteristic->setCallbacks(charCallbacks);
    pCompassCharacteristic->addDescriptor(new BLE2902());
    
    // 创建系统状态数据特征值
    pSystemCharacteristic = pService->createCharacteristic(
        BLE_CHAR_SYSTEM_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pSystemCharacteristic->setCallbacks(charCallbacks);
    pSystemCharacteristic->addDescriptor(new BLE2902());
    
    // 启动服务
    pService->start();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] ✅ 模块化BLE特征值创建成功（GPS + IMU + 罗盘 + 系统状态）");
    #endif
}

void BLEManager::startAdvertising() {
    if (!pServer || isAdvertising) {
        return;
    }
    
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    if (!pAdvertising) {
        return;
    }
    
    // 配置广播参数
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMaxPreferred(0x12);
    
    // 开始广播
    pAdvertising->start();
    isAdvertising = true;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] ✅ BLE广播已启动");
    #endif
}

void BLEManager::stopAdvertising() {
    if (!pServer || !isAdvertising) {
        return;
    }
    
    BLEAdvertising* pAdvertising = pServer->getAdvertising();
    if (pAdvertising) {
        pAdvertising->stop();
    }
    
    isAdvertising = false;
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] BLE广播已停止");
    #endif
}

void BLEManager::update() {
    if (!isInitialized || !isConnected) {
        return;
    }
    
    // 模块化更新不需要在这里处理
    // 各模块数据通过外部调用相应的update方法实现
    // 这里可以添加连接状态检查等逻辑
}

void BLEManager::updateGPSData(const ble_gps_data_t& gpsData) {
    if (!isInitialized || !pGPSCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastGPSUpdateTime < BLE_GPS_UPDATE_INTERVAL) {
        return;
    }
    
    // 生成JSON数据
    String jsonData = gpsDataToJSON(gpsData);
    
    // 检查数据是否有变化
    if (jsonData == lastGPSData) {
        return;
    }
    
    lastGPSData = jsonData;
    lastGPSUpdateTime = currentTime;
    
    // 设置特征值并通知
    pGPSCharacteristic->setValue(jsonData.c_str());
    pGPSCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] GPS数据已更新: %d 字节\n", jsonData.length());
    #endif
}

void BLEManager::updateIMUData(const ble_imu_data_t& imuData) {
    if (!isInitialized || !pIMUCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastIMUUpdateTime < BLE_IMU_UPDATE_INTERVAL) {
        return;
    }
    
    // 生成JSON数据
    String jsonData = imuDataToJSON(imuData);
    
    // 检查数据是否有变化
    if (jsonData == lastIMUData) {
        return;
    }
    
    lastIMUData = jsonData;
    lastIMUUpdateTime = currentTime;
    
    // 设置特征值并通知
    pIMUCharacteristic->setValue(jsonData.c_str());
    pIMUCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] IMU数据已更新: %d 字节\n", jsonData.length());
    #endif
}

void BLEManager::updateCompassData(const ble_compass_data_t& compassData) {
    if (!isInitialized || !pCompassCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastCompassUpdateTime < BLE_COMPASS_UPDATE_INTERVAL) {
        return;
    }
    
    // 生成JSON数据
    String jsonData = compassDataToJSON(compassData);
    
    // 检查数据是否有变化
    if (jsonData == lastCompassData) {
        return;
    }
    
    lastCompassData = jsonData;
    lastCompassUpdateTime = currentTime;
    
    // 设置特征值并通知
    pCompassCharacteristic->setValue(jsonData.c_str());
    pCompassCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 罗盘数据已更新: %d 字节\n", jsonData.length());
    #endif
}

void BLEManager::updateSystemData(const ble_system_data_t& systemData) {
    if (!isInitialized || !pSystemCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastSystemUpdateTime < BLE_SYSTEM_UPDATE_INTERVAL) {
        return;
    }
    
    // 生成JSON数据
    String jsonData = systemDataToJSON(systemData);
    
    // 检查数据是否有变化
    if (jsonData == lastSystemData) {
        return;
    }
    
    lastSystemData = jsonData;
    lastSystemUpdateTime = currentTime;
    
    // 设置特征值并通知
    pSystemCharacteristic->setValue(jsonData.c_str());
    pSystemCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 系统数据已更新: %d 字节\n", jsonData.length());
    #endif
}

void BLEManager::updateAllData(const ble_device_state_t& deviceState) {
    if (!isInitialized || !isConnected) {
        return;
    }
    
    // 从完整设备状态提取各模块数据并更新
    updateGPSData(extractGPSData(deviceState));
    updateIMUData(extractIMUData(deviceState));
    updateCompassData(extractCompassData(deviceState));
    updateSystemData(extractSystemData(deviceState));
}

// ==================== JSON数据生成方法 ====================

String BLEManager::gpsDataToJSON(const ble_gps_data_t& gpsData) {
    StaticJsonDocument<512> doc;
    
    doc["device_id"] = gpsData.device_id;
    doc["timestamp"] = gpsData.timestamp;
    
    JsonObject location = doc.createNestedObject("location");
    location["lat"] = gpsData.location.lat;
    location["lng"] = gpsData.location.lng;
    location["altitude"] = gpsData.location.altitude;
    location["speed"] = gpsData.location.speed;
    location["heading"] = gpsData.location.heading;
    location["satellites"] = gpsData.location.satellites;
    location["hdop"] = gpsData.location.hdop;
    location["vdop"] = gpsData.location.vdop;
    location["pdop"] = gpsData.location.pdop;
    location["fix_type"] = gpsData.location.fix_type;
    location["timestamp"] = gpsData.location.timestamp;
    location["valid"] = gpsData.location.valid;
    
    JsonObject status = doc.createNestedObject("status");
    status["fix_quality"] = gpsData.status.fix_quality;
    status["last_fix_age"] = gpsData.status.last_fix_age;
    
    return doc.as<String>();
}

String BLEManager::imuDataToJSON(const ble_imu_data_t& imuData) {
    StaticJsonDocument<512> doc;
    
    doc["device_id"] = imuData.device_id;
    doc["timestamp"] = imuData.timestamp;
    
    JsonObject imu = doc.createNestedObject("imu");
    
    JsonObject accel = imu.createNestedObject("accel");
    accel["x"] = imuData.imu.accel.x;
    accel["y"] = imuData.imu.accel.y;
    accel["z"] = imuData.imu.accel.z;
    
    JsonObject gyro = imu.createNestedObject("gyro");
    gyro["x"] = imuData.imu.gyro.x;
    gyro["y"] = imuData.imu.gyro.y;
    gyro["z"] = imuData.imu.gyro.z;
    
    JsonObject attitude = imu.createNestedObject("attitude");
    attitude["roll"] = imuData.imu.attitude.roll;
    attitude["pitch"] = imuData.imu.attitude.pitch;
    attitude["yaw"] = imuData.imu.attitude.yaw;
    
    imu["temperature"] = imuData.imu.temperature;
    imu["timestamp"] = imuData.imu.timestamp;
    imu["valid"] = imuData.imu.valid;
    
    JsonObject status = doc.createNestedObject("status");
    status["imu_ready"] = imuData.status.imu_ready;
    status["calibrated"] = imuData.status.calibrated;
    status["motion_detected"] = imuData.status.motion_detected;
    status["vibration_level"] = imuData.status.vibration_level;
    
    return doc.as<String>();
}

String BLEManager::compassDataToJSON(const ble_compass_data_t& compassData) {
    StaticJsonDocument<512> doc;
    
    doc["device_id"] = compassData.device_id;
    doc["timestamp"] = compassData.timestamp;
    
    JsonObject compass = doc.createNestedObject("compass");
    compass["heading"] = compassData.compass.heading;
    
    JsonObject magnetic = compass.createNestedObject("magnetic");
    magnetic["x"] = compassData.compass.magnetic.x;
    magnetic["y"] = compassData.compass.magnetic.y;
    magnetic["z"] = compassData.compass.magnetic.z;
    
    compass["declination"] = compassData.compass.declination;
    compass["inclination"] = compassData.compass.inclination;
    compass["field_strength"] = compassData.compass.field_strength;
    compass["timestamp"] = compassData.compass.timestamp;
    compass["valid"] = compassData.compass.valid;
    
    JsonObject status = doc.createNestedObject("status");
    status["compass_ready"] = compassData.status.compass_ready;
    status["calibrated"] = compassData.status.calibrated;
    status["interference"] = compassData.status.interference;
    status["calibration_quality"] = compassData.status.calibration_quality;
    
    return doc.as<String>();
}

String BLEManager::systemDataToJSON(const ble_system_data_t& systemData) {
    StaticJsonDocument<400> doc;  // 减小到400字节
    
    doc["device_id"] = systemData.device_id;
    doc["timestamp"] = systemData.timestamp;
    doc["firmware"] = systemData.firmware;
    doc["hardware"] = systemData.hardware;
    doc["power_mode"] = systemData.power_mode;
    
    JsonObject system = doc.createNestedObject("system");
    system["battery_pct"] = systemData.system.battery_percentage;  // 简化字段名
    system["charging"] = systemData.system.is_charging;
    system["ext_power"] = systemData.system.external_power;
    system["signal"] = systemData.system.signal_strength;
    system["uptime"] = systemData.system.uptime;
    system["heap"] = systemData.system.free_heap;
    system["temp"] = systemData.system.temperature;
    
    JsonObject modules = doc.createNestedObject("modules");
    modules["wifi"] = systemData.modules.wifi_ready;
    modules["ble"] = systemData.modules.ble_ready;
    modules["gsm"] = systemData.modules.gsm_ready;
    modules["imu"] = systemData.modules.imu_ready;
    modules["compass"] = systemData.modules.compass_ready;
    
    return doc.as<String>();
}

// ==================== 数据提取方法 ====================

ble_gps_data_t BLEManager::extractGPSData(const ble_device_state_t& deviceState) {
    ble_gps_data_t gpsData;
    
    gpsData.device_id = deviceState.device_id;
    gpsData.timestamp = deviceState.timestamp;
    
    // 逐个复制位置数据字段
    gpsData.location.lat = deviceState.location.lat;
    gpsData.location.lng = deviceState.location.lng;
    gpsData.location.altitude = deviceState.location.altitude;
    gpsData.location.speed = deviceState.location.speed;
    gpsData.location.heading = deviceState.location.heading;
    gpsData.location.satellites = deviceState.location.satellites;
    gpsData.location.hdop = deviceState.location.hdop;
    gpsData.location.vdop = deviceState.location.vdop;
    gpsData.location.pdop = deviceState.location.pdop;
    gpsData.location.fix_type = deviceState.location.fix_type;
    gpsData.location.valid = deviceState.location.valid;
    gpsData.location.timestamp = deviceState.location.timestamp;
    
    // 设置状态信息
    gpsData.status.fix_quality = deviceState.location.valid ? "3D_FIX" : "NO_FIX";
    gpsData.status.last_fix_age = millis() - deviceState.location.timestamp;
    
    return gpsData;
}

ble_imu_data_t BLEManager::extractIMUData(const ble_device_state_t& deviceState) {
    ble_imu_data_t imuData;
    
    imuData.device_id = deviceState.device_id;
    imuData.timestamp = deviceState.timestamp;
    
    // 复制IMU数据
    imuData.imu.accel.x = deviceState.sensors.imu.accel_x;
    imuData.imu.accel.y = deviceState.sensors.imu.accel_y;
    imuData.imu.accel.z = deviceState.sensors.imu.accel_z;
    
    imuData.imu.gyro.x = deviceState.sensors.imu.gyro_x;
    imuData.imu.gyro.y = deviceState.sensors.imu.gyro_y;
    imuData.imu.gyro.z = deviceState.sensors.imu.gyro_z;
    
    imuData.imu.attitude.roll = deviceState.sensors.imu.roll;
    imuData.imu.attitude.pitch = deviceState.sensors.imu.pitch;
    imuData.imu.attitude.yaw = deviceState.sensors.imu.yaw;
    
    imuData.imu.temperature = deviceState.sensors.imu.temperature;
    imuData.imu.timestamp = deviceState.sensors.imu.timestamp;
    imuData.imu.valid = deviceState.sensors.imu.valid;
    
    // 设置状态信息
    imuData.status.imu_ready = deviceState.modules.imu_ready;
    imuData.status.calibrated = true; // 假设已校准
    imuData.status.motion_detected = false; // 需要根据实际情况设置
    imuData.status.vibration_level = 0.0; // 需要根据实际情况计算
    
    return imuData;
}

ble_compass_data_t BLEManager::extractCompassData(const ble_device_state_t& deviceState) {
    ble_compass_data_t compassData;
    
    compassData.device_id = deviceState.device_id;
    compassData.timestamp = deviceState.timestamp;
    
    // 复制罗盘数据
    compassData.compass.heading = deviceState.sensors.compass.heading;
    compassData.compass.magnetic.x = deviceState.sensors.compass.mag_x;
    compassData.compass.magnetic.y = deviceState.sensors.compass.mag_y;
    compassData.compass.magnetic.z = deviceState.sensors.compass.mag_z;
    compassData.compass.declination = deviceState.sensors.compass.declination;
    compassData.compass.inclination = deviceState.sensors.compass.inclination;
    compassData.compass.field_strength = deviceState.sensors.compass.field_strength;
    compassData.compass.timestamp = deviceState.sensors.compass.timestamp;
    compassData.compass.valid = deviceState.sensors.compass.valid;
    
    // 设置状态信息
    compassData.status.compass_ready = deviceState.modules.compass_ready;
    compassData.status.calibrated = true; // 假设已校准
    compassData.status.interference = false; // 需要根据实际情况设置
    compassData.status.calibration_quality = 85; // 需要根据实际情况设置
    
    return compassData;
}

ble_system_data_t BLEManager::extractSystemData(const ble_device_state_t& deviceState) {
    ble_system_data_t systemData;
    
    systemData.device_id = deviceState.device_id;
    systemData.timestamp = deviceState.timestamp;
    systemData.firmware = deviceState.firmware;
    systemData.hardware = deviceState.hardware;
    systemData.power_mode = deviceState.power_mode;
    
    // 逐个复制系统数据字段
    systemData.system.battery_voltage = deviceState.system.battery_voltage;
    systemData.system.battery_percentage = deviceState.system.battery_percentage;
    systemData.system.is_charging = deviceState.system.is_charging;
    systemData.system.external_power = deviceState.system.external_power;
    systemData.system.signal_strength = deviceState.system.signal_strength;
    systemData.system.uptime = deviceState.system.uptime;
    systemData.system.free_heap = deviceState.system.free_heap;
    systemData.system.cpu_usage = deviceState.system.cpu_usage;
    systemData.system.temperature = deviceState.system.temperature;
    
    // 逐个复制模块状态字段
    systemData.modules.wifi_ready = deviceState.modules.wifi_ready;
    systemData.modules.ble_ready = deviceState.modules.ble_ready;
    systemData.modules.gsm_ready = deviceState.modules.gsm_ready;
    systemData.modules.imu_ready = deviceState.modules.imu_ready;
    systemData.modules.compass_ready = deviceState.modules.compass_ready;
    
    // 逐个复制网络数据字段
    systemData.network.wifi_connected = deviceState.network.wifi_connected;
    systemData.network.gsm_connected = deviceState.network.gsm_connected;
    
    return systemData;
}

int BLEManager::getConnectedClients() const {
    if (!pServer) {
        return 0;
    }
    
    return pServer->getConnectedCount();
}

void BLEManager::setDebug(bool enable) {
    // 调试功能通过编译时宏控制
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 调试模式: %s\n", enable ? "启用" : "禁用");
    #endif
}

void BLEManager::printStatus() {
    Serial.println("=== BLE状态 ===");
    Serial.printf("初始化状态: %s\n", isInitialized ? "已初始化" : "未初始化");
    Serial.printf("连接状态: %s\n", isConnected ? "已连接" : "未连接");
    Serial.printf("广播状态: %s\n", isAdvertising ? "广播中" : "未广播");
    Serial.printf("连接客户端数: %d\n", getConnectedClients());
    Serial.printf("设备名称: %s\n", deviceName.c_str());
    Serial.println("===============");
}

#endif // ENABLE_BLE
