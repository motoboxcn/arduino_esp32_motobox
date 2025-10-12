# BLE设备名称示例

## 设备名称格式

BLE设备名称现在使用格式：`MotoBox-{设备ID}`

## 设备ID生成

设备ID通过ESP32的MAC地址生成，格式为12位十六进制字符串。

### 示例设备名称

| MAC地址 | 设备ID | BLE设备名称 |
|---------|--------|-------------|
| A1:B2:C3:D4:E5:F6 | A1B2C3D4E5F6 | MotoBox-A1B2C3D4E5F6 |
| 24:6F:28:12:34:56 | 246F28123456 | MotoBox-246F28123456 |
| 3C:61:05:78:9A:BC | 3C6105789ABC | MotoBox-3C6105789ABC |

## 客户端扫描示例

### Python客户端
```python
# 扫描所有MotoBox设备
devices = await BleakScanner.discover()
motobox_devices = []
for device in devices:
    if device.name and device.name.startswith("MotoBox-"):
        motobox_devices.append(device)
        print(f"发现MotoBox设备: {device.name}")
```

### Android客户端
```java
// 扫描MotoBox设备
BluetoothLeScanner scanner = bluetoothAdapter.getBluetoothLeScanner();
ScanFilter filter = new ScanFilter.Builder()
    .setDeviceName("MotoBox-")
    .build();
List<ScanFilter> filters = Arrays.asList(filter);
ScanSettings settings = new ScanSettings.Builder()
    .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
    .build();
scanner.startScan(filters, settings, scanCallback);
```

### iOS客户端
```swift
// 扫描MotoBox设备
let services = [CBUUID(string: "12345678-1234-1234-1234-123456789ABC")]
centralManager.scanForPeripherals(withServices: services, options: nil)

// 在回调中检查设备名称
func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
    if let name = peripheral.name, name.hasPrefix("MotoBox-") {
        print("发现MotoBox设备: \(name)")
    }
}
```

## 配置说明

### 编译时配置
```cpp
#define BLE_DEVICE_NAME_PREFIX "MotoBox-"
```

### 运行时生成
```cpp
// 在BLEManager::begin()中
String deviceName = String(BLE_DEVICE_NAME_PREFIX) + deviceId;
// 例如：deviceName = "MotoBox-A1B2C3D4E5F6"
```

## 优势

1. **唯一性**：每个设备都有唯一的BLE名称
2. **可识别性**：名称包含设备ID，便于识别特定设备
3. **兼容性**：保持"MotoBox"前缀，便于客户端识别
4. **可扩展性**：可以轻松添加更多设备信息到名称中

## 注意事项

1. **名称长度**：BLE设备名称有长度限制（通常31字节）
2. **字符限制**：只能使用ASCII字符
3. **客户端兼容**：客户端需要支持前缀匹配
4. **调试友好**：设备名称包含设备ID，便于调试和识别
