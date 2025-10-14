#include "qmi8658.h"

#define USE_WIRE

IMU imu(IMU_INT_PIN);

imu_data_t imu_data;

volatile bool IMU::motionInterruptFlag = false;
void IRAM_ATTR IMU::motionISR()
{
    IMU::motionInterruptFlag = true;
}

IMU::IMU(int motionIntPin)
    : motionIntPin(motionIntPin),
    _debug(false),
    _lastDebugPrintTime(0),
    _highPrecisionMode(false)
{
    this->motionIntPin = motionIntPin;
    motionThreshold = MOTION_DETECTION_THRESHOLD_DEFAULT;
    motionDetectionEnabled = false;
    sampleWindow = MOTION_DETECTION_WINDOW_DEFAULT;
}

void IMU::debugPrint(const String &message)
{
    if (_debug)
    {
        Serial.println("[IMU] [debug] " + message);
    }
}

void IMU::begin()
{
    Serial.println("[IMU] 初始化开始");
#ifdef USE_WIRE
    Serial.printf("[IMU] 使用共享I2C总线, INT引脚: %d\n", motionIntPin);
    
    // 使用共享I2C管理器
    if (!I2CManager::getInstance().isInitialized()) {
        Serial.println("[IMU] ❌ 共享I2C未初始化，请先初始化I2C管理器");
        return;
    }
    
    TwoWire& wire = getSharedWire();
    if (!qmi.begin(wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA_PIN, IIC_SCL_PIN))
    {
        Serial.println("[IMU] 初始化失败");
        for (int i = 0; i < 3; i++)
        {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(wire, QMI8658_L_SLAVE_ADDRESS, IIC_SDA_PIN, IIC_SCL_PIN))
            {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2)
            {
                Serial.println("[IMU] 多次重试失败，系统将重启");
                delay(1000);
                esp_restart();
            }
        }
    }
#else
    if (!qmi.begin(IMU_CS))
    {
        Serial.println("[IMU] 初始化失败 - 请检查接线!");
        for (int i = 0; i < 3; i++)
        {
            delay(1000);
            Serial.println("[IMU] 重试初始化...");
            if (qmi.begin(IMU_CS))
            {
                Serial.println("[IMU] 重试成功");
                break;
            }
            if (i == 2)
            {
                Serial.println("[IMU] 多次重试失败，系统将重启");
                delay(1000);
                esp_restart();
            }
        }
    }
#endif

    Serial.print("[IMU] 设备ID: 0x");
    Serial.println(qmi.getChipID(), HEX);

    // 配置陀螺仪 - 896.8Hz采样率（先配置陀螺仪）
    qmi.configGyroscope(
        SensorQMI8658::GYR_RANGE_1024DPS,  // ±1024°/s量程
        SensorQMI8658::GYR_ODR_896_8Hz,    // 896.8Hz采样率
        SensorQMI8658::LPF_MODE_3          // 低通滤波器模式3
    );
    qmi.enableGyroscope();
    Serial.println("[IMU] 陀螺仪已启用 - 896.8Hz");

    // 配置加速度计 - 1000Hz采样率（与陀螺仪接近）
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz);
    qmi.enableAccelerometer();
    Serial.println("[IMU] 加速度计已启用 - 1000Hz");
    
    // 根据官方文档：同时启用加速度计和陀螺仪时，输出频率基于陀螺仪频率
    // 注意：两个传感器采样率应该尽可能接近，避免getDataReady()问题
    Serial.println("[IMU] 数据输出频率将基于陀螺仪频率 (896.8Hz)");

    // 配置三轴任意运动检测
    uint8_t modeCtrl = SensorQMI8658::ANY_MOTION_EN_X |
                       SensorQMI8658::ANY_MOTION_EN_Y |
                       SensorQMI8658::ANY_MOTION_EN_Z;
    float AnyMotionXThr = 1.0 * 1000; // 1.0g, 单位mg
    float AnyMotionYThr = 1.0 * 1000;
    float AnyMotionZThr = 1.0 * 1000;
    uint8_t AnyMotionWindow = 16; // 连续16个采样点
    qmi.configMotion(modeCtrl,
                     AnyMotionXThr, AnyMotionYThr, AnyMotionZThr, AnyMotionWindow,
                     0, 0, 0, 0, 0, 0);
    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
    // 配置中断引脚
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE);
        Serial.printf("[IMU] 运动检测中断已绑定: GPIO%d\n", motionIntPin);
    }
    Serial.println("[IMU] 运动检测初始化完成");
}

