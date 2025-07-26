# LED调试功能快速入门

## 🚀 快速启用调试

### 1. 修改配置文件
在 `src/config.h` 中找到并取消注释：
```cpp
#define LED_DEBUG_ENABLED  // 启用LED调试输出
```

### 2. 编译上传
```bash
pio run --environment esp32-air780eg
pio run --target upload --environment esp32-air780eg
```

### 3. 查看调试输出
打开串口监视器（115200波特率），您将看到：
```
[LED_DEBUG] LEDManager 构造函数调用
[LED_DEBUG] 进入函数: LEDManager::begin
[LED_DEBUG] 初始化 PWM LED
[LED_DEBUG] PWM LED 初始化完成 - 模式: LED_ON, 颜色: GREEN, 亮度: 5
```

## 🛠️ 使用自动化脚本

```bash
# 启用调试并编译
python3 test/test_led_debug.py enable

# 禁用调试并编译  
python3 test/test_led_debug.py disable

# 查看调试示例
python3 test/test_led_debug.py examples
```

## 📊 调试输出说明

- `[LED_DEBUG]` - 基本调试信息
- `[LED_DEBUG][时间戳]` - 带时间戳的信息
- `[LED_ERROR]` - 错误信息
- `[LED_WARNING]` - 警告信息

## ⚡ 性能影响

- **启用时**: 增加约2-3KB Flash，1-2% CPU
- **禁用时**: 零性能影响（编译器完全优化掉）

## 🎯 常用场景

1. **LED不亮问题**: 查看初始化和硬件设置日志
2. **充电显示异常**: 监控充电状态变化和模式切换
3. **呼吸效果不正常**: 观察亮度值变化过程
4. **性能优化**: 使用时间戳分析执行时间

详细文档请参考 `docs/led_debug_system.md`
