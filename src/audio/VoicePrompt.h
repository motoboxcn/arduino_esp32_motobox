#ifndef VOICE_PROMPT_H
#define VOICE_PROMPT_H

#include <Arduino.h>

// 语音提示类型枚举
enum PromptType {
    PROMPT_INFO,
    PROMPT_WARNING,
    PROMPT_SUCCESS,
    PROMPT_ERROR,
    PROMPT_UPGRADE_START,
    PROMPT_UPGRADE_PROGRESS,
    PROMPT_UPGRADE_SUCCESS,
    PROMPT_UPGRADE_FAILED
};

class VoicePrompt {
public:
    VoicePrompt(int buzzerPin = -1);
    
    // 初始化
    void begin();
    
    // 播放语音提示
    void playPrompt(String message, PromptType type = PROMPT_INFO);
    void playUpgradePrompt(String message);
    
    // 设置蜂鸣器引脚
    void setBuzzerPin(int pin);
    
    // 启用/禁用语音提示
    void setEnabled(bool enabled) { isEnabled = enabled; }
    bool getEnabled() { return isEnabled; }
    
private:
    int buzzerPin;
    bool isEnabled;
    
    // 蜂鸣器相关
    void playTone(int frequency, int duration, int times = 1);
    void playToneSequence(PromptType type);
    
    // 日志输出
    void logPrompt(String message, PromptType type);
    
    // 获取提示类型字符串
    String getPromptTypeString(PromptType type);
};

extern VoicePrompt voicePrompt;

#endif // VOICE_PROMPT_H