void IMU::configureMotionDetection(float threshold)
{
    // 配置三轴任意运动检测
    uint8_t modeCtrl = SensorQMI8658::ANY_MOTION_EN_X |
                       SensorQMI8658::ANY_MOTION_EN_Y |
                       SensorQMI8658::ANY_MOTION_EN_Z;

    // 将g转换为mg
    float thresholdMg = threshold * 1000;

    qmi.configMotion(modeCtrl,
                     thresholdMg, thresholdMg, thresholdMg,
                     MOTION_DETECTION_WINDOW_DEFAULT,
                     0, 0, 0, 0, 0, 0);

    qmi.enableMotionDetect(SensorQMI8658::INTERRUPT_PIN_1);
}

void IMU::disableMotionDetection()
{
    if (!motionDetectionEnabled)
        return;

    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    qmi.disableMotionDetect();
    motionDetectionEnabled = false;
    Serial.println("[IMU] 运动检测已禁用");
}

bool IMU::configureForDeepSleep()
{
    // 禁用当前的运动检测中断
    if (motionIntPin >= 0)
    {
        detachInterrupt(motionIntPin);
    }

    // 使用官方的WakeOnMotion配置，专门用于深度睡眠唤醒
    // 参数：255mg阈值（uint8_t最大值），低功耗128Hz，使用中断引脚1，默认引脚值1，抑制时间0x30
    int result = qmi.configWakeOnMotion(
        255,                                   // 255mg阈值，uint8_t最大值，比默认200mg更保守
        SensorQMI8658::ACC_ODR_LOWPOWER_128Hz, // 低功耗模式
        SensorQMI8658::INTERRUPT_PIN_1,        // 使用中断引脚1
        1,                                     // 默认引脚值为1
        0x30                                   // 增加抑制时间，减少误触发
    );

    if (result != DEV_WIRE_NONE)
    {
        Serial.println("[IMU] WakeOnMotion配置失败");
        return false;
    }

    // 重新配置中断引脚为CHANGE触发（官方例子的方式）
    if (motionIntPin >= 0)
    {
        pinMode(motionIntPin, INPUT_PULLUP);
        attachInterrupt(motionIntPin, IMU::motionISR, CHANGE); // 使用CHANGE而非特定边沿
    }

    Serial.println("[IMU] 已配置为WakeOnMotion深度睡眠模式 (阈值=255mg, 低功耗128Hz)");
    return true;
}

bool IMU::restoreFromDeepSleep()
{
    // 唤醒后适当延时，确保I2C/IMU电源和时钟ready
    delay(500); // 增加到500ms，确保电源稳定

    // 重新初始化I2C总线
    TwoWire& wire = getSharedWire();
    initSharedI2C(IIC_SDA_PIN, IIC_SCL_PIN);
    delay(50); // 等待I2C总线稳定

    // 重置设备
    if (!qmi.reset())
    {
        Serial.println("[IMU] 重置失败");
        return false;
    }
    delay(50); // 等待重置完成

    // 重新配置正常的加速度计
    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
    qmi.enableAccelerometer();
    delay(50); // 等待配置生效

    // 重新启用陀螺仪
    setGyroEnabled(true);
    delay(50); // 等待陀螺仪稳定

    // 恢复正常的运动检测配置（如果之前启用了的话）
    if (motionDetectionEnabled)
    {
        configureMotionDetection(motionThreshold);
        delay(50); // 等待运动检测配置生效
    }

    Serial.println("[IMU] 已从WakeOnMotion模式恢复到正常模式");
    return true;
}

