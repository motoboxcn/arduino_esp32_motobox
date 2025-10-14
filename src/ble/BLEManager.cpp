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
    , pTelemetryCharacteristic(nullptr)
    , serverCallbacks(nullptr)
    , charCallbacks(nullptr)
    , isInitialized(false)
    , lastUpdateTime(0)
    , deviceName("")
    , isConnected(false)
    , isAdvertising(false)
    , lastTelemetryData("")
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
    
    // 创建统一遥测特征值
    pTelemetryCharacteristic = pService->createCharacteristic(
        BLE_CHAR_TELEMETRY_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pTelemetryCharacteristic->setCallbacks(charCallbacks);
    pTelemetryCharacteristic->addDescriptor(new BLE2902());
    
    // 启动服务
    pService->start();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] ✅ BLE特征值创建成功（统一遥测数据）");
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
    
    unsigned long currentTime = millis();
    if (currentTime - lastUpdateTime < BLE_UPDATE_INTERVAL) {
        return;
    }
    
    lastUpdateTime = currentTime;
    
    // 这里可以添加定期数据更新逻辑
    // 目前数据更新通过外部调用updateTelemetryData方法实现
}

void BLEManager::updateTelemetryData(const ble_device_state_t& deviceState) {
    if (!isInitialized || !pTelemetryCharacteristic) {
        return;
    }
    
    // 生成JSON数据
    String jsonData = telemetryDataToJSON(deviceState);
    
    // 检查数据是否有变化
    if (jsonData == lastTelemetryData) {
        return;
    }
    
    lastTelemetryData = jsonData;
    
    // 设置特征值并通知
    pTelemetryCharacteristic->setValue(jsonData.c_str());
    pTelemetryCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 遥测数据已更新: %d 字节\n", jsonData.length());
    #endif
}

String BLEManager::telemetryDataToJSON(const ble_device_state_t& deviceState) {
    StaticJsonDocument<2048> doc;
    
    // 设备基础信息
    doc["device_id"] = deviceState.device_id;
    doc["timestamp"] = millis();
    doc["firmware"] = deviceState.firmware;
    doc["hardware"] = deviceState.hardware;
    doc["power_mode"] = deviceState.power_mode;
    
    // 位置数据
    if (deviceState.location.valid) {
        JsonObject location = doc.createNestedObject("location");
        location["lat"] = deviceState.location.lat;
        location["lng"] = deviceState.location.lng;
        location["alt"] = deviceState.location.altitude;
        location["speed"] = deviceState.location.speed;
        location["course"] = deviceState.location.heading;
        location["satellites"] = deviceState.location.satellites;
        location["hdop"] = deviceState.location.hdop;
        location["timestamp"] = deviceState.location.timestamp;
    }
    
    // 传感器数据
    JsonObject sensors = doc.createNestedObject("sensors");
    
    // IMU数据
    if (deviceState.sensors.imu.valid) {
        JsonObject imu = sensors.createNestedObject("imu");
        imu["accel_x"] = deviceState.sensors.imu.accel_x;
        imu["accel_y"] = deviceState.sensors.imu.accel_y;
        imu["accel_z"] = deviceState.sensors.imu.accel_z;
        imu["gyro_x"] = deviceState.sensors.imu.gyro_x;
        imu["gyro_y"] = deviceState.sensors.imu.gyro_y;
        imu["gyro_z"] = deviceState.sensors.imu.gyro_z;
        imu["roll"] = deviceState.sensors.imu.roll;
        imu["pitch"] = deviceState.sensors.imu.pitch;
        imu["yaw"] = deviceState.sensors.imu.yaw;
        imu["timestamp"] = deviceState.sensors.imu.timestamp;
    }
    
    // 罗盘数据
    if (deviceState.sensors.compass.valid) {
        JsonObject compass = sensors.createNestedObject("compass");
        compass["heading"] = deviceState.sensors.compass.heading;
        compass["mag_x"] = deviceState.sensors.compass.mag_x;
        compass["mag_y"] = deviceState.sensors.compass.mag_y;
        compass["mag_z"] = deviceState.sensors.compass.mag_z;
        compass["timestamp"] = deviceState.sensors.compass.timestamp;
    }
    
    // 系统状态
    JsonObject system = doc.createNestedObject("system");
    system["battery"] = deviceState.system.battery_voltage;
    system["battery_pct"] = deviceState.system.battery_percentage;
    system["charging"] = deviceState.system.is_charging;
    system["external_power"] = deviceState.system.external_power;
    system["signal"] = deviceState.system.signal_strength;
    system["uptime"] = deviceState.system.uptime;
    system["free_heap"] = deviceState.system.free_heap;
    
    // 模块状态
    JsonObject modules = doc.createNestedObject("modules");
    modules["wifi"] = deviceState.modules.wifi_ready;
    modules["ble"] = deviceState.modules.ble_ready;
    modules["gsm"] = deviceState.modules.gsm_ready;
    modules["gnss"] = deviceState.modules.gnss_ready;
    modules["imu"] = deviceState.modules.imu_ready;
    modules["compass"] = deviceState.modules.compass_ready;
    modules["sd"] = deviceState.modules.sd_ready;
    modules["audio"] = deviceState.modules.audio_ready;
    
    // 存储信息
    JsonObject storage = doc.createNestedObject("storage");
    storage["size_mb"] = deviceState.storage.size_mb;
    storage["free_mb"] = deviceState.storage.free_mb;
    
    return doc.as<String>();
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
