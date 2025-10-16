#include "BLEManager.h"

#ifdef ENABLE_BLE

#include "utils/DebugUtils.h"

// 外部Device对象声明
extern Device device;

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

void BLEManager::updateGPSData() {
    if (!isInitialized || !pGPSCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastGPSUpdateTime < BLE_GPS_UPDATE_INTERVAL) {
        return;
    }
    
    // 直接从Device对象获取JSON数据
    String jsonData = device.getBLEGPSJSON();
    
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

void BLEManager::updateIMUData() {
    if (!isInitialized || !pIMUCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastIMUUpdateTime < BLE_IMU_UPDATE_INTERVAL) {
        return;
    }
    
    // 直接从Device对象获取JSON数据
    String jsonData = device.getBLEIMUJSON();
    
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

void BLEManager::updateCompassData() {
    if (!isInitialized || !pCompassCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastCompassUpdateTime < BLE_COMPASS_UPDATE_INTERVAL) {
        return;
    }
    
    // 直接从Device对象获取JSON数据
    String jsonData = device.getBLECompassJSON();
    
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

void BLEManager::updateSystemData() {
    if (!isInitialized || !pSystemCharacteristic) {
        return;
    }
    
    unsigned long currentTime = millis();
    if (currentTime - lastSystemUpdateTime < BLE_SYSTEM_UPDATE_INTERVAL) {
        return;
    }
    
    // 直接从Device对象获取JSON数据
    String jsonData = device.getBLESystemJSON();
    
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

void BLEManager::updateAllData() {
    if (!isInitialized || !isConnected) {
        return;
    }

    // 直接更新各模块数据（不再需要传递device_state参数）
    updateGPSData();
    updateIMUData();
    updateCompassData();
    updateSystemData();
}

// ==================== JSON数据生成方法已移至Device类 ====================
// 现在直接使用 device.getBLEGPSJSON(), device.getBLEIMUJSON() 等方法


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