bool IMU::checkWakeOnMotionEvent()
{
    // 使用官方例子的方式检查状态
    uint8_t status = qmi.getStatusRegister();

    if (status & SensorQMI8658::EVENT_WOM_MOTION)
    {
        Serial.println("[IMU] 检测到WakeOnMotion事件");
        return true;
    }

    return false;
}

bool IMU::isMotionDetected()
{
    if (!motionDetectionEnabled)
        return false;

    if (motionInterruptFlag)
    {
        motionInterruptFlag = false;
        uint8_t status = qmi.getStatusRegister();
        return (status & SensorQMI8658::EVENT_ANY_MOTION) != 0;
    }

    return false;
}

void IMU::setAccelPowerMode(uint8_t mode)
{
    // 配置加速度计功耗模式
    switch (mode)
    {
    case 0: // 低功耗
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_125Hz);
        Serial.println("[IMU] 加速度计设置为低功耗模式");
        break;
    case 1: // 正常
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_500Hz);
        Serial.println("[IMU] 加速度计设置为正常模式");
        break;
    case 2: // 高性能
        qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_1000Hz);
        Serial.println("[IMU] 加速度计设置为高性能模式");
        break;
    }
}

void IMU::setGyroEnabled(bool enabled)
{
    if (enabled)
    {
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,  // ±1024°/s量程
            SensorQMI8658::GYR_ODR_896_8Hz,    // 896.8Hz采样率
            SensorQMI8658::LPF_MODE_3          // 低通滤波器模式3
        );
        qmi.enableGyroscope();
        Serial.println("[IMU] 陀螺仪已启用 - 896.8Hz");
    }
    else
    {
        qmi.disableGyroscope();
        Serial.println("[IMU] 陀螺仪已禁用");
    }
}

