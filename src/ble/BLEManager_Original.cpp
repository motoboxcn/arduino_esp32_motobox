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
    , pBatteryCharacteristic(nullptr)
    , pIMUCharacteristic(nullptr)
    , pFusionCharacteristic(nullptr)
    , pSystemCharacteristic(nullptr)
    , serverCallbacks(nullptr)
    , charCallbacks(nullptr)
    , isInitialized(false)
    , lastUpdateTime(0)
    , deviceName("")
    , isConnected(false)
    , isAdvertising(false)
{
    // 初始化数据结构
    memset(&lastGPSData, 0, sizeof(lastGPSData));
    memset(&lastBatteryData, 0, sizeof(lastBatteryData));
    memset(&lastIMUData, 0, sizeof(lastIMUData));
    memset(&lastFusionData, 0, sizeof(lastFusionData));
    memset(&lastSystemStatus, 0, sizeof(lastSystemStatus));
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
    
    // 创建GPS位置特征值
    pGPSCharacteristic = pService->createCharacteristic(
        BLE_CHAR_GPS_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pGPSCharacteristic->setCallbacks(charCallbacks);
    pGPSCharacteristic->addDescriptor(new BLE2902());
    
    // 创建电池电量特征值
    pBatteryCharacteristic = pService->createCharacteristic(
        BLE_CHAR_BATTERY_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pBatteryCharacteristic->setCallbacks(charCallbacks);
    pBatteryCharacteristic->addDescriptor(new BLE2902());
    
    // 创建IMU倾角特征值
    pIMUCharacteristic = pService->createCharacteristic(
        BLE_CHAR_IMU_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pIMUCharacteristic->setCallbacks(charCallbacks);
    pIMUCharacteristic->addDescriptor(new BLE2902());
    
    // 创建融合定位特征值
    pFusionCharacteristic = pService->createCharacteristic(
        BLE_CHAR_FUSION_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pFusionCharacteristic->setCallbacks(charCallbacks);
    pFusionCharacteristic->addDescriptor(new BLE2902());
    
    // 创建系统状态特征值
    pSystemCharacteristic = pService->createCharacteristic(
        BLE_CHAR_SYSTEM_UUID,
        BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
    );
    pSystemCharacteristic->setCallbacks(charCallbacks);
    pSystemCharacteristic->addDescriptor(new BLE2902());
    
    // 启动服务
    pService->start();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.println("[BLE] ✅ BLE特征值创建成功（含融合定位和系统状态）");
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
    // 目前数据更新通过外部调用updateXXXData方法实现
}

void BLEManager::updateGPSData(const BLEGPSData& data) {
    if (!isInitialized || !pGPSCharacteristic) {
        return;
    }
    
    // 检查数据是否有变化
    if (memcmp(&data, &lastGPSData, sizeof(BLEGPSData)) == 0) {
        return;
    }
    
    lastGPSData = data;
    
    String jsonData = gpsDataToJSON(data);
    pGPSCharacteristic->setValue(jsonData.c_str());
    pGPSCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] GPS数据已更新: %s\n", jsonData.c_str());
    #endif
}

void BLEManager::updateBatteryData(const BLEBatteryData& data) {
    if (!isInitialized || !pBatteryCharacteristic) {
        return;
    }
    
    // 检查数据是否有变化
    if (memcmp(&data, &lastBatteryData, sizeof(BLEBatteryData)) == 0) {
        return;
    }
    
    lastBatteryData = data;
    
    String jsonData = batteryDataToJSON(data);
    pBatteryCharacteristic->setValue(jsonData.c_str());
    pBatteryCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 电池数据已更新: %s\n", jsonData.c_str());
    #endif
}

void BLEManager::updateIMUData(const BLEIMUData& data) {
    if (!isInitialized || !pIMUCharacteristic) {
        return;
    }
    
    // 检查数据是否有变化
    if (memcmp(&data, &lastIMUData, sizeof(BLEIMUData)) == 0) {
        return;
    }
    
    lastIMUData = data;
    
    String jsonData = imuDataToJSON(data);
    pIMUCharacteristic->setValue(jsonData.c_str());
    pIMUCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] IMU数据已更新: %s\n", jsonData.c_str());
    #endif
}

void BLEManager::updateFusionData(const BLEFusionData& data) {
    if (!isInitialized || !pFusionCharacteristic) {
        return;
    }
    
    // 检查数据是否有变化（只检查关键字段以减少开销）
    if (lastFusionData.lat == data.lat && 
        lastFusionData.lng == data.lng && 
        lastFusionData.speed == data.speed &&
        lastFusionData.timestamp == data.timestamp) {
        return;
    }
    
    lastFusionData = data;
    
    String jsonData = fusionDataToJSON(data);
    pFusionCharacteristic->setValue(jsonData.c_str());
    pFusionCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 融合数据已更新: 位置(%.6f,%.6f) 速度%.1fm/s\n", 
                 data.lat, data.lng, data.speed);
    #endif
}

void BLEManager::updateSystemStatus(const BLESystemStatus& data) {
    if (!isInitialized || !pSystemCharacteristic) {
        return;
    }
    
    // 检查数据是否有变化
    if (memcmp(&data, &lastSystemStatus, sizeof(BLESystemStatus)) == 0) {
        return;
    }
    
    lastSystemStatus = data;
    
    String jsonData = systemStatusToJSON(data);
    pSystemCharacteristic->setValue(jsonData.c_str());
    pSystemCharacteristic->notify();
    
    #ifdef BLE_DEBUG_ENABLED
    Serial.printf("[BLE] 系统状态已更新: GPS:%d IMU:%d BAT:%d\n",
                 data.gps_status, data.imu_status, data.battery_status);
    #endif
}

String BLEManager::gpsDataToJSON(const BLEGPSData& data) {
    StaticJsonDocument<256> doc;
    doc["lat"] = data.latitude;
    doc["lng"] = data.longitude;
    doc["alt"] = data.altitude;
    doc["spd"] = data.speed;
    doc["crs"] = data.course;
    doc["sat"] = data.satellites;
    doc["valid"] = data.valid;
    doc["ts"] = millis();
    
    return doc.as<String>();
}

String BLEManager::batteryDataToJSON(const BLEBatteryData& data) {
    StaticJsonDocument<128> doc;
    doc["voltage"] = data.voltage;
    doc["percentage"] = data.percentage;
    doc["charging"] = data.is_charging;
    doc["external"] = data.external_power;
    doc["ts"] = millis();
    
    return doc.as<String>();
}

String BLEManager::imuDataToJSON(const BLEIMUData& data) {
    StaticJsonDocument<256> doc;
    doc["pitch"] = data.pitch;
    doc["roll"] = data.roll;
    doc["yaw"] = data.yaw;
    doc["accel"]["x"] = data.accel_x;
    doc["accel"]["y"] = data.accel_y;
    doc["accel"]["z"] = data.accel_z;
    doc["gyro"]["x"] = data.gyro_x;
    doc["gyro"]["y"] = data.gyro_y;
    doc["gyro"]["z"] = data.gyro_z;
    doc["valid"] = data.valid;
    doc["ts"] = millis();
    
    return doc.as<String>();
}

String BLEManager::fusionDataToJSON(const BLEFusionData& data) {
    StaticJsonDocument<512> doc;
    
    // 融合位置
    JsonArray pos = doc.createNestedArray("pos");
    pos.add(data.lat);
    pos.add(data.lng);
    pos.add(data.altitude);
    
    // 运动状态
    JsonArray mov = doc.createNestedArray("mov");
    mov.add(data.speed);
    mov.add(data.heading);
    
    // Madgwick原始姿态
    JsonArray raw = doc.createNestedArray("raw");
    raw.add(data.raw_roll);
    raw.add(data.raw_pitch);
    raw.add(data.raw_yaw);
    
    // Kalman滤波姿态
    JsonArray kal = doc.createNestedArray("kal");
    kal.add(data.kalman_roll);
    kal.add(data.kalman_pitch);
    kal.add(data.kalman_yaw);
    
    // 倾斜数据
    JsonArray lean = doc.createNestedArray("lean");
    lean.add(data.lean_angle);
    lean.add(data.lean_rate);
    
    // 加速度
    JsonArray acc = doc.createNestedArray("acc");
    acc.add(data.forward_accel);
    acc.add(data.lateral_accel);
    
    // 积分位置
    JsonArray ipos = doc.createNestedArray("ipos");
    ipos.add(data.pos_x);
    ipos.add(data.pos_y);
    ipos.add(data.pos_z);
    
    // 积分速度
    JsonArray ivel = doc.createNestedArray("ivel");
    ivel.add(data.vel_x);
    ivel.add(data.vel_y);
    ivel.add(data.vel_z);
    
    // 状态标志
    doc["src"] = data.source;
    doc["kf"] = data.kalman_enabled;
    doc["v"] = data.valid;
    doc["ts"] = data.timestamp;
    
    return doc.as<String>();
}

String BLEManager::systemStatusToJSON(const BLESystemStatus& data) {
    StaticJsonDocument<384> doc;
    
    // 运行状态
    doc["mode"] = data.mode;
    doc["up"] = data.uptime;
    doc["loop"] = data.loop_count;
    
    // 模块状态
    JsonObject st = doc.createNestedObject("st");
    st["gps"] = data.gps_status;
    st["imu"] = data.imu_status;
    st["bat"] = data.battery_status;
    st["fus"] = data.fusion_status;
    
    // 内存状态
    JsonObject mem = doc.createNestedObject("mem");
    mem["free"] = data.free_heap;
    mem["kb"] = data.free_heap_kb;
    
    // 统计数据
    JsonObject stats = doc.createNestedObject("stats");
    stats["dist"] = data.total_distance;
    stats["maxspd"] = data.max_speed;
    stats["maxln"] = data.max_lean_angle;
    stats["gps"] = data.gps_updates;
    stats["imu"] = data.imu_updates;
    stats["fus"] = data.fusion_updates;
    
    // 错误信息
    doc["err"] = data.error_code;
    doc["msg"] = String(data.error_msg);
    doc["ts"] = data.timestamp;
    
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
