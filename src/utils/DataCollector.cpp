#include "DataCollector.h"

DataCollector dataCollector;

DataCollector::DataCollector() {
    current_mode = MODE_NORMAL;
    collecting = false;
    transmission_enabled = true;
    debug_enabled = false;
    
    last_collection_time = 0;
    last_transmission_time = 0;
    collection_interval = 1000;      // 1秒采集一次
    transmission_interval = 5000;    // 5秒传输一次
    
    // 初始化IMU统计数据
    resetIMUStats();
}

void DataCollector::begin() {
    Serial.println("[数据采集] 初始化开始");
    
    updateCollectionInterval();
    
    // 重置所有数据
    memset(&current_sensor_data, 0, sizeof(sensor_data_t));
    resetIMUStats();
    
    Serial.printf("[数据采集] 模式: %s\n", 
        current_mode == MODE_NORMAL ? "正常模式(5秒)" : "运动模式(1秒)");
    Serial.printf("[数据采集] 采集间隔: %lu ms\n", collection_interval);
    Serial.printf("[数据采集] 传输间隔: %lu ms\n", transmission_interval);
    
    collecting = true;
    Serial.println("[数据采集] 初始化完成");
}

void DataCollector::loop() {
    if (!collecting) return;
    
    unsigned long now = millis();
    
    // 数据采集
    if (now - last_collection_time >= collection_interval) {
        collectGPSData();
        collectIMUData();
        collectCompassData();
        collectSystemData();
        
        last_collection_time = now;
        
        if (debug_enabled) {
            debugPrint("数据采集完成");
        }
    }
    
    // 数据传输
    if (transmission_enabled && (now - last_transmission_time >= transmission_interval)) {
        transmitData();
        last_transmission_time = now;
    }
}

void DataCollector::setMode(DataCollectionMode mode) {
    if (current_mode != mode) {
        current_mode = mode;
        updateCollectionInterval();
        
        Serial.printf("[数据采集] 模式切换: %s\n", 
            mode == MODE_NORMAL ? "正常模式(5秒)" : "运动模式(1秒)");
        
        // 重置统计数据
        resetIMUStats();
    }
}

void DataCollector::updateCollectionInterval() {
    switch (current_mode) {
        case MODE_NORMAL:
            transmission_interval = 5000;  // 5秒传输
            break;
        case MODE_SPORT:
            transmission_interval = 1000;  // 1秒传输
            break;
    }
    
    // 采集频率始终为1秒，确保数据及时性
    collection_interval = 1000;
}

void DataCollector::collectGPSData() {
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    
    if (air780eg.isInitialized() && air780eg.getGNSS().isDataValid()) {
        current_sensor_data.gps.latitude = air780eg.getGNSS().getLatitude();
        current_sensor_data.gps.longitude = air780eg.getGNSS().getLongitude();
        current_sensor_data.gps.altitude = air780eg.getGNSS().getAltitude();
        current_sensor_data.gps.speed = air780eg.getGNSS().getSpeed();
        current_sensor_data.gps.course = air780eg.getGNSS().getCourse();
        current_sensor_data.gps.satellites = air780eg.getGNSS().getSatelliteCount();
        current_sensor_data.gps.hdop = air780eg.getGNSS().getHDOP();
        current_sensor_data.gps.valid = true;
        current_sensor_data.gps.timestamp = millis();
        
        debugPrint("GPS数据更新: " + String(current_sensor_data.gps.latitude, 6) + 
                  "," + String(current_sensor_data.gps.longitude, 6));
    } else {
        current_sensor_data.gps.valid = false;
        debugPrint("GPS数据无效");
    }
    #endif
}

void DataCollector::collectIMUData() {
    #ifdef ENABLE_IMU
    extern IMU imu;
    
    // 获取当前IMU数据并更新统计
    updateIMUStatistics();
    
    debugPrint("IMU数据更新: 加速度(" + String(imu.getAccelX(), 3) + "," + 
              String(imu.getAccelY(), 3) + "," + String(imu.getAccelZ(), 3) + ")");
    #endif
}

