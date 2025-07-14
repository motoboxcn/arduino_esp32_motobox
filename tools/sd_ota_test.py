#!/usr/bin/env python3
"""
ESP32-S3 MotoBox SD卡OTA升级测试工具
支持创建多个固件文件并自动选择最新版本升级
"""

import os
import hashlib
import argparse
from pathlib import Path

def create_single_firmware(firmware_path, version, output_dir, naming_style="standard"):
    """创建单个固件文件"""
    
    if not os.path.exists(firmware_path):
        print(f"❌ 固件文件不存在: {firmware_path}")
        return False
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    # 根据命名风格确定文件名
    if naming_style == "standard":
        # 标准模式：firmware.bin + version.txt
        firmware_dest = os.path.join(output_dir, "firmware.bin")
        version_file = os.path.join(output_dir, "version.txt")
        
        # 复制固件文件
        with open(firmware_path, 'rb') as src, open(firmware_dest, 'wb') as dst:
            dst.write(src.read())
        
        # 创建版本文件
        with open(version_file, 'w') as f:
            f.write(version)
            
    elif naming_style == "embedded":
        # 嵌入式命名：firmware_v4.1.0.bin
        clean_version = version.replace('v', '')
        firmware_dest = os.path.join(output_dir, f"firmware_v{clean_version}.bin")
        
        # 复制固件文件
        with open(firmware_path, 'rb') as src, open(firmware_dest, 'wb') as dst:
            dst.write(src.read())
    
    elif naming_style == "motobox":
        # MotoBox命名：motobox_v4.1.0.bin
        clean_version = version.replace('v', '')
        firmware_dest = os.path.join(output_dir, f"motobox_v{clean_version}.bin")
        
        # 复制固件文件
        with open(firmware_path, 'rb') as src, open(firmware_dest, 'wb') as dst:
            dst.write(src.read())
    
    print(f"✅ 固件文件已创建: {os.path.basename(firmware_dest)} ({get_file_size(firmware_dest)})")
    return True

def create_multiple_firmwares(firmware_path, versions, output_dir):
    """创建多个版本的固件文件"""
    
    if not os.path.exists(firmware_path):
        print(f"❌ 固件文件不存在: {firmware_path}")
        return False
    
    # 创建输出目录
    os.makedirs(output_dir, exist_ok=True)
    
    print(f"🔧 创建多个固件版本...")
    
    naming_styles = ["standard", "embedded", "motobox"]
    
    for i, version in enumerate(versions):
        # 验证版本号格式
        if not validate_version_format(version, silent=True):
            print(f"⚠️ 跳过无效版本号: {version}")
            continue
        
        # 使用不同的命名风格
        style = naming_styles[i % len(naming_styles)]
        
        if create_single_firmware(firmware_path, version, output_dir, style):
            print(f"   版本 {version} - {style} 风格")
    
    print(f"\n📋 使用说明:")
    print(f"   1. 将 {output_dir} 目录中的所有文件复制到SD卡根目录")
    print(f"   2. 确保设备电池电量≥90%")
    print(f"   3. 插入SD卡并重启设备")
    print(f"   4. 设备会自动选择最新版本进行升级")
    
    return True

def create_sd_upgrade_files(firmware_path, version, output_dir, style="standard"):
    """创建SD卡升级所需的文件（兼容旧接口）"""
    return create_single_firmware(firmware_path, version, output_dir, style)

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
    print("📖 ESP32-S3 MotoBox SD卡多固件升级指南")
    print("\n🔍 升级条件检查:")
    print("   ✅ 电池电量 ≥ 90%")
    print("   ✅ 新版本号 > 当前版本号")
    print("   ✅ SD卡中存在有效的固件文件")
    
    print("\n📁 支持的固件文件格式:")
    print("   1. 标准格式: firmware.bin + version.txt")
    print("   2. 嵌入版本: firmware_v4.1.0.bin")
    print("   3. MotoBox格式: motobox_v4.1.0.bin")
    print("   4. ESP32格式: esp32_v4.1.0.bin")
    
    print("\n🔄 多固件升级逻辑:")
    print("   1. 扫描SD卡中所有支持的固件文件")
    print("   2. 提取每个文件的版本号")
    print("   3. 自动选择版本号最高的固件")
    print("   4. 执行升级到最新版本")
    
    print("\n📋 升级步骤:")
    print("   1. 使用本工具创建固件文件")
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
    print("   • 系统会自动选择最新版本")
    print("   • 升级成功后设备会自动重启")

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

