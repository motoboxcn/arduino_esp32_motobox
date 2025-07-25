#!/usr/bin/env python3
"""
LED充电状态功能测试脚本
测试LED根据充电状态自动切换显示效果的功能
"""

import serial
import time
import sys
import re

class LEDChargingTest:
    def __init__(self, port='/dev/cu.usbserial-0001', baudrate=115200):
        """初始化测试"""
        self.port = port
        self.baudrate = baudrate
        self.ser = None
        
    def connect(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=1)
            print(f"✅ 已连接到 {self.port}")
            time.sleep(2)  # 等待连接稳定
            return True
        except Exception as e:
            print(f"❌ 连接失败: {e}")
            return False
    
    def disconnect(self):
        """断开连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("🔌 已断开连接")
    
    def read_serial_data(self, timeout=5):
        """读取串口数据"""
        start_time = time.time()
        data_lines = []
        
        while time.time() - start_time < timeout:
            if self.ser.in_waiting > 0:
                try:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        data_lines.append(line)
                        print(f"📥 {line}")
                except Exception as e:
                    print(f"⚠️ 读取数据错误: {e}")
            time.sleep(0.1)
        
        return data_lines
    
    def send_command(self, command):
        """发送命令"""
        if self.ser and self.ser.is_open:
            self.ser.write(f"{command}\n".encode())
            print(f"📤 发送命令: {command}")
            time.sleep(0.5)
    
    def test_led_charging_status(self):
        """测试LED充电状态功能"""
        print("\n🔋 开始测试LED充电状态功能...")
        
        # 读取初始状态
        print("\n1️⃣ 读取初始LED状态...")
        initial_data = self.read_serial_data(timeout=10)
        
        # 查找LED和充电相关信息
        led_info = []
        charging_info = []
        
        for line in initial_data:
            if 'LED' in line or 'led' in line:
                led_info.append(line)
            if '充电' in line or 'charging' in line or 'Charging' in line:
                charging_info.append(line)
        
        print(f"\n📊 LED相关信息:")
        for info in led_info:
            print(f"   {info}")
        
        print(f"\n🔌 充电相关信息:")
        for info in charging_info:
            print(f"   {info}")
        
        # 测试手动LED控制（应该暂时禁用自动充电显示）
        print("\n2️⃣ 测试手动LED控制...")
        test_commands = [
            "led red",      # 设置红色LED
            "led green",    # 设置绿色LED  
            "led blue",     # 设置蓝色LED
            "led off",      # 关闭LED
        ]
        
        for cmd in test_commands:
            self.send_command(cmd)
            data = self.read_serial_data(timeout=3)
            
            # 检查是否有LED状态变化的反馈
            led_response = [line for line in data if 'LED' in line or 'led' in line]
            if led_response:
                print(f"   ✅ LED响应: {led_response}")
            else:
                print(f"   ⚠️ 未检测到LED响应")
        
        # 等待一段时间观察自动充电状态恢复
        print("\n3️⃣ 等待自动充电状态恢复...")
        time.sleep(10)
        recovery_data = self.read_serial_data(timeout=5)
        
        # 分析恢复后的状态
        led_recovery = [line for line in recovery_data if 'LED' in line or 'led' in line]
        charging_recovery = [line for line in recovery_data if '充电' in line or 'charging' in line]
        
        print(f"\n📈 恢复状态分析:")
        print(f"   LED状态: {led_recovery}")
        print(f"   充电状态: {charging_recovery}")
        
        return True
    
    def test_battery_status(self):
        """测试电池状态"""
        print("\n🔋 测试电池状态...")
        
        # 发送电池状态查询命令
        self.send_command("battery")
        data = self.read_serial_data(timeout=5)
        
        # 查找电池相关信息
        battery_info = []
        for line in data:
            if any(keyword in line.lower() for keyword in ['battery', '电池', 'voltage', '电压', '%']):
                battery_info.append(line)
        
        print(f"📊 电池状态信息:")
        for info in battery_info:
            print(f"   {info}")
        
        return battery_info
    
    def run_comprehensive_test(self):
        """运行综合测试"""
        print("🚀 开始LED充电状态综合测试")
        print("=" * 50)
        
        if not self.connect():
            return False
        
        try:
            # 测试电池状态
            self.test_battery_status()
            
            # 测试LED充电状态功能
            self.test_led_charging_status()
            
            # 长期监控测试
            print("\n4️⃣ 长期监控测试（30秒）...")
            print("请在此期间插拔充电器观察LED变化...")
            
            start_time = time.time()
            charging_changes = []
            led_changes = []
            
            while time.time() - start_time < 30:
                data = self.read_serial_data(timeout=2)
                
                for line in data:
                    if '充电状态变化' in line or 'charging' in line.lower():
                        charging_changes.append((time.time() - start_time, line))
                    if 'LEDManager' in line or 'LED模式' in line:
                        led_changes.append((time.time() - start_time, line))
            
            print(f"\n📈 监控结果:")
            print(f"   充电状态变化: {len(charging_changes)} 次")
            for timestamp, change in charging_changes:
                print(f"     {timestamp:.1f}s: {change}")
            
            print(f"   LED状态变化: {len(led_changes)} 次")
            for timestamp, change in led_changes:
                print(f"     {timestamp:.1f}s: {change}")
            
            print("\n✅ 测试完成!")
            
        except KeyboardInterrupt:
            print("\n⏹️ 测试被用户中断")
        except Exception as e:
            print(f"\n❌ 测试过程中出现错误: {e}")
        finally:
            self.disconnect()
        
        return True

def main():
    """主函数"""
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = '/dev/cu.usbserial-0001'  # 默认端口
    
    print(f"🔧 使用串口: {port}")
    print("💡 提示: 如果端口不正确，请使用: python test_led_charging.py <端口名>")
    
    tester = LEDChargingTest(port)
    tester.run_comprehensive_test()

if __name__ == "__main__":
    main()