void DataCollector::updateIMUStatistics() {
    #ifdef ENABLE_IMU
    extern IMU imu;
    
    unsigned long now = millis();
    
    // 初始化统计数据
    if (!imu_statistics.valid) {
        imu_statistics.accel_x_min = imu_statistics.accel_x_max = imu.getAccelX();
        imu_statistics.accel_y_min = imu_statistics.accel_y_max = imu.getAccelY();
        imu_statistics.accel_z_min = imu_statistics.accel_z_max = imu.getAccelZ();
        
        imu_statistics.gyro_x_min = imu_statistics.gyro_x_max = imu.getGyroX();
        imu_statistics.gyro_y_min = imu_statistics.gyro_y_max = imu.getGyroY();
        imu_statistics.gyro_z_min = imu_statistics.gyro_z_max = imu.getGyroZ();
        
        imu_statistics.roll_min = imu_statistics.roll_max = imu.getRoll();
        imu_statistics.pitch_min = imu_statistics.pitch_max = imu.getPitch();
        imu_statistics.yaw_min = imu_statistics.yaw_max = imu.getYaw();
        
        imu_statistics.start_time = now;
        imu_statistics.valid = true;
    }
    
    // 更新极值
    float accel_x = imu.getAccelX();
    float accel_y = imu.getAccelY();
    float accel_z = imu.getAccelZ();
    
    imu_statistics.accel_x_min = min(imu_statistics.accel_x_min, accel_x);
    imu_statistics.accel_x_max = max(imu_statistics.accel_x_max, accel_x);
    imu_statistics.accel_y_min = min(imu_statistics.accel_y_min, accel_y);
    imu_statistics.accel_y_max = max(imu_statistics.accel_y_max, accel_y);
    imu_statistics.accel_z_min = min(imu_statistics.accel_z_min, accel_z);
    imu_statistics.accel_z_max = max(imu_statistics.accel_z_max, accel_z);
    
    float gyro_x = imu.getGyroX();
    float gyro_y = imu.getGyroY();
    float gyro_z = imu.getGyroZ();
    
    imu_statistics.gyro_x_min = min(imu_statistics.gyro_x_min, gyro_x);
    imu_statistics.gyro_x_max = max(imu_statistics.gyro_x_max, gyro_x);
    imu_statistics.gyro_y_min = min(imu_statistics.gyro_y_min, gyro_y);
    imu_statistics.gyro_y_max = max(imu_statistics.gyro_y_max, gyro_y);
    imu_statistics.gyro_z_min = min(imu_statistics.gyro_z_min, gyro_z);
    imu_statistics.gyro_z_max = max(imu_statistics.gyro_z_max, gyro_z);
    
    float roll = imu.getRoll();
    float pitch = imu.getPitch();
    float yaw = imu.getYaw();
    
    imu_statistics.roll_min = min(imu_statistics.roll_min, roll);
    imu_statistics.roll_max = max(imu_statistics.roll_max, roll);
    imu_statistics.pitch_min = min(imu_statistics.pitch_min, pitch);
    imu_statistics.pitch_max = max(imu_statistics.pitch_max, pitch);
    imu_statistics.yaw_min = min(imu_statistics.yaw_min, yaw);
    imu_statistics.yaw_max = max(imu_statistics.yaw_max, yaw);
    
    imu_statistics.last_update = now;
    
    // 更新当前数据结构
    current_sensor_data.imu_stats = imu_statistics;
    #endif
}

void DataCollector::collectCompassData() {
    #ifdef ENABLE_COMPASS
    extern Compass compass;
    
    if (compass.isDataValid()) {
        current_sensor_data.compass.heading = compass.getHeading();
        current_sensor_data.compass.x = compass_data.x;
        current_sensor_data.compass.y = compass_data.y;
        current_sensor_data.compass.z = compass_data.z;
        current_sensor_data.compass.valid = true;
        current_sensor_data.compass.timestamp = millis();
        
        debugPrint("罗盘数据更新: 航向 " + String(current_sensor_data.compass.heading, 1) + "°");
    } else {
        current_sensor_data.compass.valid = false;
        debugPrint("罗盘数据无效");
    }
    #endif
}

void DataCollector::collectSystemData() {
    current_sensor_data.system.mode = current_mode;
    current_sensor_data.system.uptime = millis();
    
    // 获取电池电压
    #ifdef ENABLE_BAT
    extern BAT bat;
    current_sensor_data.system.battery_voltage = bat.getVoltage();
    #endif
    
    // 获取信号强度 - 暂时设为0，需要Air780EG库支持
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    if (air780eg.isInitialized()) {
        current_sensor_data.system.signal_strength = 0; // TODO: 实现信号强度获取
    }
    #endif
    
    debugPrint("系统数据更新: 电池 " + String(current_sensor_data.system.battery_voltage, 2) + "V");
}

void DataCollector::resetIMUStats() {
    memset(&imu_statistics, 0, sizeof(imu_stats_t));
    imu_statistics.valid = false;
    Serial.println("[数据采集] IMU统计数据已重置");
}

void DataCollector::startCollection() {
    collecting = true;
    resetIMUStats();
    Serial.println("[数据采集] 开始采集");
}

void DataCollector::stopCollection() {
    collecting = false;
    Serial.println("[数据采集] 停止采集");
}

void DataCollector::debugPrint(const String& message) {
    if (debug_enabled) {
        Serial.println("[数据采集] " + message);
    }
}