def validate_version_format(version, silent=False):
    """验证版本号格式"""
    import re
    
    # 支持的版本格式: v4.0.0, 4.0.0, v4.0.0+694
    pattern = r'^v?\d+\.\d+\.\d+(\+\d+)?$'
    
    if re.match(pattern, version):
        if not silent:
            print(f"✅ 版本号格式正确: {version}")
        return True
    else:
        if not silent:
            print(f"❌ 版本号格式错误: {version}")
            print("   支持的格式: v4.0.0, 4.0.0, v4.0.0+694")
        return False

def show_examples():
    """显示使用示例"""
    print("💡 ESP32-S3 MotoBox SD卡OTA升级工具使用示例")
    
    print("\n1️⃣ 创建单个固件文件:")
    print("   python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0")
    print("   python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0 --style embedded")
    
    print("\n2️⃣ 创建多个版本固件:")
    print("   python sd_ota_test.py --create-multi --firmware firmware.bin --versions v4.0.0,v4.1.0,v4.2.0")
    
    print("\n3️⃣ 验证固件文件:")
    print("   python sd_ota_test.py --validate --firmware firmware.bin")
    
    print("\n4️⃣ 显示升级指南:")
    print("   python sd_ota_test.py --guide")
    
    print("\n📁 命名风格说明:")
    print("   • standard: firmware.bin + version.txt")
    print("   • embedded: firmware_v4.1.0.bin")
    print("   • motobox: motobox_v4.1.0.bin")

def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 MotoBox SD卡OTA升级测试工具")
    parser.add_argument("--create", action="store_true", help="创建单个固件文件")
    parser.add_argument("--create-multi", action="store_true", help="创建多个版本固件文件")
    parser.add_argument("--firmware", type=str, help="固件文件路径")
    parser.add_argument("--version", type=str, help="固件版本号")
    parser.add_argument("--versions", type=str, help="多个版本号，用逗号分隔")
    parser.add_argument("--style", type=str, choices=["standard", "embedded", "motobox"], 
                       default="standard", help="文件命名风格")
    parser.add_argument("--output", type=str, default="./sd_upgrade", help="输出目录")
    parser.add_argument("--guide", action="store_true", help="显示升级指南")
    parser.add_argument("--validate", action="store_true", help="验证固件文件")
    parser.add_argument("--examples", action="store_true", help="显示使用示例")
    
    args = parser.parse_args()
    
    if args.create:
        if not args.firmware or not args.version:
            print("❌ 创建固件文件需要指定 --firmware 和 --version 参数")
            print("   示例: python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0")
            return
        
        # 验证版本号格式
        if not validate_version_format(args.version):
            return
        
        # 验证固件文件
        if not validate_firmware_file(args.firmware):
            return
        
        create_single_firmware(args.firmware, args.version, args.output, args.style)
    
    elif args.create_multi:
        if not args.firmware or not args.versions:
            print("❌ 创建多个固件文件需要指定 --firmware 和 --versions 参数")
            print("   示例: python sd_ota_test.py --create-multi --firmware firmware.bin --versions v4.0.0,v4.1.0,v4.2.0")
            return
        
        # 验证固件文件
        if not validate_firmware_file(args.firmware):
            return
        
        versions = [v.strip() for v in args.versions.split(',')]
        create_multiple_firmwares(args.firmware, versions, args.output)
    
    elif args.validate:
        if not args.firmware:
            print("❌ 验证固件文件需要指定 --firmware 参数")
            return
        validate_firmware_file(args.firmware)
    
    elif args.guide:
        show_upgrade_guide()
    
    elif args.examples:
        show_examples()
    
    else:
        print("🔧 ESP32-S3 MotoBox SD卡多固件OTA升级测试工具")
        print("\n✨ 新功能: 支持多个固件文件，自动选择最新版本升级")
        print("\n使用示例:")
        print("  # 创建单个固件文件")
        print("  python sd_ota_test.py --create --firmware firmware.bin --version v4.1.0")
        print("\n  # 创建多个版本固件文件")
        print("  python sd_ota_test.py --create-multi --firmware firmware.bin --versions v4.0.0,v4.1.0,v4.2.0")
        print("\n  # 显示更多示例")
        print("  python sd_ota_test.py --examples")
        print("\n使用 --help 查看详细帮助")

if __name__ == "__main__":
    main()
