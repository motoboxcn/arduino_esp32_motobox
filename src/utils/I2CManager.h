#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include <Wire.h>
#include "config.h"

/**
 * @brief I2C总线管理器
 * 用于管理共享的I2C总线，避免多个设备重复初始化
 */
class I2CManager {
public:
    /**
     * @brief 获取I2C管理器单例
     */
    static I2CManager& getInstance();

    /**
     * @brief 初始化I2C总线
     * @param sda SDA引脚
     * @param scl SCL引脚
     * @param frequency I2C频率，默认100kHz
     * @return 是否初始化成功
     */
    bool begin(int sda = IIC_SDA_PIN, int scl = IIC_SCL_PIN, uint32_t frequency = 50000);

    /**
     * @brief 获取Wire对象引用
     */
    TwoWire& getWire();

    /**
     * @brief 检查是否已初始化
     */
    bool isInitialized() const;

    /**
     * @brief 扫描I2C设备
     */
    void scanDevices();

private:
    I2CManager() : _initialized(false), _sda(-1), _scl(-1), _frequency(50000) {}
    ~I2CManager() = default;
    
    // 禁用拷贝构造和赋值
    I2CManager(const I2CManager&) = delete;
    I2CManager& operator=(const I2CManager&) = delete;

    bool _initialized;
    int _sda;
    int _scl;
    uint32_t _frequency;
    TwoWire* _wire;
};

// 全局访问函数
TwoWire& getSharedWire();
bool initSharedI2C(int sda = IIC_SDA_PIN, int scl = IIC_SCL_PIN, uint32_t frequency = 50000);

#endif
