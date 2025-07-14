#!/usr/bin/env python3
"""
ESP32-S3 MotoBox OTA升级测试脚本
用于测试SD卡升级和MQTT在线升级功能
"""

import os
import json
import hashlib
import argparse
from pathlib import Path

def create_sd_upgrade_files(firmware_path, version, output_dir):
    """创建SD卡升级所需的文件"""
    
    if not os.path.exists(firmware_path):
        print(f"❌ 固件文件不存在: {firmware_path}")
        return False
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    # 复制固件文件
    firmware_dest = os.path.join(output_dir, "firmware.bin")
    with open(firmware_path, 'rb') as src, open(firmware_dest, 'wb') as dst:
        dst.write(src.read())
    
    # 创建版本文件
    version_file = os.path.join(output_dir, "version.txt")
    with open(version_file, 'w') as f:
        f.write(version)
    
    # 计算并创建校验和文件
    checksum = calculate_md5(firmware_path)
    checksum_file = os.path.join(output_dir, "checksum.txt")
    with open(checksum_file, 'w') as f:
        f.write(checksum)
    
    print(f"✅ SD卡升级文件已创建:")
    print(f"   📁 输出目录: {output_dir}")
    print(f"   📦 固件文件: firmware.bin ({get_file_size(firmware_dest)})")
    print(f"   📄 版本文件: version.txt ({version})")
    print(f"   🔐 校验文件: checksum.txt ({checksum})")
    
    return True

def calculate_md5(file_path):
    """计算文件MD5校验和"""
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def get_file_size(file_path):
    """获取文件大小的可读格式"""
    size = os.path.getsize(file_path)
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024.0:
            return f"{size:.1f} {unit}"
        size /= 1024.0
    return f"{size:.1f} TB"

def create_mqtt_test_messages():
    """创建MQTT测试消息"""
    
    device_id = "ESP32_ABCD1234"  # 示例设备ID
    
    # 版本检查响应消息
    version_check_response = {
        "latest_version": "v4.1.0",
        "download_url": "https://example.com/firmware/v4.1.0.bin",
        "force_update": False,
        "release_notes": "修复蓝牙连接问题，优化电源管理"
    }
    
    # 手动触发OTA检查命令
    manual_check_command = {
        "cmd": "check_ota"
    }
    
    print("📡 MQTT测试消息:")
    print("\n1. 版本检查响应 (发布到: device/{}/ota/check)".format(device_id))
    print(json.dumps(version_check_response, indent=2, ensure_ascii=False))
    
    print("\n2. 手动触发OTA检查 (发布到: vehicle/v1/{}/ctrl/ota)".format(device_id))
    print(json.dumps(manual_check_command, indent=2, ensure_ascii=False))
    
    # 保存到文件
    with open("mqtt_test_messages.json", "w", encoding='utf-8') as f:
        json.dump({
            "version_check_response": version_check_response,
            "manual_check_command": manual_check_command,
            "topics": {
                "version_check": f"device/{device_id}/ota/check",
                "manual_trigger": f"vehicle/v1/{device_id}/ctrl/ota",
                "status_report": f"device/{device_id}/ota/status"
            }
        }, f, indent=2, ensure_ascii=False)
    
    print("\n💾 测试消息已保存到: mqtt_test_messages.json")

def validate_upgrade_conditions():
    """验证升级条件检查清单"""
    
    print("🔍 OTA升级条件检查清单:")
    print("\n必须满足以下所有条件才能进行升级:")
    print("  ✅ 电池电量 ≥ 90%")
    print("  ✅ 新版本号 > 当前版本号")
    print("  ✅ 固件文件存在且完整")
    print("  ✅ 有足够的Flash存储空间")
    print("  ✅ 设备未在升级过程中")
    
    print("\n📋 升级前检查步骤:")
    print("  1. 检查设备电池电量显示")
    print("  2. 确认SD卡已正确插入")
    print("  3. 验证固件文件完整性")
    print("  4. 确保设备处于稳定状态")
    
    print("\n⚠️  升级注意事项:")
    print("  • 升级过程中请勿断电")
    print("  • 升级过程中请勿移除SD卡")
    print("  • 升级失败时设备会自动回滚")
    print("  • 升级成功后设备会自动重启")

def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 MotoBox OTA升级测试工具")
    parser.add_argument("--create-sd", action="store_true", help="创建SD卡升级文件")
    parser.add_argument("--firmware", type=str, help="固件文件路径")
    parser.add_argument("--version", type=str, help="固件版本号")
    parser.add_argument("--output", type=str, default="./sd_upgrade", help="输出目录")
    parser.add_argument("--mqtt-test", action="store_true", help="生成MQTT测试消息")
    parser.add_argument("--check-conditions", action="store_true", help="显示升级条件检查清单")
    
    args = parser.parse_args()
    
    if args.create_sd:
        if not args.firmware or not args.version:
            print("❌ 创建SD卡升级文件需要指定 --firmware 和 --version 参数")
            return
        
        create_sd_upgrade_files(args.firmware, args.version, args.output)
    
    elif args.mqtt_test:
        create_mqtt_test_messages()
    
    elif args.check_conditions:
        validate_upgrade_conditions()
    
    else:
        print("ESP32-S3 MotoBox OTA升级测试工具")
        print("\n使用示例:")
        print("  python ota_test.py --create-sd --firmware firmware.bin --version v4.1.0")
        print("  python ota_test.py --mqtt-test")
        print("  python ota_test.py --check-conditions")
        print("\n使用 --help 查看详细帮助")

if __name__ == "__main__":
    main()
