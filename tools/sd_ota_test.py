#!/usr/bin/env python3
"""
ESP32-S3 MotoBox SD卡OTA升级测试工具
专门用于创建和测试SD卡升级文件
"""

import os
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
    
    print(f"✅ SD卡升级文件已创建:")
    print(f"   📁 输出目录: {output_dir}")
    print(f"   📦 固件文件: firmware.bin ({get_file_size(firmware_dest)})")
    print(f"   📄 版本文件: version.txt ({version})")
    
    print(f"\n📋 使用说明:")
    print(f"   1. 将 {output_dir} 目录中的所有文件复制到SD卡根目录")
    print(f"   2. 确保设备电池电量≥90%")
    print(f"   3. 插入SD卡并重启设备")
    print(f"   4. 设备会自动检测并升级固件")
    
    return True

def get_file_size(file_path):
    """获取文件大小的可读格式"""
    size = os.path.getsize(file_path)
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size < 1024.0:
            return f"{size:.1f} {unit}"
        size /= 1024.0
    return f"{size:.1f} TB"

def show_upgrade_guide():
    """显示SD卡升级指南"""
    print("📖 ESP32-S3 MotoBox SD卡升级指南")
    print("\n🔍 升级条件检查:")
    print("   ✅ 电池电量 ≥ 90%")
    print("   ✅ 新版本号 > 当前版本号")
    print("   ✅ SD卡中存在 firmware.bin 文件")
    print("   ✅ SD卡中存在 version.txt 文件")
    
    print("\n📋 升级步骤:")
    print("   1. 使用本工具创建升级文件")
    print("   2. 将文件复制到SD卡根目录")
    print("   3. 确保设备电池电量充足")
    print("   4. 插入SD卡并重启设备")
    print("   5. 听到升级提示音后等待完成")
    
    print("\n🎵 语音提示说明:")
    print("   🔊 3声短促音 - 开始升级")
    print("   🔊 2声中等音 - 升级进行中")
    print("   🔊 上升音调 - 升级成功")
    print("   🔊 长声低音 - 升级失败")
    print("   🔊 单声短促 - 进度提示")
    
    print("\n⚠️  注意事项:")
    print("   • 升级过程中请勿断电")
    print("   • 升级过程中请勿移除SD卡")
    print("   • 升级成功后设备会自动重启")
    print("   • 如果升级失败，设备会继续使用原固件")

def validate_firmware_file(firmware_path):
    """验证固件文件"""
    if not os.path.exists(firmware_path):
        print(f"❌ 固件文件不存在: {firmware_path}")
        return False
    
    size = os.path.getsize(firmware_path)
    if size < 100 * 1024:  # 小于100KB
        print(f"⚠️  固件文件可能太小: {get_file_size(firmware_path)}")
        return False
    
    if size > 10 * 1024 * 1024:  # 大于10MB
        print(f"⚠️  固件文件可能太大: {get_file_size(firmware_path)}")
        return False
    
    print(f"✅ 固件文件验证通过: {get_file_size(firmware_path)}")
    return True

def validate_version_format(version):
    """验证版本号格式"""
    import re
    
    # 支持的版本格式: v4.0.0, 4.0.0, v4.0.0+694
    pattern = r'^v?\d+\.\d+\.\d+(\+\d+)?$'
    
    if re.match(pattern, version):
        print(f"✅ 版本号格式正确: {version}")
        return True
    else:
        print(f"❌ 版本号格式错误: {version}")
        print("   支持的格式: v4.0.0, 4.0.0, v4.0.0+694")
        return False

def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 MotoBox SD卡OTA升级测试工具")
    parser.add_argument("--create", action="store_true", help="创建SD卡升级文件")
    parser.add_argument("--firmware", type=str, help="固件文件路径")
    parser.add_argument("--version", type=str, help="固件版本号")
    parser.add_argument("--output", type=str, default="./sd_upgrade", help="输出目录")
    parser.add_argument("--guide", action="store_true", help="显示升级指南")
    parser.add_argument("--validate", action="store_true", help="验证固件文件")
    
    args = parser.parse_args()
    
    if args.create:
        if not args.firmware or not args.version:
            print("❌ 创建SD卡升级文件需要指定 --firmware 和 --version 参数")
            print("   示例: python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0")
            return
        
        # 验证版本号格式
        if not validate_version_format(args.version):
            return
        
        # 验证固件文件
        if not validate_firmware_file(args.firmware):
            return
        
        create_sd_upgrade_files(args.firmware, args.version, args.output)
    
    elif args.validate:
        if not args.firmware:
            print("❌ 验证固件文件需要指定 --firmware 参数")
            return
        validate_firmware_file(args.firmware)
    
    elif args.guide:
        show_upgrade_guide()
    
    else:
        print("🔧 ESP32-S3 MotoBox SD卡OTA升级测试工具")
        print("\n使用示例:")
        print("  # 创建升级文件")
        print("  python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0")
        print("\n  # 验证固件文件")
        print("  python sd_ota_test.py --validate --firmware firmware.bin")
        print("\n  # 显示升级指南")
        print("  python sd_ota_test.py --guide")
        print("\n使用 --help 查看详细帮助")

if __name__ == "__main__":
    main()
