#include "compass/Compass.h"

// TAG
static const char *TAG = "Compass";


#ifdef ENABLE_COMPASS
// 使用共享I2C管理器，不需要指定引脚
Compass compass;

// 初始化全局罗盘数据
compass_data_t compass_data = {
    .x = 0,
    .y = 0,
    .z = 0,
    .heading = 0,
    .headingRadians = 0,
    .direction = NORTH,
    .directionStr = "N",
    .directionName = "North",
    .directionCN = "北",
    .isValid = false,
    .timestamp = 0
};

// 方向字符串数组
static const char* DIRECTION_STRS[] = {"N", "NE", "E", "SE", "S", "SW", "W", "NW"};
static const char* DIRECTION_NAMES[] = {"North", "Northeast", "East", "Southeast", 
                                       "South", "Southwest", "West", "Northwest"};
static const char* DIRECTION_CN[] = {"北", "东北", "东", "东南", "南", "西南", "西", "西北"};

/**
 * @brief 工具函数实现
 */
float normalizeHeading(float heading) {
    while (heading < 0) heading += 360;
    while (heading >= 360) heading -= 360;
    return heading;
}

CompassDirection getDirection(float heading) {
    heading = normalizeHeading(heading);
    int direction = (int)((heading + 22.5) / 45.0);
    if (direction >= 8) direction = 0;
    return (CompassDirection)direction;
}

const char* getDirectionStr(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_STRS[dir];
}

const char* getDirectionName(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_NAMES[dir];
}

const char* getDirectionCN(float heading) {
    CompassDirection dir = getDirection(heading);
    return DIRECTION_CN[dir];
}

void printCompassData() {
    if (!compass_data.isValid) {
        Serial.printf("[%s] 数据无效\n", TAG);
        return;
    }
    
    Serial.printf("[%s] 航向: %.2f° (%.3f rad), 方向: %s (%s, %s), 磁场: X=%.2f Y=%.2f Z=%.2f\n", 
        TAG,
        compass_data.heading, 
        compass_data.headingRadians,
        compass_data.directionStr, 
        compass_data.directionName,
        compass_data.directionCN,
        compass_data.x, 
        compass_data.y, 
        compass_data.z);
}

/**
 * @brief QMC5883L 罗盘传感器驱动实现
 */
Compass::Compass() {
    _declination = -6.5f;  // 默认磁偏角，需要根据地理位置调整
    _initialized = false;
    _lastReadTime = 0;  
    _lastDebugPrintTime = 0;
}

bool Compass::begin() {
    Serial.printf("[%s] 初始化指南针，磁偏角=%.2f°\n", TAG, _declination);
    
    // 使用共享I2C管理器（如果IMU已经初始化过，这里会跳过重复初始化）
    if (!I2CManager::getInstance().isInitialized()) {
        ESP_LOGE(TAG, "共享I2C未初始化，请先初始化I2C管理器!");
        return false;
    }
    Serial.printf("[%s] 使用共享I2C总线\n", TAG);

    delay(100);  // 给一些初始化时间
    
    // 初始化QMC5883L
    qmc.init();
    qmc.setCalibrationOffsets(0, 0, 0);
    qmc.setCalibrationScales(1.0, 1.0, 1.0);
    
    _initialized = true;
    device_state.compassReady = true;
    
    Serial.printf("[%s] 初始化完成\n", TAG);
    return true;
}

bool Compass::update() {
    if (!_initialized) {
        return false;
    }

    qmc.read();
    int16_t x, y, z;
    getRawData(x, y, z);
    
    float heading = calculateHeading(x, y);
    updateCompassData(x, y, z, heading);
    
    return true;
}

void Compass::loop() {
    if (!_initialized) {
        return;
    }

    // 更新数据
    update();

    // 定期打印调试信息
    if (millis() - _lastDebugPrintTime > 2000) {
        _lastDebugPrintTime = millis();
        printCompassData();
    }
}

float Compass::getHeading() {
    return compass_data.heading;
}

float Compass::getHeadingRadians() {
    return compass_data.headingRadians;
}

