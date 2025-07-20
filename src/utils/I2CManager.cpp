#include "I2CManager.h"
#include <Arduino.h>

I2CManager& I2CManager::getInstance() {
    static I2CManager instance;
    return instance;
}

bool I2CManager::begin(int sda, int scl, uint32_t frequency) {
    if (_initialized && _sda == sda && _scl == scl && _frequency == frequency) {
        Serial.println("[I2C管理器] 已初始化，跳过重复初始化");
        return true;
    }

    Serial.println("[I2C管理器] 初始化 SDA=" + String(sda) + ", SCL=" + String(scl) + ", 频率=" + String(frequency) + "Hz");

    // 使用Wire1作为共享I2C总线
    _wire = &Wire1;
    
    // 如果已经初始化过，先结束之前的连接
    if (_initialized) {
        _wire->end();
        delay(10);
    }

    // 初始化I2C总线
    bool success = _wire->begin(sda, scl);
    if (success) {
        _wire->setClock(frequency);
        _sda = sda;
        _scl = scl;
        _frequency = frequency;
        _initialized = true;
        
        Serial.println("[I2C管理器] ✅ 初始化成功");
        
        // 扫描设备
        scanDevices();
        
        return true;
    } else {
        Serial.println("[I2C管理器] ❌ 初始化失败");
        return false;
    }
}

TwoWire& I2CManager::getWire() {
    if (!_initialized) {
        Serial.println("[I2C管理器] ⚠️ 未初始化，使用默认参数初始化");
        begin();
    }
    return *_wire;
}

bool I2CManager::isInitialized() const {
    return _initialized;
}

void I2CManager::scanDevices() {
    Serial.println("[I2C管理器] 扫描I2C设备...");
    
    int deviceCount = 0;
    for (byte address = 1; address < 127; address++) {
        _wire->beginTransmission(address);
        if (_wire->endTransmission() == 0) {
            Serial.println("[I2C管理器] 发现设备: 0x" + String(address, HEX));
            deviceCount++;
        }
    }
    
    if (deviceCount == 0) {
        Serial.println("[I2C管理器] 未发现I2C设备");
    } else {
        Serial.println("[I2C管理器] 共发现 " + String(deviceCount) + " 个I2C设备");
    }
}

// 全局访问函数
TwoWire& getSharedWire() {
    return I2CManager::getInstance().getWire();
}

bool initSharedI2C(int sda, int scl, uint32_t frequency) {
    return I2CManager::getInstance().begin(sda, scl, frequency);
}
