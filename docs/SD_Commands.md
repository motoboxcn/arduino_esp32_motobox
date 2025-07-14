# SD卡串口命令使用指南

## 概述
ESP32-S3 MotoBox 支持通过串口命令对SD卡进行丰富的文件系统操作。这些命令可以帮助你管理SD卡上的文件和目录，进行调试和数据管理。

## 基本命令

### sd.info
显示SD卡的详细信息，包括容量、剩余空间、使用率等。
```
>>> sd.info
=== SD卡信息 ===
设备ID: ESP32_XXXXXX
总容量: 32768 MB
剩余空间: 30240 MB
使用率: 8%
初始化状态: 已初始化
```

### sd.status
检查SD卡的当前状态和基本读写功能。
```
>>> sd.status
=== SD卡状态检查 ===
✅ SD卡状态: 已初始化
正在测试基本读写功能...
✅ 读写功能正常
剩余空间: 30240 MB
```

### sd.help
显示所有可用的SD卡命令帮助信息。

## 文件操作命令

### sd.ls [path]
列出指定目录的内容，如果不指定路径则列出根目录。
```
>>> sd.ls
=== 目录列表: / ===
  [DIR]  data/
  [DIR]  config/
  [DIR]  logs/
  [FILE] device.json (1.2 KB)
--- 统计信息 ---
目录: 3 个, 文件: 1 个
总大小: 1.2 KB

>>> sd.ls /data
=== 目录列表: /data ===
  [DIR]  gps/
  [FILE] session_001.json (856 B)
```

### sd.tree
显示SD卡的目录树结构（最大深度3层）。
```
>>> sd.tree
=== SD卡目录树 ===
📁 /
├── 📁 data/
│   ├── 📁 gps/
│   │   ├── 📄 20250714_001.json (2.1 KB)
│   │   └── 📄 20250714_002.json (1.8 KB)
│   └── 📄 session_001.json (856 B)
├── 📁 config/
│   └── 📄 device.json (1.2 KB)
└── 📁 logs/
```

### sd.cat <file>
显示文件内容（限制10KB以内的文件）。
```
>>> sd.cat /config/device.json
文件大小: 1.2 KB
--- 文件内容开始 ---
{
  "device_id": "ESP32_XXXXXX",
  "firmware_version": "v4.1.0",
  "hardware_version": "v1.0"
}
--- 文件内容结束 ---
```

### sd.mkdir <directory>
创建新目录，支持递归创建父目录。
```
>>> sd.mkdir /logs/test
正在创建目录: /logs/test
✅ 目录创建成功: /logs/test

>>> sd.mkdir /data/backup/2025
正在创建目录: /data/backup/2025
✅ 目录创建成功: /data/backup/2025
```

### sd.rm <path>
删除文件或目录（目录会递归删除所有内容）。
```
>>> sd.rm /logs/old_file.txt
⚠️ 正在删除: /logs/old_file.txt
注意: 此操作不可恢复！
✅ 删除成功: /logs/old_file.txt

>>> sd.rm /temp
⚠️ 正在删除: /temp
注意: 此操作不可恢复！
✅ 删除成功: /temp
```

## GPS相关命令

### sd.test
测试GPS数据记录功能。
```
>>> sd.test
正在测试GPS数据记录...
测试数据: 北京天安门广场坐标
当前会话文件: /data/gps/20250714_001.json
✅ GPS数据记录测试成功
```

### sd.session
显示当前GPS记录会话信息。
```
>>> sd.session
=== GPS会话信息 ===
当前会话文件: /data/gps/20250714_001.json
启动次数: 42
运行时间: 3600 秒
设备ID: ESP32_XXXXXX
```

### sd.finish
结束当前GPS记录会话。
```
>>> sd.finish
正在结束当前GPS会话...
✅ GPS会话已正确结束
```

### sd.dirs
检查和创建GPS相关的目录结构。
```
>>> sd.dirs
=== 目录状态检查 ===
/data 目录: 存在
/data/gps 目录: 存在
/config 目录: 存在

正在确保GPS目录存在...
✅ GPS目录检查/创建成功
```

## 系统操作命令

### sd.fmt
显示SD卡格式化说明（ESP32不支持直接格式化）。
```
>>> sd.fmt
⚠️ 格式化SD卡功能
警告: 此操作将删除SD卡上的所有数据！
ESP32-S3不支持直接格式化SD卡
建议操作:
  1. 将SD卡取出
  2. 使用电脑格式化为FAT32格式
  3. 重新插入SD卡
  4. 重启设备
```

## 使用示例

### 日常文件管理
```bash
# 查看SD卡状态
>>> sd.status

# 浏览根目录
>>> sd.ls

# 查看目录树
>>> sd.tree

# 创建日志目录
>>> sd.mkdir /logs/2025/07

# 查看配置文件
>>> sd.cat /config/device.json

# 清理临时文件
>>> sd.rm /temp/cache.tmp
```

### GPS数据管理
```bash
# 检查GPS目录
>>> sd.dirs

# 查看当前会话
>>> sd.session

# 测试GPS记录
>>> sd.test

# 查看GPS数据文件
>>> sd.ls /data/gps

# 查看具体GPS记录
>>> sd.cat /data/gps/20250714_001.json
```

## 注意事项

1. **文件大小限制**: `sd.cat` 命令只能显示10KB以内的文件，超过限制只显示前1024字节
2. **删除操作**: `sd.rm` 命令的删除操作不可恢复，请谨慎使用
3. **目录创建**: `sd.mkdir` 支持递归创建父目录
4. **路径格式**: 所有路径都应以 `/` 开头，如 `/data/gps`
5. **SD卡状态**: 所有命令都需要SD卡正确初始化，如果SD卡未就绪会显示错误信息

## 错误处理

如果遇到以下错误：
- `❌ SD卡未初始化`: 检查SD卡是否正确插入，重启设备
- `❌ 无法打开目录/文件`: 检查路径是否正确，文件是否存在
- `❌ 路径不是目录`: 确认指定的路径是目录而不是文件
- `❌ 目录创建失败`: 检查SD卡剩余空间，确认父目录存在

## 技术实现

这些命令通过 `SDManager` 类实现，支持：
- SPI模式和SDMMC模式的SD卡
- 递归目录操作
- 文件大小格式化显示
- 错误处理和状态检查
- 内存优化的文件读取

所有操作都经过充分测试，确保在ESP32-S3平台上稳定运行。
