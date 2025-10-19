#include "utils/serialCommand.h"
#include "utils/DataCollector.h"
#include "ota/OTAManager.h"

#ifdef ENABLE_POWER_MODE_MANAGEMENT
#include "power/PowerModeManager.h"
#endif

#ifdef ENABLE_IMU_FUSION
#include "location/FusionLocationManager.h"
#endif

// ===================== 串口命令处理函数 =====================
/**
 * 处理串口输入命令
 */
void handleSerialCommand()
{
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.length() > 0)
    {
        Serial.println(">>> 收到命令: " + command);
        if (command == "info")
        {
            Serial.println("=== 设备信息 ===");
            Serial.println("设备ID: " + device_state.device_id);
            Serial.println("固件版本: " + String(device_state.device_firmware_version));
            Serial.println("硬件版本: " + String(device_state.device_hardware_version));
            Serial.println("运行时间: " + String(millis() / 1000) + " 秒");
            Serial.println("");
            Serial.println("--- 连接状态 ---");
            Serial.println("WiFi状态: " + String(device_state.telemetry.modules.wifi_ready ? "已连接" : "未连接"));
            Serial.println("BLE状态: " + String(device_state.telemetry.modules.ble_ready ? "已连接" : "未连接"));
#ifdef ENABLE_GSM
            Serial.println("GSM状态: " + String(device_state.telemetry.modules.gsm_ready ? "就绪" : "未就绪"));
#ifdef USE_AIR780EG_GSM
            if (device_state.telemetry.modules.gsm_ready)
            {
                Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
                Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
                Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
                if (air780eg.getGNSS().isValid())
                {
                    Serial.println("GNSS状态: 已定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
                }
                else
                {
                    Serial.println("GNSS状态: 未定位 (卫星数: " + String(air780eg.getGNSS().getSatelliteCount()) + ")");
                }
            }
#endif

            // MQTT连接状态和配置信息
#ifndef DISABLE_MQTT
            Serial.println("MQTT状态: " + air780eg.getMQTT().getState());
            Serial.println("MQTT服务器: " + String(MQTT_BROKER) + ":" + String(MQTT_PORT));

            // 显示已注册的主题
            Serial.println("--- MQTT主题配置 ---");
            String deviceId = device_state.device_id;
            String baseTopic = "vehicle/v1/" + deviceId;
            Serial.println("基础主题: " + baseTopic);
            Serial.println("设备信息: " + baseTopic + "/telemetry/device");
            Serial.println("位置信息: " + baseTopic + "/telemetry/location");
            Serial.println("运动信息: " + baseTopic + "/telemetry/motion");
            Serial.println("控制命令: " + baseTopic + "/ctrl/#");
#else
            Serial.println("MQTT功能: ❌ 已禁用");
#endif
#endif
            Serial.println("");
            Serial.println("--- 传感器状态 ---");
            Serial.println("IMU状态: " + String(device_state.telemetry.modules.imu_ready ? "就绪" : "未就绪"));
            Serial.println("罗盘状态: " + String(device_state.telemetry.modules.compass_ready ? "就绪" : "未就绪"));
            Serial.println("");
            Serial.println("--- 电源状态 ---");
            Serial.println("电池电压: " + String(device_state.telemetry.system.battery_voltage) + " mV");
            Serial.println("电池电量: " + String(device_state.telemetry.system.battery_percentage) + "%");
            Serial.println("充电状态: " + String(device_state.telemetry.system.is_charging ? "充电中" : "未充电"));
            Serial.println("外部电源: " + String(device_state.telemetry.system.external_power ? "已连接" : "未连接"));
            Serial.println("");
#ifdef ENABLE_SDCARD
            Serial.println("--- SD卡状态 ---");
            if (device_state.sdCardReady)
            {
                Serial.println("SD卡状态: 就绪");
                Serial.println("SD卡容量: " + String((unsigned long)device_state.sdCardSizeMB) + " MB");
                Serial.println("SD卡剩余: " + String((unsigned long)device_state.sdCardFreeMB) + " MB");
            }
            else
            {
                Serial.println("SD卡状态: 未就绪");
                Serial.println("⚠️ 请检查SD卡是否正确插入");
            }
#endif
        }
        else if (command == "status")
        {
            Serial.println("=== 系统状态 ===");
            Serial.println("系统正常运行");
            Serial.println("空闲内存: " + String(ESP.getFreeHeap()) + " 字节");
            Serial.println("最小空闲内存: " + String(ESP.getMinFreeHeap()) + " 字节");
            Serial.println("芯片温度: " + String(temperatureRead(), 1) + "°C");
            Serial.println("CPU频率: " + String(ESP.getCpuFreqMHz()) + " MHz");
        }
        else if (command.startsWith("mqtt."))
        {
#ifndef DISABLE_MQTT
            if (command == "mqtt.status")
            {
                Serial.println("=== MQTT状态 ===");
                Serial.println("MQTT服务器: " + String(MQTT_BROKER));
                Serial.println("MQTT端口: " + String(MQTT_PORT));
                Serial.println("保持连接: " + String(MQTT_KEEPALIVE) + "秒");

#ifdef USE_AIR780EG_GSM
                Serial.println("连接方式: Air780EG GSM");
                Serial.println("GSM状态: " + String(device_state.telemetry.modules.gsm_ready ? "就绪" : "未就绪"));
                if (device_state.telemetry.modules.gsm_ready)
                {
                    Serial.println("网络状态: " + String(air780eg.getNetwork().isNetworkRegistered() ? "已连接" : "未连接"));
                    Serial.println("信号强度: " + String(air780eg.getNetwork().getSignalStrength()) + " dBm");
                    Serial.println("运营商: " + air780eg.getNetwork().getOperatorName());
                }
#elif defined(ENABLE_WIFI)
                Serial.println("连接方式: WiFi");
                Serial.println("WiFi状态: " + String(device_state.wifiConnected ? "已连接" : "未连接"));
#endif
                // MQTT连接状态
                Serial.println("MQTT连接: " + air780eg.getMQTT().getState());
            }
            else
            {
                Serial.println("未知MQTT命令，输入 'mqtt.help' 查看帮助");
            }
#else
            Serial.println("MQTT功能已禁用");
#endif
        }
        else if (command == "restart" || command == "reboot")
        {
            Serial.println("正在重启设备...");
            Serial.flush();
            delay(1000);
            ESP.restart();
        }
        else if (command.startsWith("power."))
        {
            #ifdef ENABLE_POWER_MODE_MANAGEMENT
            extern PowerModeManager powerModeManager;
            
            if (command == "power.status")
            {
                powerModeManager.printCurrentStatus();
            }
            else if (command == "power.sleep")
            {
                Serial.println("切换到休眠模式...");
                powerModeManager.setMode(POWER_MODE_SLEEP);
            }
            else if (command == "power.basic")
            {
                Serial.println("切换到基本模式...");
                powerModeManager.setMode(POWER_MODE_BASIC);
            }
            else if (command == "power.normal")
            {
                Serial.println("切换到正常模式...");
                powerModeManager.setMode(POWER_MODE_NORMAL);
            }
            else if (command == "power.sport")
            {
                Serial.println("切换到运动模式...");
                powerModeManager.setMode(POWER_MODE_SPORT);
            }
            else if (command == "power.auto.on")
            {
                Serial.println("启用自动模式切换...");
                powerModeManager.enableAutoModeSwitch(true);
            }
            else if (command == "power.auto.off")
            {
                Serial.println("禁用自动模式切换...");
                powerModeManager.enableAutoModeSwitch(false);
            }
            else if (command == "power.eval")
            {
                Serial.println("手动触发模式评估...");
                powerModeManager.evaluateAndSwitchMode();
            }
            else if (command == "power.config")
            {
                powerModeManager.printModeConfigs();
            }
            else if (command == "power.help")
            {
                Serial.println("=== 功耗模式命令帮助 ===");
                Serial.println("状态查询:");
                Serial.println("  power.status  - 显示当前功耗模式状态");
                Serial.println("  power.config  - 显示所有模式配置");
                Serial.println("");
                Serial.println("模式切换:");
                Serial.println("  power.sleep   - 切换到休眠模式");
                Serial.println("  power.basic   - 切换到基本模式");
                Serial.println("  power.normal  - 切换到正常模式");
                Serial.println("  power.sport   - 切换到运动模式");
                Serial.println("");
                Serial.println("自动模式:");
                Serial.println("  power.auto.on  - 启用自动模式切换");
                Serial.println("  power.auto.off - 禁用自动模式切换");
                Serial.println("  power.eval     - 手动触发模式评估");
                Serial.println("");
                Serial.println("模式说明:");
                Serial.println("  休眠模式: 深度睡眠，最低功耗");
                Serial.println("  基本模式: 5s GPS，低频IMU，省电运行");
                Serial.println("  正常模式: 1s GPS，标准IMU，平衡性能");
                Serial.println("  运动模式: 1s GPS，高精度IMU，最高性能");
            }
            else
            {
                Serial.println("❌ 未知功耗命令: " + command);
                Serial.println("输入 'power.help' 查看功耗命令帮助");
            }
            #else
            Serial.println("❌ 功耗模式管理未启用");
            #endif
        }
        else if (command.startsWith("data."))
        {
            if (command == "data.status")
            {
                Serial.println("=== 数据采集状态 ===");
                dataCollector.printStats();
            }
            else if (command == "data.debug.on")
            {
                dataCollector.setDebug(true);
                Serial.println("✅ 数据采集调试输出已启用");
            }
            else if (command == "data.debug.off")
            {
                dataCollector.setDebug(false);
                Serial.println("❌ 数据采集调试输出已禁用");
            }
            else if (command == "data.verbose.on")
            {
                dataCollector.setVerbose(true);
                Serial.println("✅ 数据采集详细输出已启用");
            }
            else if (command == "data.verbose.off")
            {
                dataCollector.setVerbose(false);
                Serial.println("❌ 数据采集详细输出已禁用");
            }
            else if (command == "data.start")
            {
                dataCollector.startCollection();
                Serial.println("✅ 数据采集已启动");
            }
            else if (command == "data.stop")
            {
                dataCollector.stopCollection();
                Serial.println("❌ 数据采集已停止");
            }
            else if (command == "data.mode.normal")
            {
                dataCollector.setMode(MODE_NORMAL);
                Serial.println("✅ 数据采集模式已切换为正常模式");
            }
            else if (command == "data.mode.sport")
            {
                dataCollector.setMode(MODE_SPORT);
                Serial.println("✅ 数据采集模式已切换为运动模式");
            }
            else if (command == "data.transmit.on")
            {
                dataCollector.enableTransmission(true);
                Serial.println("✅ 数据传输已启用");
            }
            else if (command == "data.transmit.off")
            {
                dataCollector.enableTransmission(false);
                Serial.println("❌ 数据传输已禁用");
            }
            else if (command == "data.help")
            {
                Serial.println("=== 数据采集命令帮助 ===");
                Serial.println("状态查询:");
                Serial.println("  data.status  - 显示数据采集状态");
                Serial.println("");
                Serial.println("调试控制:");
                Serial.println("  data.debug.on   - 启用调试输出");
                Serial.println("  data.debug.off  - 禁用调试输出");
                Serial.println("  data.verbose.on - 启用详细输出");
                Serial.println("  data.verbose.off- 禁用详细输出");
                Serial.println("");
                Serial.println("采集控制:");
                Serial.println("  data.start      - 开始数据采集");
                Serial.println("  data.stop       - 停止数据采集");
                Serial.println("  data.mode.normal- 切换到正常模式(5秒)");
                Serial.println("  data.mode.sport - 切换到运动模式(1秒)");
                Serial.println("");
                Serial.println("传输控制:");
                Serial.println("  data.transmit.on - 启用数据传输");
                Serial.println("  data.transmit.off- 禁用数据传输");
                Serial.println("");
                Serial.println("模式说明:");
                Serial.println("  正常模式: 5秒传输间隔，适合日常使用");
                Serial.println("  运动模式: 1秒传输间隔，适合运动追踪");
            }
            else
            {
                Serial.println("❌ 未知数据采集命令: " + command);
                Serial.println("输入 'data.help' 查看数据采集命令帮助");
            }
        }
        else if (command.startsWith("fusion."))
        {
            #ifdef ENABLE_IMU_FUSION
            extern FusionLocationManager fusionLocationManager;
            
            if (command == "fusion.status")
            {
                Serial.println("=== 融合定位状态 ===");
                if (fusionLocationManager.isInitialized()) {
                    Position pos = fusionLocationManager.getFusedPosition();
                    if (pos.valid) {
                        Serial.printf("融合位置: %.6f, %.6f\n", pos.lat, pos.lng);
                        Serial.printf("融合速度: %.2f km/h\n", pos.speed * 3.6f);
                        Serial.printf("融合航向: %.1f°\n", pos.heading);
                        Serial.printf("位置精度: %.2f m\n", pos.accuracy);
                    } else {
                        Serial.println("融合位置: 无效");
                    }
                    fusionLocationManager.printStats();
                } else {
                    Serial.println("融合定位系统: 未初始化");
                }
            }
            else if (command == "fusion.debug.on")
            {
                fusionLocationManager.setDebug(true);
                fusionLocationManager.resetOrigin();
                fusionLocationManager.resetStats();
                Serial.println("✅ 融合定位调试输出已启用，系统已重置");
            }
            else if (command == "fusion.debug.off")
            {
                fusionLocationManager.setDebug(false);
                Serial.println("❌ 融合定位调试输出已禁用");
            }
            else if (command == "fusion.reset")
            {
                fusionLocationManager.resetOrigin();
                fusionLocationManager.resetStats();
                Serial.println("✅ 融合定位系统已重置");
            }
            else if (command == "fusion.calibrate")
            {
                if (fusionLocationManager.isInitialized()) {
                    fusionLocationManager.calibrateIMU();
                    Serial.println("✅ IMU校准完成，当前设备位置已设为水平基准");
                } else {
                    Serial.println("❌ 融合定位系统未初始化");
                }
            }
            else if (command == "fusion.gravity.on")
            {
                if (fusionLocationManager.isInitialized()) {
                    fusionLocationManager.enableGravityCompensation();
                    Serial.println("✅ 重力补偿已启用，将分离重力和运动加速度");
                } else {
                    Serial.println("❌ 融合定位系统未初始化");
                }
            }
            else if (command == "fusion.gravity.off")
            {
                if (fusionLocationManager.isInitialized()) {
                    fusionLocationManager.disableGravityCompensation();
                    Serial.println("❌ 重力补偿已禁用");
                } else {
                    Serial.println("❌ 融合定位系统未初始化");
                }
            }
            else if (command == "fusion.reset.displacement")
            {
                if (fusionLocationManager.isInitialized()) {
                    fusionLocationManager.resetDisplacement();
                    Serial.println("✅ 相对位移已重置");
                } else {
                    Serial.println("❌ 融合定位系统未初始化");
                }
            }
            else if (command == "fusion.help")
            {
                Serial.println("=== 融合定位命令帮助 ===");
                Serial.println("状态查询:");
                Serial.println("  fusion.status  - 显示融合定位状态");
                Serial.println("");
                Serial.println("调试控制:");
                Serial.println("  fusion.debug.on - 启用调试输出");
                Serial.println("  fusion.debug.off- 禁用调试输出");
                Serial.println("");
                Serial.println("系统控制:");
                Serial.println("  fusion.reset    - 重置融合定位系统");
                Serial.println("  fusion.calibrate- 校准IMU（以当前位置为水平基准）");
                Serial.println("  fusion.gravity.on - 启用重力补偿（分离重力和运动加速度）");
                Serial.println("  fusion.gravity.off- 禁用重力补偿");
                Serial.println("  fusion.reset.displacement - 重置相对位移（当前位置为新的起始点）");
                Serial.println("");
                Serial.println("说明:");
                Serial.println("  融合定位结合IMU、GPS、罗盘数据");
                Serial.println("  提供更精确的位置和速度估计");
            }
            else
            {
                Serial.println("❌ 未知融合定位命令: " + command);
                Serial.println("输入 'fusion.help' 查看融合定位命令帮助");
            }
            #else
            Serial.println("❌ 融合定位功能未启用");
            #endif
        }
        else if (command.startsWith("ota."))
        {
            if (command == "ota.check")
            {
                Serial.println("🔍 手动检查OTA更新...");
                otaManager.checkForUpdates();
            }
            else if (command == "ota.auto.on")
            {
                otaManager.setAutoUpgrade(true);
                Serial.println("✅ 自动升级已启用");
            }
            else if (command == "ota.auto.off")
            {
                otaManager.setAutoUpgrade(false);
                Serial.println("❌ 自动升级已禁用");
            }
            else if (command == "ota.status")
            {
                Serial.println("=== OTA状态 ===");
                Serial.println("自动升级: " + String(otaManager.getAutoUpgrade() ? "启用" : "禁用"));
                
                String statusText;
                switch(otaManager.getStatus()) {
                    case 0: statusText = "空闲"; break;
                    case 1: statusText = "检查中"; break;
                    case 2: statusText = "下载中"; break;
                    case 3: statusText = "安装中"; break;
                    case 4: statusText = "成功"; break;
                    case 5: statusText = "失败"; break;
                    default: statusText = "未知"; break;
                }
                
                Serial.println("当前状态: " + statusText);
                Serial.println("升级进度: " + String(otaManager.getProgress()) + "%");
            }
            else if (command == "ota.help")
            {
                Serial.println("=== OTA升级命令帮助 ===");
                Serial.println("  ota.check    - 手动检查更新");
                Serial.println("  ota.auto.on  - 启用自动升级");
                Serial.println("  ota.auto.off - 禁用自动升级");
                Serial.println("  ota.status   - 显示OTA状态");
                Serial.println("  ota.help     - 显示此帮助");
            }
            else
            {
                Serial.println("❌ 未知OTA命令: " + command);
                Serial.println("输入 'ota.help' 查看OTA命令帮助");
            }
        }
        else if (command == "help")
        {
            Serial.println("=== 可用命令 ===");
            Serial.println("基本命令:");
            Serial.println("  info     - 显示详细设备信息");
            Serial.println("  status   - 显示系统状态");
            Serial.println("  restart  - 重启设备");
            Serial.println("  help     - 显示此帮助信息");
            Serial.println("");
#ifdef ENABLE_SDCARD
            Serial.println("SD卡命令:");
            Serial.println("  sd.info      - 显示SD卡详细信息");
            Serial.println("  sd.status    - 检查SD卡状态");
            Serial.println("  sd.tree      - 显示目录树结构");
            Serial.println("  sd.structure - 显示目录结构定义");
            Serial.println("  sd.fmt       - 格式化说明");
            Serial.println("  sd.init      - 重新初始化SD卡");
            Serial.println("  sd.help      - 显示SD卡命令帮助");
            Serial.println("");
#endif
#ifdef ENABLE_GPS_LOGGER
            Serial.println("GPS记录器命令:");
            Serial.println("  gs         - 开始GPS记录会话");
            Serial.println("  gt         - 停止GPS记录会话");
            Serial.println("  gst        - 显示GPS记录状态");
            Serial.println("  ge         - 导出当前会话为GeoJSON");
            Serial.println("  gl         - 列出GPS日志文件");
            Serial.println("  gi         - 显示GPS存储信息");
            Serial.println("  gh         - 显示GPS命令帮助");
            Serial.println("");
#endif
#ifdef ENABLE_POWER_MODE_MANAGEMENT
            Serial.println("功耗模式命令:");
            Serial.println("  power.status - 显示功耗模式状态");
            Serial.println("  power.basic  - 切换到基本模式");
            Serial.println("  power.normal - 切换到正常模式");
            Serial.println("  power.sport  - 切换到运动模式");
            Serial.println("  power.help   - 显示功耗命令帮助");
            Serial.println("");
#endif
            Serial.println("数据采集命令:");
            Serial.println("  data.status  - 显示数据采集状态");
            Serial.println("  data.debug.on- 启用调试输出");
            Serial.println("  data.help    - 显示数据采集命令帮助");
            Serial.println("");
#ifdef ENABLE_IMU_FUSION
            Serial.println("融合定位命令:");
            Serial.println("  fusion.status- 显示融合定位状态");
            Serial.println("  fusion.debug.on- 启用调试输出 fusion.debug.off");
            Serial.println("  fusion.help  - 显示融合定位命令帮助");
            Serial.println("");
#endif
            Serial.println("  ota.check    - 手动检查OTA更新");
            Serial.println("  ota.auto.on  - 启用自动升级");
            Serial.println("  ota.auto.off - 禁用自动升级");
            Serial.println("  ota.status   - 显示OTA状态");
            Serial.println("  ota.help     - 显示OTA命令帮助");
            Serial.println("");
            Serial.println("提示: 命令不区分大小写");
        }
        else
        {
            Serial.println("❌ 未知命令: " + command);
            Serial.println("输入 'help' 查看可用命令");
        }

        Serial.println(""); // 添加空行分隔
    }
}
