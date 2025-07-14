#!/usr/bin/env python3
"""
ESP32-S3 MotoBox 固件发布脚本
支持自动版本管理和固件发布
"""

import os
import re
import shutil
import argparse
import subprocess
from pathlib import Path
from datetime import datetime

class FirmwareReleaser:
    def __init__(self):
        self.project_root = Path(__file__).parent.parent
        self.build_dir = self.project_root / ".pio" / "build" / "esp32-air780eg"
        self.release_dir = self.project_root / "releases"
        self.version_file = self.project_root / "src" / "version.h"
        
    def get_current_version(self):
        """从version.h获取当前版本"""
        try:
            with open(self.version_file, 'r') as f:
                content = f.read()
                
            # 查找FIRMWARE_VERSION定义
            version_match = re.search(r'#define\s+FIRMWARE_VERSION\s+"([^"]+)"', content)
            if version_match:
                return version_match.group(1)
            else:
                print("❌ 无法从version.h中找到FIRMWARE_VERSION")
                return None
        except Exception as e:
            print(f"❌ 读取版本文件失败: {e}")
            return None
    
    def parse_version(self, version_str):
        """解析版本号，支持 v4.0.0+697 格式"""
        # 移除v前缀
        version_str = version_str.replace('v', '')
        
        # 分离主版本号和构建号
        if '+' in version_str:
            main_version, build_number = version_str.split('+')
        else:
            main_version = version_str
            build_number = '0'
        
        # 解析主版本号
        try:
            parts = main_version.split('.')
            major = int(parts[0]) if len(parts) > 0 else 0
            minor = int(parts[1]) if len(parts) > 1 else 0
            patch = int(parts[2]) if len(parts) > 2 else 0
            build = int(build_number)
            
            return {
                'major': major,
                'minor': minor,
                'patch': patch,
                'build': build,
                'main_version': main_version,
                'full_version': version_str
            }
        except ValueError as e:
            print(f"❌ 版本号解析失败: {version_str}, 错误: {e}")
            return None
    
    def compare_versions(self, version1, version2):
        """比较两个版本号，返回 1(v1>v2), 0(v1=v2), -1(v1<v2)"""
        v1 = self.parse_version(version1)
        v2 = self.parse_version(version2)
        
        if not v1 or not v2:
            return 0
        
        # 比较主版本号
        if v1['major'] != v2['major']:
            return 1 if v1['major'] > v2['major'] else -1
        if v1['minor'] != v2['minor']:
            return 1 if v1['minor'] > v2['minor'] else -1
        if v1['patch'] != v2['patch']:
            return 1 if v1['patch'] > v2['patch'] else -1
        
        # 比较构建号
        if v1['build'] != v2['build']:
            return 1 if v1['build'] > v2['build'] else -1
        
        return 0
    
    def scan_existing_releases(self):
        """扫描已有的发布版本"""
        if not self.release_dir.exists():
            return []
        
        releases = []
        pattern = re.compile(r'motobox_v(\d+\.\d+\.\d+(?:\+\d+)?).bin')
        
        for file_path in self.release_dir.glob("motobox_v*.bin"):
            match = pattern.match(file_path.name)
            if match:
                version = match.group(1)
                releases.append({
                    'version': version,
                    'file_path': file_path,
                    'file_name': file_path.name,
                    'size': file_path.stat().st_size,
                    'mtime': datetime.fromtimestamp(file_path.stat().st_mtime)
                })
        
        # 按版本号排序
        def version_sort_key(release):
            parsed = self.parse_version(release['version'])
            if parsed:
                return (parsed['major'], parsed['minor'], parsed['patch'], parsed['build'])
            else:
                return (0, 0, 0, 0)
        
        releases.sort(key=version_sort_key, reverse=True)
        
        return releases
    
    def get_latest_release_version(self):
        """获取最新发布版本"""
        releases = self.scan_existing_releases()
        if releases:
            return releases[0]['version']
        return None
    
    def build_firmware(self):
        """编译固件"""
        print("🔨 开始编译固件...")
        
        try:
            # 执行编译命令
            result = subprocess.run(
                ["pio", "run", "-e", "esp32-air780eg"],
                cwd=self.project_root,
                capture_output=True,
                text=True
            )
            
            if result.returncode == 0:
                print("✅ 固件编译成功")
                return True
            else:
                print("❌ 固件编译失败:")
                print(result.stderr)
                return False
                
        except Exception as e:
            print(f"❌ 编译过程出错: {e}")
            return False
    
    def create_release(self, version=None, force=False):
        """创建发布版本"""
        # 获取当前版本
        current_version = self.get_current_version()
        if not current_version:
            return False
        
        # 如果指定了版本，使用指定版本，否则使用当前版本
        release_version = version if version else current_version
        
        print(f"📦 准备发布版本: {release_version}")
        
        # 检查是否已存在该版本
        latest_version = self.get_latest_release_version()
        if latest_version and not force:
            comparison = self.compare_versions(release_version, latest_version)
            if comparison <= 0:
                print(f"⚠️ 版本 {release_version} 不比最新版本 {latest_version} 新")
                if comparison == 0:
                    print("   版本号相同，使用 --force 强制发布")
                else:
                    print("   版本号较旧，请检查版本号设置")
                return False
        
        # 编译固件
        if not self.build_firmware():
            return False
        
        # 检查编译产物
        firmware_bin = self.build_dir / "firmware.bin"
        if not firmware_bin.exists():
            print("❌ 找不到编译后的固件文件")
            return False
        
        # 创建发布目录
        self.release_dir.mkdir(exist_ok=True)
        
        # 生成发布文件名
        clean_version = release_version.replace('v', '')
        release_filename = f"motobox_v{clean_version}.bin"
        release_path = self.release_dir / release_filename
        
        # 复制固件文件
        try:
            shutil.copy2(firmware_bin, release_path)
            print(f"✅ 固件已发布: {release_filename}")
            
            # 显示文件信息
            file_size = release_path.stat().st_size
            size_mb = file_size / (1024 * 1024)
            print(f"   📁 文件路径: {release_path}")
            print(f"   📏 文件大小: {size_mb:.2f} MB ({file_size} 字节)")
            print(f"   🕒 发布时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            
            return True
            
        except Exception as e:
            print(f"❌ 复制固件文件失败: {e}")
            return False
    
    def list_releases(self):
        """列出所有发布版本"""
        releases = self.scan_existing_releases()
        
        if not releases:
            print("📋 暂无发布版本")
            return
        
        print("📋 已发布的固件版本:")
        print("-" * 80)
        print(f"{'版本号':<20} {'文件名':<30} {'大小':<10} {'发布时间':<20}")
        print("-" * 80)
        
        for release in releases:
            size_mb = release['size'] / (1024 * 1024)
            mtime_str = release['mtime'].strftime('%Y-%m-%d %H:%M:%S')
            print(f"{release['version']:<20} {release['file_name']:<30} {size_mb:.2f}MB{'':<4} {mtime_str}")
    
    def check_version_status(self):
        """检查版本状态"""
        current_version = self.get_current_version()
        latest_version = self.get_latest_release_version()
        
        print("🔍 版本状态检查:")
        print(f"   当前版本: {current_version}")
        print(f"   最新发布: {latest_version if latest_version else '无'}")
        
        if current_version and latest_version:
            comparison = self.compare_versions(current_version, latest_version)
            if comparison > 0:
                print("   状态: ✅ 当前版本较新，可以发布")
            elif comparison == 0:
                print("   状态: ⚠️ 版本号相同")
            else:
                print("   状态: ❌ 当前版本较旧")
        elif current_version and not latest_version:
            print("   状态: ✅ 首次发布")
        else:
            print("   状态: ❌ 无法确定版本信息")
    
    def clean_old_releases(self, keep_count=5):
        """清理旧版本，保留最新的几个版本"""
        releases = self.scan_existing_releases()
        
        if len(releases) <= keep_count:
            print(f"📋 当前有 {len(releases)} 个版本，无需清理")
            return
        
        to_remove = releases[keep_count:]
        print(f"🗑️ 准备清理 {len(to_remove)} 个旧版本:")
        
        for release in to_remove:
            print(f"   删除: {release['file_name']} (版本: {release['version']})")
            try:
                release['file_path'].unlink()
                print(f"   ✅ 已删除: {release['file_name']}")
            except Exception as e:
                print(f"   ❌ 删除失败: {e}")

def main():
    parser = argparse.ArgumentParser(description="ESP32-S3 MotoBox 固件发布工具")
    parser.add_argument("--release", action="store_true", help="创建发布版本")
    parser.add_argument("--version", type=str, help="指定发布版本号")
    parser.add_argument("--force", action="store_true", help="强制发布（即使版本号相同）")
    parser.add_argument("--list", action="store_true", help="列出所有发布版本")
    parser.add_argument("--status", action="store_true", help="检查版本状态")
    parser.add_argument("--clean", type=int, metavar="N", help="清理旧版本，保留最新N个")
    parser.add_argument("--build", action="store_true", help="仅编译固件")
    
    args = parser.parse_args()
    
    releaser = FirmwareReleaser()
    
    if args.build:
        releaser.build_firmware()
    elif args.release:
        releaser.create_release(args.version, args.force)
    elif args.list:
        releaser.list_releases()
    elif args.status:
        releaser.check_version_status()
    elif args.clean is not None:
        releaser.clean_old_releases(args.clean)
    else:
        print("🚀 ESP32-S3 MotoBox 固件发布工具")
        print("\n使用示例:")
        print("  # 检查版本状态")
        print("  python firmware_release.py --status")
        print("\n  # 创建发布版本")
        print("  python firmware_release.py --release")
        print("\n  # 指定版本号发布")
        print("  python firmware_release.py --release --version v4.2.0")
        print("\n  # 列出所有版本")
        print("  python firmware_release.py --list")
        print("\n  # 清理旧版本，保留最新5个")
        print("  python firmware_release.py --clean 5")
        print("\n使用 --help 查看详细帮助")

if __name__ == "__main__":
    main()
