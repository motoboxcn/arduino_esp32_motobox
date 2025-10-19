# GitHub Secrets 配置说明

## 1. 添加SSH私钥到GitHub Secrets

### 步骤：
1. 进入GitHub仓库 → Settings → Secrets and variables → Actions
2. 点击 "New repository secret"
3. Name: `SERVER_SSH_KEY`
4. Value: 复制以下SSH私钥

```
-----BEGIN OPENSSH PRIVATE KEY-----
b3BlbnNzaC1rZXktdjEAAAAABG5vbmUAAAAEbm9uZQAAAAAAAAABAAAAMwAAAAtzc2gtZW
QyNTUxOQAAACADTMrPpBLA+6amb9d6IbiId+KYJwU8ARAITG3zl/HYLwAAAKDcghli3IIZ
YgAAAAtzc2gtZWQyNTUxOQAAACADTMrPpBLA+6amb9d6IbiId+KYJwU8ARAITG3zl/HYLw
AAAEBlaiR0KcI+GXzT7kK+GlGNpDnvdEkbFbY7KBx6SxhF8wNMys+kEsD7pqZv13ohuIh3
4pgnBTwBEAhMbfOX8dgvAAAAGWdpdGh1Yi1hY3Rpb25zQGRhYm9sdW8uY2MBAgME
-----END OPENSSH PRIVATE KEY-----
```

## 2. 测试部署

### 手动触发工作流：
1. 进入GitHub仓库 → Actions
2. 选择 "Build and Deploy OTA Firmware"
3. 点击 "Run workflow"
4. 输入版本号（如：v4.3.0）
5. 点击 "Run workflow"

### 自动触发（Release）：
1. 进入GitHub仓库 → Releases
2. 点击 "Create a new release"
3. 输入Tag version（如：v4.3.0）
4. 填写Release title和描述
5. 点击 "Publish release"

## 3. 验证部署

部署成功后，固件将可通过以下地址下载：
```
https://daboluo.cc/ota/firmware/esp32-air780eg/v4.3.0.bin
```

## 4. MQTT测试消息

发送到设备进行OTA升级：
```json
{
  "latest_version": "v4.3.0",
  "download_url": "https://daboluo.cc/ota/firmware/esp32-air780eg/v4.3.0.bin",
  "force_update": false,
  "release_notes": "GitHub Actions自动构建版本"
}
```

## 5. 安全注意事项

- SSH私钥仅用于GitHub Actions部署
- 私钥已限制只能访问固件目录
- 建议定期轮换SSH密钥
- 监控部署日志确保安全