void IMU::loop()
{
    // 高频数据读取，支持EKF算法的高频更新需求
    get_device_state()->telemetry.modules.imu_ready = true;
    
    // 添加调试：检查数据读取前的状态
    static unsigned long lastDebugTime = 0;
    static float last_accel_x = 0, last_accel_y = 0, last_accel_z = 0;
    static float last_gyro_x = 0, last_gyro_y = 0, last_gyro_z = 0;
    
    // 读取加速度计数据
    float new_accel_x, new_accel_y, new_accel_z;
    bool accel_success = qmi.getAccelerometer(new_accel_x, new_accel_y, new_accel_z);
    
    // 读取陀螺仪数据
    float new_gyro_x, new_gyro_y, new_gyro_z;
    bool gyro_success = qmi.getGyroscope(new_gyro_x, new_gyro_y, new_gyro_z);
    
    // 每5秒输出一次调试信息
    #ifdef IMU_DEBUG_ENABLED
    if (millis() - lastDebugTime > 5000) {
        Serial.printf("[IMU DEBUG] Accel read: %s, Gyro read: %s\n", 
                     accel_success ? "OK" : "FAIL", 
                     gyro_success ? "OK" : "FAIL");
        
        Serial.printf("[IMU DEBUG] Accel change: %.3f, Gyro change: %.3f\n",
                     abs(new_accel_x - last_accel_x) + abs(new_accel_y - last_accel_y) + abs(new_accel_z - last_accel_z),
                     abs(new_gyro_x - last_gyro_x) + abs(new_gyro_y - last_gyro_y) + abs(new_gyro_z - last_gyro_z));
        
        lastDebugTime = millis();
    }
    #endif
    
    // 更新数据
    imu_data.accel_x = new_accel_x;
    imu_data.accel_y = new_accel_y;
    imu_data.accel_z = new_accel_z;
    imu_data.gyro_x = new_gyro_x;
    imu_data.gyro_y = new_gyro_y;
    imu_data.gyro_z = new_gyro_z;
    
    // 保存用于下次比较
    last_accel_x = new_accel_x;
    last_accel_y = new_accel_y;
    last_accel_z = new_accel_z;
    last_gyro_x = new_gyro_x;
    last_gyro_y = new_gyro_y;
    last_gyro_z = new_gyro_z;

    // 应用传感器旋转（如果定义了）
#if defined(IMU_ROTATION)
    // 顺时针旋转90度（适用于传感器侧装）
    float temp = imu_data.accel_x;
    imu_data.accel_x = imu_data.accel_y;
    imu_data.accel_y = -temp;
#endif

    // 注释掉IMU层的姿态解算，让Fusion层处理
    // 这样可以避免双重滤波，提高精度
    // 
    // 如果需要简单的姿态角用于调试，可以保留：
    // float roll_acc = atan2(imu_data.accel_y, imu_data.accel_z) * 180 / M_PI;
    // float pitch_acc = atan2(-imu_data.accel_x, sqrt(imu_data.accel_y * imu_data.accel_y + imu_data.accel_z * imu_data.accel_z)) * 180 / M_PI;
    // imu_data.roll = ALPHA * (imu_data.roll + imu_data.gyro_x * dt) + (1.0 - ALPHA) * roll_acc;
    // imu_data.pitch = ALPHA * (imu_data.pitch + imu_data.gyro_y * dt) + (1.0 - ALPHA) * pitch_acc;
    
    // 保持简单的互补滤波用于基础姿态估计（仅用于调试和兼容性）
    float roll_acc = atan2(imu_data.accel_y, imu_data.accel_z) * 180 / M_PI;
    float pitch_acc = atan2(-imu_data.accel_x, sqrt(imu_data.accel_y * imu_data.accel_y + imu_data.accel_z * imu_data.accel_z)) * 180 / M_PI;
    
    // 使用较低的ALPHA值，减少对Fusion层的影响
    imu_data.roll = 0.5f * (imu_data.roll + imu_data.gyro_x * IMU_DT) + 0.5f * roll_acc;
    imu_data.pitch = 0.5f * (imu_data.pitch + imu_data.gyro_y * IMU_DT) + 0.5f * pitch_acc;

    imu_data.temperature = qmi.getTemperature_C();

    // 更新到统一数据管理
    extern Device device;
    device.updateIMUData(imu_data.accel_x, imu_data.accel_y, imu_data.accel_z,
                        imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z,
                        imu_data.roll, imu_data.pitch, imu_data.yaw);

    // 处理运动检测中断
    if (motionDetectionEnabled && isMotionDetected())
    {
        static unsigned long lastMotionTime = 0;
        unsigned long now = millis();
        if (now - lastMotionTime > MOTION_DETECTION_DEBOUNCE_MS)
        {
            Serial.printf("[IMU] 检测到运动! 中断引脚: GPIO%d\n", motionIntPin);
            lastMotionTime = now;
        }
    }
    
}

/**
 * @brief 检测是否有运动
 * @return true: 检测到运动, false: 未检测到
 */
bool IMU::detectMotion()
{
    float accelMagnitude = sqrt(
        imu_data.accel_x * imu_data.accel_x +
        imu_data.accel_y * imu_data.accel_y +
        imu_data.accel_z * imu_data.accel_z);
    float delta = fabs(accelMagnitude - lastAccelMagnitude);
    lastAccelMagnitude = accelMagnitude;
    accumulatedDelta += delta;
    sampleIndex++;
    if (sampleIndex >= sampleWindow)
    {
        float averageDelta = accumulatedDelta / sampleWindow;
        
        // 减少调试输出频率，只在调试模式下且需要时才输出
        #ifdef IMU_DEBUG_ENABLED
        static unsigned long last_motion_debug = 0;
        if (millis() - last_motion_debug > 10000) { // 每10秒输出一次
            Serial.printf("[IMU] 运动检测阈值: %f, 平均加速度变化: %f\n", motionThreshold, averageDelta);
            last_motion_debug = millis();
        }
        #endif
        
        bool motionDetected = averageDelta > (motionThreshold * 0.8);
        accumulatedDelta = 0;
        sampleIndex = 0;
        return motionDetected;
    }
    return false;
}

