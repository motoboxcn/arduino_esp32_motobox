#!/usr/bin/env python3
"""
MotoBox BLE客户端示例
用于测试和演示BLE功能的使用

依赖库：
pip install bleak

使用方法：
python ble_client_example.py
"""

import asyncio
import json
from datetime import datetime
from bleak import BleakClient, BleakScanner

# BLE配置
DEVICE_NAME_PREFIX = "MotoBox-"  # 设备名称前缀，完整名称格式：MotoBox-{设备ID}
SERVICE_UUID = "12345678-1234-1234-1234-123456789ABC"
GPS_CHAR_UUID = "12345678-1234-1234-1234-123456789ABD"
BATTERY_CHAR_UUID = "12345678-1234-1234-1234-123456789ABE"
IMU_CHAR_UUID = "12345678-1234-1234-1234-123456789ABF"

class MotoBoxClient:
    def __init__(self):
        self.client = None
        self.connected = False
        
    async def scan_devices(self):
        """扫描BLE设备"""
        print("正在扫描BLE设备...")
        devices = await BleakScanner.discover()
        
        motobox_devices = []
        for device in devices:
            if device.name and device.name.startswith(DEVICE_NAME_PREFIX):
                motobox_devices.append(device)
                print(f"发现MotoBox设备: {device.name} ({device.address})")
        
        return motobox_devices
    
    async def connect(self, device_address):
        """连接到MotoBox设备"""
        try:
            self.client = BleakClient(device_address)
            await self.client.connect()
            self.connected = True
            print(f"✅ 已连接到设备: {device_address}")
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self.client and self.connected:
            await self.client.disconnect()
            self.connected = False
            print("🔌 已断开连接")
    
    def gps_data_callback(self, sender, data):
        """GPS数据回调"""
        try:
            json_data = json.loads(data.decode('utf-8'))
            timestamp = datetime.now().strftime("%H:%M:%S")
            
            print(f"\n📍 GPS数据 [{timestamp}]:")
            print(f"  位置: {json_data.get('lat', 'N/A'):.6f}, {json_data.get('lng', 'N/A'):.6f}")
            print(f"  海拔: {json_data.get('alt', 'N/A'):.1f}m")
            print(f"  速度: {json_data.get('spd', 'N/A'):.1f}km/h")
            print(f"  航向: {json_data.get('crs', 'N/A'):.1f}°")
            print(f"  卫星: {json_data.get('sat', 'N/A')}")
            print(f"  有效: {json_data.get('valid', False)}")
            
        except Exception as e:
            print(f"❌ GPS数据解析错误: {e}")
    
    def battery_data_callback(self, sender, data):
        """电池数据回调"""
        try:
            json_data = json.loads(data.decode('utf-8'))
            timestamp = datetime.now().strftime("%H:%M:%S")
            
            print(f"\n🔋 电池数据 [{timestamp}]:")
            print(f"  电压: {json_data.get('voltage', 'N/A')}mV")
            print(f"  电量: {json_data.get('percentage', 'N/A')}%")
            print(f"  充电: {'是' if json_data.get('charging', False) else '否'}")
            print(f"  外部电源: {'是' if json_data.get('external', False) else '否'}")
            
        except Exception as e:
            print(f"❌ 电池数据解析错误: {e}")
    
    def imu_data_callback(self, sender, data):
        """IMU数据回调"""
        try:
            json_data = json.loads(data.decode('utf-8'))
            timestamp = datetime.now().strftime("%H:%M:%S")
            
            print(f"\n📱 IMU数据 [{timestamp}]:")
            print(f"  姿态: 俯仰={json_data.get('pitch', 'N/A'):.1f}°, "
                  f"横滚={json_data.get('roll', 'N/A'):.1f}°, "
                  f"偏航={json_data.get('yaw', 'N/A'):.1f}°")
            
            accel = json_data.get('accel', {})
            print(f"  加速度: X={accel.get('x', 'N/A'):.2f}, "
                  f"Y={accel.get('y', 'N/A'):.2f}, "
                  f"Z={accel.get('z', 'N/A'):.2f} m/s²")
            
            gyro = json_data.get('gyro', {})
            print(f"  角速度: X={gyro.get('x', 'N/A'):.3f}, "
                  f"Y={gyro.get('y', 'N/A'):.3f}, "
                  f"Z={gyro.get('z', 'N/A'):.3f} rad/s")
            
            print(f"  有效: {json_data.get('valid', False)}")
            
        except Exception as e:
            print(f"❌ IMU数据解析错误: {e}")
    
    async def subscribe_to_data(self):
        """订阅数据更新"""
        if not self.client or not self.connected:
            print("❌ 设备未连接")
            return False
        
        try:
            # 订阅GPS数据
            await self.client.start_notify(GPS_CHAR_UUID, self.gps_data_callback)
            print("✅ 已订阅GPS数据")
            
            # 订阅电池数据
            await self.client.start_notify(BATTERY_CHAR_UUID, self.battery_data_callback)
            print("✅ 已订阅电池数据")
            
            # 订阅IMU数据
            await self.client.start_notify(IMU_CHAR_UUID, self.imu_data_callback)
            print("✅ 已订阅IMU数据")
            
            return True
            
        except Exception as e:
            print(f"❌ 订阅失败: {e}")
            return False
    
    async def read_initial_data(self):
        """读取初始数据"""
        if not self.client or not self.connected:
            return
        
        try:
            print("\n📖 读取初始数据:")
            
            # 读取GPS数据
            gps_data = await self.client.read_gatt_char(GPS_CHAR_UUID)
            self.gps_data_callback(None, gps_data)
            
            # 读取电池数据
            battery_data = await self.client.read_gatt_char(BATTERY_CHAR_UUID)
            self.battery_data_callback(None, battery_data)
            
            # 读取IMU数据
            imu_data = await self.client.read_gatt_char(IMU_CHAR_UUID)
            self.imu_data_callback(None, imu_data)
            
        except Exception as e:
            print(f"❌ 读取初始数据失败: {e}")

async def main():
    """主函数"""
    print("🚀 MotoBox BLE客户端启动")
    print("=" * 50)
    
    client = MotoBoxClient()
    
    try:
        # 扫描设备
        devices = await client.scan_devices()
        if not devices:
            print("❌ 未发现MotoBox设备")
            return
        
        # 连接第一个发现的设备
        device = devices[0]
        if not await client.connect(device.address):
            return
        
        # 读取初始数据
        await client.read_initial_data()
        
        # 订阅数据更新
        if not await client.subscribe_to_data():
            return
        
        print("\n🎉 数据订阅成功！按Ctrl+C退出")
        print("=" * 50)
        
        # 保持连接并接收数据
        while True:
            await asyncio.sleep(1)
            
    except KeyboardInterrupt:
        print("\n\n👋 用户中断，正在退出...")
    except Exception as e:
        print(f"\n❌ 程序异常: {e}")
    finally:
        await client.disconnect()
        print("✅ 程序结束")

if __name__ == "__main__":
    # 检查依赖
    try:
        import bleak
    except ImportError:
        print("❌ 缺少依赖库，请安装: pip install bleak")
        exit(1)
    
    # 运行主程序
    asyncio.run(main())