CompassDirection Compass::getCurrentDirection() {
    return compass_data.direction;
}

const char* Compass::getCurrentDirectionStr() {
    return compass_data.directionStr;
}

const char* Compass::getCurrentDirectionName() {
    return compass_data.directionName;
}

const char* Compass::getCurrentDirectionCN() {
    return compass_data.directionCN;
}

bool Compass::calibrate() {
    if (!_initialized) {
        Serial.printf("[%s] 罗盘未初始化，无法校准\n", TAG);
        return false;
    }
    
    Serial.printf("[%s] 开始校准，请旋转模块...\n", TAG);
    qmc.calibrate();
    Serial.printf("[%s] 校准完成，请将以下参数写入代码：\n", TAG);
    
    Serial.printf("qmc.setCalibrationOffsets(%d, %d, %d);\n",
        qmc.getCalibrationOffset(0), qmc.getCalibrationOffset(1), qmc.getCalibrationOffset(2));
    Serial.printf("qmc.setCalibrationScales(%.2f, %.2f, %.2f);\n",
        qmc.getCalibrationScale(0), qmc.getCalibrationScale(1), qmc.getCalibrationScale(2));
    
    return true;
}

void Compass::setCalibration(int xOffset, int yOffset, int zOffset, float xScale, float yScale, float zScale) {
    qmc.setCalibrationOffsets(xOffset, yOffset, zOffset);
    qmc.setCalibrationScales(xScale, yScale, zScale);
    Serial.printf("[%s] 校准参数已设置\n", TAG);
}

void Compass::getRawData(int16_t &x, int16_t &y, int16_t &z) {
    x = qmc.getX();
    y = qmc.getY();
    z = qmc.getZ();
}

void Compass::setDeclination(float declination) {
    _declination = declination;
    Serial.printf("[%s] 磁偏角设置为: %.2f°\n", TAG, _declination);
}

float Compass::getDeclination() {
    return _declination;
}

bool Compass::isInitialized() {
    return _initialized;
}

bool Compass::isDataValid() {
    return compass_data.isValid;
}

void Compass::reset() {
    _initialized = false;
    device_state.compassReady = false;
    compass_data.isValid = false;
    Serial.printf("[%s] 罗盘已重置\n", TAG);
}

float Compass::calculateHeading(int16_t x, int16_t y) {
    float heading = atan2(y, x) * 180.0 / PI;
    heading += _declination;  // 应用磁偏角校正
    return normalizeHeading(heading);
}

void Compass::updateCompassData(int16_t x, int16_t y, int16_t z, float heading) {
    compass_data.x = x;
    compass_data.y = y;
    compass_data.z = z;
    compass_data.heading = heading;
    compass_data.headingRadians = heading * PI / 180.0;
    compass_data.direction = getDirection(heading);
    compass_data.directionStr = DIRECTION_STRS[compass_data.direction];
    compass_data.directionName = DIRECTION_NAMES[compass_data.direction];
    compass_data.directionCN = DIRECTION_CN[compass_data.direction];
    compass_data.isValid = true;
    compass_data.timestamp = millis();
}

void Compass::enterLowPowerMode()
{
    if (!_initialized) return;
    
    // QMC5883L进入待机模式
    // 通过设置控制寄存器进入低功耗模式
    TwoWire& wire = getSharedWire();
    wire.beginTransmission(0x0D); // QMC5883L地址
    wire.write(0x09); // 控制寄存器1
    wire.write(0x00); // 待机模式
    wire.endTransmission();
    
    Serial.printf("[%s] 进入低功耗模式\n", TAG);
}

void Compass::exitLowPowerMode()
{
    if (!_initialized) return;
    
    // 重新初始化QMC5883L
    qmc.init();
    delay(50); // 等待传感器稳定
    
    Serial.printf("[%s] 退出低功耗模式\n", TAG);
}

bool Compass::configureForDeepSleep()
{
    if (!_initialized) return false;
    
    // QMC5883L进入最低功耗模式
    enterLowPowerMode();
    
    Serial.printf("[%s] 已配置深度睡眠模式\n", TAG);
    return true;
}

#endif