#!/usr/bin/env python3
"""
LED充电状态调试脚本
简化版本，专注于调试LED呼吸效果问题
"""

import serial
import time
import sys

def main():
    port = '/dev/cu.usbserial-0001' if len(sys.argv) == 1 else sys.argv[1]
    
    try:
        ser = serial.Serial(port, 115200, timeout=1)
        print(f"✅ 已连接到 {port}")
        print("🔍 开始监控LED调试信息...")
        print("📝 请插拔充电器观察LED变化")
        print("⏹️  按 Ctrl+C 停止监控")
        print("-" * 50)
        
        while True:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # 过滤LED相关的调试信息
                        if any(keyword in line for keyword in ['LEDManager', 'PWMLED', '充电状态', '呼吸效果', 'LED模式']):
                            timestamp = time.strftime("%H:%M:%S")
                            print(f"[{timestamp}] {line}")
                except Exception as e:
                    print(f"⚠️ 读取错误: {e}")
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n⏹️ 监控已停止")
    except Exception as e:
        print(f"❌ 连接失败: {e}")
        print("💡 请检查串口设备名称")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()

if __name__ == "__main__":
    main()
