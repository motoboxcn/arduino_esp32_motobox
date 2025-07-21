#!/usr/bin/env python3
"""
IMU数据更新频率测试脚本
用于分析串口输出中IMU数据的更新频率和变化情况
"""

import re
import sys
from datetime import datetime
from collections import defaultdict

def analyze_imu_data(log_file):
    """分析IMU数据更新频率"""
    
    # 正则表达式匹配IMU数据
    imu_pattern = r'imu_data: roll=([-\d.]+), pitch=([-\d.]+), yaw=([-\d.]+), temp=([-\d.]+)°C'
    
    imu_records = []
    line_count = 0
    
    print("🔍 分析IMU数据更新频率...")
    print("=" * 60)
    
    try:
        with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line_num, line in enumerate(f, 1):
                line_count += 1
                
                # 匹配IMU数据
                match = re.search(imu_pattern, line)
                if match:
                    roll = float(match.group(1))
                    pitch = float(match.group(2))
                    yaw = float(match.group(3))
                    temp = float(match.group(4))
                    
                    imu_records.append({
                        'line': line_num,
                        'roll': roll,
                        'pitch': pitch,
                        'yaw': yaw,
                        'temp': temp,
                        'raw_line': line.strip()
                    })
    
    except FileNotFoundError:
        print(f"❌ 文件未找到: {log_file}")
        return
    except Exception as e:
        print(f"❌ 读取文件出错: {e}")
        return
    
    if not imu_records:
        print("❌ 未找到IMU数据记录")
        return
    
    print(f"📊 总行数: {line_count}")
    print(f"📊 IMU记录数: {len(imu_records)}")
    print(f"📊 IMU数据占比: {len(imu_records)/line_count*100:.1f}%")
    print()
    
    # 分析数据变化频率
    analyze_data_changes(imu_records)
    
    # 分析数据分布
    analyze_data_distribution(imu_records)
    
    # 显示最近的数据样本
    show_recent_samples(imu_records)

def analyze_data_changes(records):
    """分析数据变化频率"""
    print("📈 数据变化分析:")
    print("-" * 40)
    
    if len(records) < 2:
        print("❌ 数据不足，无法分析变化")
        return
    
    # 统计连续相同值的情况
    roll_changes = 0
    pitch_changes = 0
    yaw_changes = 0
    temp_changes = 0
    
    consecutive_same = defaultdict(int)
    current_same_count = 1
    
    for i in range(1, len(records)):
        prev = records[i-1]
        curr = records[i]
        
        # 检查各个值是否发生变化
        if abs(curr['roll'] - prev['roll']) > 0.01:
            roll_changes += 1
        if abs(curr['pitch'] - prev['pitch']) > 0.01:
            pitch_changes += 1
        if abs(curr['yaw'] - prev['yaw']) > 0.01:
            yaw_changes += 1
        if abs(curr['temp'] - prev['temp']) > 0.1:
            temp_changes += 1
        
        # 检查是否完全相同
        if (abs(curr['roll'] - prev['roll']) < 0.01 and 
            abs(curr['pitch'] - prev['pitch']) < 0.01 and
            abs(curr['yaw'] - prev['yaw']) < 0.01):
            current_same_count += 1
        else:
            consecutive_same[current_same_count] += 1
            current_same_count = 1
    
    # 添加最后一组
    consecutive_same[current_same_count] += 1
    
    total_records = len(records)
    print(f"Roll变化次数: {roll_changes}/{total_records-1} ({roll_changes/(total_records-1)*100:.1f}%)")
    print(f"Pitch变化次数: {pitch_changes}/{total_records-1} ({pitch_changes/(total_records-1)*100:.1f}%)")
    print(f"Yaw变化次数: {yaw_changes}/{total_records-1} ({yaw_changes/(total_records-1)*100:.1f}%)")
    print(f"温度变化次数: {temp_changes}/{total_records-1} ({temp_changes/(total_records-1)*100:.1f}%)")
    print()
    
    # 显示连续相同值的统计
    print("连续相同值统计:")
    for count in sorted(consecutive_same.keys()):
        if count > 1:
            print(f"  连续{count}次相同: {consecutive_same[count]}组")
    print()

def analyze_data_distribution(records):
    """分析数据分布"""
    print("📊 数据分布分析:")
    print("-" * 40)
    
    # 提取各个数据序列
    rolls = [r['roll'] for r in records]
    pitches = [r['pitch'] for r in records]
    yaws = [r['yaw'] for r in records]
    temps = [r['temp'] for r in records]
    
    def print_stats(name, values):
        if values:
            print(f"{name}:")
            print(f"  范围: {min(values):.2f} ~ {max(values):.2f}")
            print(f"  平均: {sum(values)/len(values):.2f}")
            print(f"  唯一值数量: {len(set(values))}")
    
    print_stats("Roll", rolls)
    print_stats("Pitch", pitches)
    print_stats("Yaw", yaws)
    print_stats("温度", temps)
    print()

def show_recent_samples(records):
    """显示最近的数据样本"""
    print("📝 最近数据样本 (最后10条):")
    print("-" * 40)
    
    recent_records = records[-10:] if len(records) >= 10 else records
    
    for i, record in enumerate(recent_records):
        print(f"{i+1:2d}. Line {record['line']:4d}: "
              f"roll={record['roll']:6.2f}, pitch={record['pitch']:6.2f}, "
              f"yaw={record['yaw']:6.2f}, temp={record['temp']:5.1f}°C")
    print()

def main():
    if len(sys.argv) != 2:
        print("用法: python3 test_imu_frequency.py <log_file>")
        print("示例: python3 test_imu_frequency.py ../tmp/out")
        sys.exit(1)
    
    log_file = sys.argv[1]
    print(f"🚀 开始分析文件: {log_file}")
    print(f"⏰ 分析时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
    print()
    
    analyze_imu_data(log_file)
    
    print("✅ 分析完成!")
    print()
    print("💡 期望结果:")
    print("  - 启用陀螺仪后，数据变化频率应该显著提高")
    print("  - 连续相同值的情况应该大幅减少")
    print("  - Roll/Pitch/Yaw应该有更频繁的变化")

if __name__ == "__main__":
    main()
