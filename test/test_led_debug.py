#!/usr/bin/env python3
"""
LED调试功能测试脚本
演示如何启用和使用LED调试输出
"""

import os
import sys
import subprocess
import time

def modify_config_for_debug(enable_debug=True):
    """修改config.h文件以启用或禁用LED调试"""
    config_path = "/Users/mikas/daboluo/arduino_esp32_motobox/src/config.h"
    
    with open(config_path, 'r', encoding='utf-8') as f:
        content = f.read()
    
    if enable_debug:
        # 启用LED调试
        content = content.replace(
            '// #define LED_DEBUG_ENABLED  // 取消注释以启用LED调试输出',
            '#define LED_DEBUG_ENABLED  // LED调试已启用'
        )
        print("✅ LED调试已启用")
    else:
        # 禁用LED调试
        content = content.replace(
            '#define LED_DEBUG_ENABLED  // LED调试已启用',
            '// #define LED_DEBUG_ENABLED  // 取消注释以启用LED调试输出'
        )
        print("❌ LED调试已禁用")
    
    with open(config_path, 'w', encoding='utf-8') as f:
        f.write(content)

def compile_project():
    """编译项目"""
    print("🔨 开始编译项目...")
    os.chdir("/Users/mikas/daboluo/arduino_esp32_motobox")
    
    result = subprocess.run(
        ["pio", "run", "--environment", "esp32-air780eg"],
        capture_output=True,
        text=True
    )
    
    if result.returncode == 0:
        print("✅ 编译成功")
        return True
    else:
        print("❌ 编译失败:")
        print(result.stderr)
        return False

def show_debug_examples():
    """显示调试输出示例"""
    print("\n" + "="*60)
    print("LED调试输出示例")
    print("="*60)
    
    examples = [
        {
            "场景": "LED初始化",
            "输出": "[LED_DEBUG] LEDManager 构造函数调用\n[LED_DEBUG] 进入函数: LEDManager::begin\n[LED_DEBUG] 初始化 PWM LED\n[LED_DEBUG] PWM LED 初始化完成 - 模式: LED_ON, 颜色: GREEN, 亮度: 5"
        },
        {
            "场景": "LED模式变化",
            "输出": "[LED_DEBUG] 状态变化 [LED模式]: LED_ON -> LED_BREATH\n[LED_DEBUG] 设置呼吸模式，初始亮度: 12, 目标亮度: 50"
        },
        {
            "场景": "充电状态检测",
            "输出": "[LED_DEBUG][15234] 状态监控 - 充电: 是, 自动模式: 启用, LED模式: LED_BREATH, 颜色: GREEN, 亮度: 50"
        },
        {
            "场景": "呼吸效果更新",
            "输出": "[LED_DEBUG][15456] 呼吸效果更新: 当前值=25, 目标=50, 递增=是"
        },
        {
            "场景": "LED硬件更新",
            "输出": "[LED_DEBUG][15678] 显示LED: 模式=LED_BREATH, 颜色=GREEN, 设定亮度=50, 实际亮度=25, RGB=(0,255,0)"
        }
    ]
    
    for i, example in enumerate(examples, 1):
        print(f"\n{i}. {example['场景']}:")
        print(f"   {example['输出']}")

def show_debug_macros():
    """显示可用的调试宏"""
    print("\n" + "="*60)
    print("可用的LED调试宏")
    print("="*60)
    
    macros = [
        {
            "宏名": "LED_DEBUG_PRINTF(fmt, ...)",
            "用途": "基本调试输出",
            "示例": 'LED_DEBUG_PRINTF("LED亮度设置为: %d\\n", brightness);'
        },
        {
            "宏名": "LED_DEBUG_TIMESTAMP_PRINTF(fmt, ...)",
            "用途": "带时间戳的调试输出",
            "示例": 'LED_DEBUG_TIMESTAMP_PRINTF("LED状态变化\\n");'
        },
        {
            "宏名": "LED_DEBUG_THROTTLED(interval, fmt, ...)",
            "用途": "限制频率的调试输出",
            "示例": 'LED_DEBUG_THROTTLED(5000, "LED循环状态\\n");'
        },
        {
            "宏名": "LED_DEBUG_STATE_CHANGE(old, new, desc)",
            "用途": "状态变化调试",
            "示例": 'LED_DEBUG_STATE_CHANGE("LED_ON", "LED_BREATH", "模式");'
        },
        {
            "宏名": "LED_DEBUG_ENTER(func) / LED_DEBUG_EXIT(func)",
            "用途": "函数进入/退出跟踪",
            "示例": 'LED_DEBUG_ENTER("setLEDState");'
        },
        {
            "宏名": "LED_DEBUG_ERROR(fmt, ...) / LED_DEBUG_WARNING(fmt, ...)",
            "用途": "错误和警告输出",
            "示例": 'LED_DEBUG_ERROR("LED初始化失败\\n");'
        }
    ]
    
    for macro in macros:
        print(f"\n• {macro['宏名']}")
        print(f"  用途: {macro['用途']}")
        print(f"  示例: {macro['示例']}")

def main():
    """主函数"""
    print("LED调试功能测试")
    print("="*60)
    
    if len(sys.argv) > 1:
        action = sys.argv[1].lower()
        
        if action == "enable":
            modify_config_for_debug(True)
            if compile_project():
                print("\n🎉 LED调试功能已启用并编译成功!")
                print("现在上传固件到设备，您将看到详细的LED调试输出。")
        
        elif action == "disable":
            modify_config_for_debug(False)
            if compile_project():
                print("\n✅ LED调试功能已禁用并编译成功!")
                print("固件将不再输出LED调试信息，节省串口带宽。")
        
        elif action == "examples":
            show_debug_examples()
        
        elif action == "macros":
            show_debug_macros()
        
        else:
            print("❌ 未知操作:", action)
            show_usage()
    
    else:
        show_usage()

def show_usage():
    """显示使用说明"""
    print("\n使用方法:")
    print("  python3 test_led_debug.py enable    # 启用LED调试并编译")
    print("  python3 test_led_debug.py disable   # 禁用LED调试并编译")
    print("  python3 test_led_debug.py examples  # 显示调试输出示例")
    print("  python3 test_led_debug.py macros    # 显示可用的调试宏")
    
    print("\n配置说明:")
    print("  在 src/config.h 中:")
    print("  • 启用: #define LED_DEBUG_ENABLED")
    print("  • 禁用: // #define LED_DEBUG_ENABLED")
    
    print("\n调试输出特点:")
    print("  • 所有调试信息都带有 [LED_DEBUG] 前缀")
    print("  • 支持时间戳显示")
    print("  • 支持频率限制，避免日志过多")
    print("  • 状态变化时自动记录")
    print("  • 编译时可完全禁用，零性能影响")

if __name__ == "__main__":
    main()
