# Workflow 上下文规则

## 🔄 当前 Namespace Workflow 配置

你必须严格遵循以下 workflow 配置进行工作和协作：
执行命令获取最新的 workflow 配置：
```bash
cliExtra workflow show <namespace>
```

## 🎯 基于 Workflow 的行为指导

### 强制性遵循规则
根据上述 workflow 配置，你必须：

1. **角色职责遵循**
   - 严格按照你的角色定义中的 responsibilities 工作
   - 只使用角色定义中允许的 tools
   - 专注于你的专业领域，避免跨角色操作

2. **协作关系执行**
   - 当触发 collaboration 中定义的条件时，必须主动执行协作
   - 使用 message_template 中定义的消息格式
   - 根据 priority 确定协作的紧急程度

3. **开发流程遵循**
   - 按照 workflow.development 中定义的步骤顺序工作
   - 完成当前步骤后，主动通知 next 步骤的负责人
   - 检查 dependencies，确保前置条件满足
   - 交付 deliverables 中定义的成果物

4. **自动化通知执行**
   - 使用 notifications.templates 中定义的消息格式
   - 通过 cliExtra send 或 cliExtra broadcast 发送通知
   - 在任务完成、需要协作、发现问题时主动通知

### 协作执行模板

#### 任务完成通知
```bash
cliExtra send <target_instance> "任务完成通知：
- 完成步骤：{当前步骤}
- 交付成果：{deliverables列表}
- 下一步骤：{next步骤}
- 负责人：{next步骤的owner}
- 依赖检查：{dependencies状态}

请开始下一步骤的工作。"
```

#### 协作请求
```bash
cliExtra send <target_instance> "协作请求：
- 触发条件：{collaboration.trigger}
- 请求类型：{collaboration.action}
- 优先级：{collaboration.priority}
- 详细说明：{具体内容}

请及时响应协作请求。"
```

#### 问题报告
```bash
cliExtra send <target_instance> "问题报告：
- 问题描述：{具体问题}
- 影响范围：{受影响的步骤或功能}
- 需要协助：{required_role}
- 紧急程度：{根据workflow判断}

请提供技术支持。"
```

### 工作流程检查清单

在执行任何任务时，必须检查：
- [ ] 当前步骤是否符合 workflow 定义
- [ ] 前置依赖是否已满足
- [ ] 角色职责是否匹配
- [ ] 需要协作的实例是否已通知
- [ ] 交付成果是否完整
- [ ] 下一步骤是否已安排

---

**重要**: 这个 workflow 配置是强制性的，必须严格遵循。任何偏离 workflow 的行为都可能影响团队协作效率。

