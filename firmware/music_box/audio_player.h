#pragma once

#include <Arduino.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioGeneratorFLAC.h>
#include <AudioOutputI2S.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  音频播放模块
//  ESP32-audioI2S 库 → PCM5102A DAC → ZK-502C（TPA3116D2）
// ============================================================

static AudioFileSourceSD  *g_audioSource = nullptr;
static AudioGeneratorMP3  *g_mp3         = nullptr;
static AudioGeneratorWAV  *g_wav         = nullptr;
static AudioGeneratorFLAC *g_flac        = nullptr;
static AudioOutputI2S     *g_i2sOut      = nullptr;
static int g_volume = AUDIO_VOLUME_DEFAULT;

// 当前使用的解码器类型
enum AudioType { AUDIO_NONE, AUDIO_MP3, AUDIO_WAV, AUDIO_FLAC };
static AudioType g_audioType = AUDIO_NONE;

// 初始化 I2S 音频输出
void audio_init() {
    g_i2sOut = new AudioOutputI2S();
    g_i2sOut->SetPinout(PIN_I2S_BCK, PIN_I2S_LRCK, PIN_I2S_DATA);
    g_i2sOut->SetOutputModeMono(false); // 立体声
    g_i2sOut->SetGain((float)g_volume / 100.0f);
    Serial.println("[Audio] I2S 初始化成功");
}

// 停止当前播放
void audio_stop() {
    if (g_mp3 && g_mp3->isRunning()) {
        g_mp3->stop();
    }
    if (g_wav && g_wav->isRunning()) {
        g_wav->stop();
    }
    if (g_flac && g_flac->isRunning()) {
        g_flac->stop();
    }
    if (g_audioSource) {
        delete g_audioSource;
        g_audioSource = nullptr;
    }
    if (g_mp3)   { delete g_mp3;   g_mp3  = nullptr; }
    if (g_wav)   { delete g_wav;   g_wav  = nullptr; }
    if (g_flac)  { delete g_flac;  g_flac = nullptr; }
    g_audioType = AUDIO_NONE;
}

// 根据文件扩展名启动播放
bool audio_play(const char *filePath) {
    audio_stop();

    g_audioSource = new AudioFileSourceSD(filePath);
    if (!g_audioSource) {
        Serial.println("[Audio] 无法创建音频源");
        return false;
    }

    // 根据扩展名选择解码器
    const char *ext = strrchr(filePath, '.');
    if (ext && strcasecmp(ext, ".mp3") == 0) {
        g_mp3 = new AudioGeneratorMP3();
        if (!g_mp3->begin(g_audioSource, g_i2sOut)) {
            Serial.println("[Audio] MP3 启动失败");
            audio_stop();
            return false;
        }
        g_audioType = AUDIO_MP3;
    } else if (ext && strcasecmp(ext, ".wav") == 0) {
        g_wav = new AudioGeneratorWAV();
        if (!g_wav->begin(g_audioSource, g_i2sOut)) {
            Serial.println("[Audio] WAV 启动失败");
            audio_stop();
            return false;
        }
        g_audioType = AUDIO_WAV;
    } else if (ext && strcasecmp(ext, ".flac") == 0) {
        g_flac = new AudioGeneratorFLAC();
        if (!g_flac->begin(g_audioSource, g_i2sOut)) {
            Serial.println("[Audio] FLAC 启动失败");
            audio_stop();
            return false;
        }
        g_audioType = AUDIO_FLAC;
    } else {
        Serial.printf("[Audio] 不支持的格式: %s\n", filePath);
        audio_stop();
        return false;
    }

    Serial.printf("[Audio] 开始播放: %s\n", filePath);
    return true;
}

// 在主循环中调用，驱动音频解码（必须频繁调用）
// 返回 true 表示正在播放，false 表示播放结束
bool audio_loop() {
    switch (g_audioType) {
        case AUDIO_MP3:
            if (g_mp3 && g_mp3->isRunning()) {
                if (!g_mp3->loop()) {
                    g_mp3->stop();
                    g_audioType = AUDIO_NONE;
                    return false;
                }
                return true;
            }
            break;
        case AUDIO_WAV:
            if (g_wav && g_wav->isRunning()) {
                if (!g_wav->loop()) {
                    g_wav->stop();
                    g_audioType = AUDIO_NONE;
                    return false;
                }
                return true;
            }
            break;
        case AUDIO_FLAC:
            if (g_flac && g_flac->isRunning()) {
                if (!g_flac->loop()) {
                    g_flac->stop();
                    g_audioType = AUDIO_NONE;
                    return false;
                }
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

// 检查是否正在播放
bool audio_is_playing() {
    switch (g_audioType) {
        case AUDIO_MP3:  return g_mp3  && g_mp3->isRunning();
        case AUDIO_WAV:  return g_wav  && g_wav->isRunning();
        case AUDIO_FLAC: return g_flac && g_flac->isRunning();
        default: return false;
    }
}

// 设置音量 (0–100)
void audio_set_volume(int vol) {
    g_volume = constrain(vol, 0, 100);
    if (g_i2sOut) {
        g_i2sOut->SetGain((float)g_volume / 100.0f);
    }
}

int audio_get_volume() {
    return g_volume;
}

// 音量增加
void audio_volume_up(int step = 5) {
    audio_set_volume(g_volume + step);
}

// 音量减少
void audio_volume_down(int step = 5) {
    audio_set_volume(g_volume - step);
}