/**
 * @brief 打印IMU数据
 */
void IMU::printImuData()
{
    Serial.printf("imu_data: roll=%.2f, pitch=%.2f, yaw=%.2f, temp=%.1f°C | accel=(%.2f,%.2f,%.2f)g | gyro=(%.1f,%.1f,%.1f)°/s\n",
        imu_data.roll, imu_data.pitch, imu_data.yaw, imu_data.temperature,
        imu_data.accel_x, imu_data.accel_y, imu_data.accel_z,
        imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z);
}

// 生成精简版IMU数据JSON
String imu_data_to_json(imu_data_t &imu_data)
{
    StaticJsonDocument<256> doc;
    doc["ax"] = imu_data.accel_x; // X轴加速度
    doc["ay"] = imu_data.accel_y; // Y轴加速度
    doc["az"] = imu_data.accel_z; // Z轴加速度
    doc["gx"] = imu_data.gyro_x;  // X轴角速度
    doc["gy"] = imu_data.gyro_y;  // Y轴角速度
    doc["gz"] = imu_data.gyro_z;  // Z轴角速度
    // doc["mx"] = imu_data.mag_x;            // X轴磁场
    // doc["my"] = imu_data.mag_y;            // Y轴磁场
    // doc["mz"] = imu_data.mag_z;            // Z轴磁场
    doc["roll"] = imu_data.roll;        // 横滚角
    doc["pitch"] = imu_data.pitch;      // 俯仰角
    doc["yaw"] = imu_data.yaw;          // 航向角
    doc["temp"] = imu_data.temperature; // 温度
    return doc.as<String>();
}

float IMU::getAccelMagnitude() const {
    return sqrt(imu_data.accel_x * imu_data.accel_x + 
                imu_data.accel_y * imu_data.accel_y + 
                imu_data.accel_z * imu_data.accel_z);
}

void IMU::setHighPrecisionMode(bool enabled) {
    if (_highPrecisionMode == enabled) {
        return; // 已经是目标模式，无需改变
    }
    
    _highPrecisionMode = enabled;
    
    if (enabled) {
        Serial.println("[IMU] 切换到高精度模式");
        
        // 高精度模式：使用现有的最高采样率和更低的滤波
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_512DPS,   // 降低量程提高精度
            SensorQMI8658::GYR_ODR_896_8Hz,    // 使用现有最高采样率
            SensorQMI8658::LPF_MODE_0          // 最小滤波
        );
        
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_2G,       // 降低量程提高精度
            SensorQMI8658::ACC_ODR_1000Hz      // 使用现有最高采样率
        );
        
        // 更敏感的运动检测
        configureMotionDetection(0.02f); // 降低阈值到0.02g
        
    } else {
        Serial.println("[IMU] 切换到标准精度模式");
        
        // 标准模式：平衡精度和功耗
        qmi.configGyroscope(
            SensorQMI8658::GYR_RANGE_1024DPS,  // 标准量程
            SensorQMI8658::GYR_ODR_896_8Hz,    // 标准采样率
            SensorQMI8658::LPF_MODE_3          // 标准滤波
        );
        
        qmi.configAccelerometer(
            SensorQMI8658::ACC_RANGE_4G,       // 标准量程
            SensorQMI8658::ACC_ODR_1000Hz      // 标准采样率
        );
        
        // 标准运动检测
        configureMotionDetection(motionThreshold); // 使用默认阈值
    }
    
    Serial.printf("[IMU] 高精度模式: %s\n", enabled ? "启用" : "禁用");
}