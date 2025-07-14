#include "VoicePrompt.h"
#include "config.h"

// 全局实例
VoicePrompt voicePrompt(BUZZER_PIN);

VoicePrompt::VoicePrompt(int pin) : buzzerPin(pin), isEnabled(true) {
}

void VoicePrompt::begin() {
    if (buzzerPin >= 0) {
        pinMode(buzzerPin, OUTPUT);
        digitalWrite(buzzerPin, LOW);
    }
    
    Serial.println("[VoicePrompt] 语音提示模块初始化完成");
}

void VoicePrompt::setBuzzerPin(int pin) {
    buzzerPin = pin;
    if (buzzerPin >= 0) {
        pinMode(buzzerPin, OUTPUT);
        digitalWrite(buzzerPin, LOW);
    }
}

void VoicePrompt::playPrompt(String message, PromptType type) {
    if (!isEnabled) return;
    
    logPrompt(message, type);
    playToneSequence(type);
}

void VoicePrompt::playUpgradePrompt(String message) {
    if (!isEnabled) return;
    
    PromptType type = PROMPT_UPGRADE_START;
    
    if (message.indexOf("检测到") >= 0) {
        type = PROMPT_INFO;
    } else if (message.indexOf("开始") >= 0 || message.indexOf("正在") >= 0) {
        type = PROMPT_UPGRADE_START;
    } else if (message.indexOf("成功") >= 0 || message.indexOf("完成") >= 0) {
        type = PROMPT_UPGRADE_SUCCESS;
    } else if (message.indexOf("失败") >= 0 || message.indexOf("错误") >= 0) {
        type = PROMPT_UPGRADE_FAILED;
    } else if (message.indexOf("进度") >= 0 || message.indexOf("%") >= 0) {
        type = PROMPT_UPGRADE_PROGRESS;
    }
    
    playPrompt(message, type);
}

void VoicePrompt::playToneSequence(PromptType type) {
    if (buzzerPin < 0) return;
    
    switch (type) {
        case PROMPT_INFO:
            // 单声短促提示音
            playTone(1000, 200, 1);
            break;
            
        case PROMPT_WARNING:
            // 两声中等提示音
            playTone(800, 300, 2);
            break;
            
        case PROMPT_SUCCESS:
        case PROMPT_UPGRADE_SUCCESS:
            // 三声上升音调
            playTone(800, 200);
            delay(100);
            playTone(1000, 200);
            delay(100);
            playTone(1200, 300);
            break;
            
        case PROMPT_ERROR:
        case PROMPT_UPGRADE_FAILED:
            // 长声低音错误提示
            playTone(400, 1000, 1);
            break;
            
        case PROMPT_UPGRADE_START:
            // 三声短促提示音
            playTone(1000, 200, 3);
            break;
            
        case PROMPT_UPGRADE_PROGRESS:
            // 单声极短提示音
            playTone(1200, 100, 1);
            break;
            
        default:
            playTone(1000, 200, 1);
            break;
    }
}

void VoicePrompt::playTone(int frequency, int duration, int times) {
    if (buzzerPin < 0) return;
    
    for (int i = 0; i < times; i++) {
        tone(buzzerPin, frequency, duration);
        delay(duration);
        if (i < times - 1) {
            delay(150); // 间隔时间
        }
    }
    noTone(buzzerPin);
}

void VoicePrompt::logPrompt(String message, PromptType type) {
    Serial.print("[VoicePrompt] [");
    Serial.print(getPromptTypeString(type));
    Serial.print("] ");
    Serial.println(message);
}

String VoicePrompt::getPromptTypeString(PromptType type) {
    switch (type) {
        case PROMPT_INFO: return "INFO";
        case PROMPT_WARNING: return "WARNING";
        case PROMPT_SUCCESS: return "SUCCESS";
        case PROMPT_ERROR: return "ERROR";
        case PROMPT_UPGRADE_START: return "UPGRADE_START";
        case PROMPT_UPGRADE_PROGRESS: return "UPGRADE_PROGRESS";
        case PROMPT_UPGRADE_SUCCESS: return "UPGRADE_SUCCESS";
        case PROMPT_UPGRADE_FAILED: return "UPGRADE_FAILED";
        default: return "UNKNOWN";
    }
}
