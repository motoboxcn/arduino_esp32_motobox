#!/usr/bin/env python3
"""
简单的IMU数据变化测试
"""

import re
import sys

def test_imu_changes(log_file):
    """测试IMU数据变化频率"""
    
    imu_pattern = r'imu_data: roll=([-\d.]+), pitch=([-\d.]+), yaw=([-\d.]+)'
    
    records = []
    
    try:
        with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                match = re.search(imu_pattern, line)
                if match:
                    roll = float(match.group(1))
                    pitch = float(match.group(2))
                    yaw = float(match.group(3))
                    records.append((roll, pitch, yaw))
    except Exception as e:
        print(f"错误: {e}")
        return
    
    if len(records) < 10:
        print("数据不足")
        return
    
    print(f"总记录数: {len(records)}")
    print("最后10条记录:")
    
    changes = 0
    for i, (roll, pitch, yaw) in enumerate(records[-10:]):
        print(f"{i+1:2d}. roll={roll:6.2f}, pitch={pitch:6.2f}, yaw={yaw:6.2f}")
        
        if i > 0:
            prev_roll, prev_pitch, prev_yaw = records[-(10-i+1)]
            if (abs(roll - prev_roll) > 0.01 or 
                abs(pitch - prev_pitch) > 0.01 or 
                abs(yaw - prev_yaw) > 0.01):
                changes += 1
    
    print(f"\n最后10条中有变化的: {changes}/9")
    
    # 检查整体变化率
    total_changes = 0
    for i in range(1, len(records)):
        prev_roll, prev_pitch, prev_yaw = records[i-1]
        roll, pitch, yaw = records[i]
        if (abs(roll - prev_roll) > 0.01 or 
            abs(pitch - prev_pitch) > 0.01 or 
            abs(yaw - prev_yaw) > 0.01):
            total_changes += 1
    
    change_rate = total_changes / (len(records) - 1) * 100
    print(f"总体变化率: {change_rate:.1f}%")
    
    if change_rate > 50:
        print("✅ 数据更新频率正常")
    else:
        print("❌ 数据更新频率仍然较低")

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("用法: python3 simple_imu_test.py <log_file>")
        sys.exit(1)
    
    test_imu_changes(sys.argv[1])