String DataCollector::getDataJSON() {
    DynamicJsonDocument doc(2048);
    
    // 设备信息
    doc["device_id"] = get_device_state()->device_id;
    doc["timestamp"] = millis();
    doc["mode"] = (current_mode == MODE_NORMAL) ? "normal" : "sport";
    
    // GPS数据
    if (current_sensor_data.gps.valid) {
        JsonObject gps = doc.createNestedObject("gps");
        gps["lat"] = current_sensor_data.gps.latitude;
        gps["lng"] = current_sensor_data.gps.longitude;
        gps["alt"] = current_sensor_data.gps.altitude;
        gps["speed"] = current_sensor_data.gps.speed;
        gps["course"] = current_sensor_data.gps.course;
        gps["satellites"] = current_sensor_data.gps.satellites;
        gps["hdop"] = current_sensor_data.gps.hdop;
    }
    
    // IMU统计数据
    if (current_sensor_data.imu_stats.valid) {
        JsonObject imu = doc.createNestedObject("imu_stats");
        
        JsonObject accel = imu.createNestedObject("accel");
        accel["x_min"] = current_sensor_data.imu_stats.accel_x_min;
        accel["x_max"] = current_sensor_data.imu_stats.accel_x_max;
        accel["y_min"] = current_sensor_data.imu_stats.accel_y_min;
        accel["y_max"] = current_sensor_data.imu_stats.accel_y_max;
        accel["z_min"] = current_sensor_data.imu_stats.accel_z_min;
        accel["z_max"] = current_sensor_data.imu_stats.accel_z_max;
        
        JsonObject gyro = imu.createNestedObject("gyro");
        gyro["x_min"] = current_sensor_data.imu_stats.gyro_x_min;
        gyro["x_max"] = current_sensor_data.imu_stats.gyro_x_max;
        gyro["y_min"] = current_sensor_data.imu_stats.gyro_y_min;
        gyro["y_max"] = current_sensor_data.imu_stats.gyro_y_max;
        gyro["z_min"] = current_sensor_data.imu_stats.gyro_z_min;
        gyro["z_max"] = current_sensor_data.imu_stats.gyro_z_max;
        
        JsonObject attitude = imu.createNestedObject("attitude");
        attitude["roll_min"] = current_sensor_data.imu_stats.roll_min;
        attitude["roll_max"] = current_sensor_data.imu_stats.roll_max;
        attitude["pitch_min"] = current_sensor_data.imu_stats.pitch_min;
        attitude["pitch_max"] = current_sensor_data.imu_stats.pitch_max;
        attitude["yaw_min"] = current_sensor_data.imu_stats.yaw_min;
        attitude["yaw_max"] = current_sensor_data.imu_stats.yaw_max;
        
        imu["duration"] = current_sensor_data.imu_stats.last_update - current_sensor_data.imu_stats.start_time;
    }
    
    // 罗盘数据
    if (current_sensor_data.compass.valid) {
        JsonObject compass = doc.createNestedObject("compass");
        compass["heading"] = current_sensor_data.compass.heading;
        compass["x"] = current_sensor_data.compass.x;
        compass["y"] = current_sensor_data.compass.y;
        compass["z"] = current_sensor_data.compass.z;
    }
    
    // 系统状态
    JsonObject system = doc.createNestedObject("system");
    system["battery"] = current_sensor_data.system.battery_voltage;
    system["signal"] = current_sensor_data.system.signal_strength;
    system["uptime"] = current_sensor_data.system.uptime;
    
    String json;
    serializeJson(doc, json);
    return json;
}

void DataCollector::transmitData() {
    if (!transmission_enabled) return;
    
    String json = getDataJSON();
    String topic = generateMQTTTopic("telemetry");
    
    if (publishToMQTT(topic, json)) {
        debugPrint("数据传输成功: " + String(json.length()) + " 字节");
        
        // 传输成功后重置IMU统计（为下一个周期准备）
        if (current_mode == MODE_NORMAL) {
            resetIMUStats();
        }
    } else {
        debugPrint("数据传输失败");
    }
}

bool DataCollector::publishToMQTT(const String& topic, const String& payload) {
    #ifdef USE_AIR780EG_GSM
    extern Air780EG air780eg;
    
    if (air780eg.isInitialized() && air780eg.getMQTT().isConnected()) {
        return air780eg.getMQTT().publish(topic, payload, 0, false);
    }
    #endif
    
    return false;
}

String DataCollector::generateMQTTTopic(const String& data_type) {
    return "vehicle/v1/" + get_device_state()->device_id + "/" + data_type + "/sensors";
}

sensor_data_t DataCollector::getCurrentData() {
    return current_sensor_data;
}

void DataCollector::printStats() {
    Serial.println("=== 数据采集统计 ===");
    Serial.printf("模式: %s\n", current_mode == MODE_NORMAL ? "正常模式" : "运动模式");
    Serial.printf("采集状态: %s\n", collecting ? "运行中" : "已停止");
    Serial.printf("传输状态: %s\n", transmission_enabled ? "启用" : "禁用");
    Serial.printf("传输间隔: %lu ms\n", transmission_interval);
    
    if (current_sensor_data.gps.valid) {
        Serial.printf("GPS: %.6f,%.6f (卫星:%d)\n", 
            current_sensor_data.gps.latitude, 
            current_sensor_data.gps.longitude,
            current_sensor_data.gps.satellites);
    }
    
    if (current_sensor_data.compass.valid) {
        Serial.printf("罗盘: %.1f°\n", current_sensor_data.compass.heading);
    }
    
    if (imu_statistics.valid) {
        Serial.printf("IMU统计时长: %lu ms\n", 
            imu_statistics.last_update - imu_statistics.start_time);
    }
    
    Serial.println("==================");
}
