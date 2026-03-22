#pragma once

// ============================================================
//  全局配置
// ============================================================

// ---------- 音频 ----------
#define AUDIO_VOLUME_DEFAULT  70    // 默认音量 (0–100)
#define AUDIO_SAMPLE_RATE  44100    // 采样率 (Hz)

// ---------- NFC ----------
#define NFC_POLL_INTERVAL_MS  300   // NFC 轮询间隔（ms）
#define NFC_DEBOUNCE_MS      1500   // 同一标签再次触发的冷却时间（ms）
#define NFC_UID_MAX_LEN          7  // UID 最大字节数（ISO14443A 最长 7 字节）

// ---------- 显示屏 ----------
#define TFT_BG_COLOR       0x0000   // 背景色（黑）
#define TFT_TEXT_COLOR     0xFFFF   // 文字色（白）
#define TFT_ACCENT_COLOR   0xF800   // 强调色（红）
#define TFT_ANIM_FPS           30   // 动画帧率

// ---------- SD 卡 ----------
#define SD_MUSIC_DIR     "/music"   // 音乐文件夹
#define SD_COVER_DIR     "/cover"   // 封面图片文件夹
#define SD_TAGS_FILE   "/tags.json" // NFC → 歌曲映射文件

// ---------- 支持的音频格式 ----------
// ESP32-audioI2S 库支持 MP3、AAC、FLAC、WAV、OGG
#define AUDIO_EXT_MP3    ".mp3"
#define AUDIO_EXT_WAV    ".wav"
#define AUDIO_EXT_FLAC   ".flac"

// ---------- 编码器 ----------
#define ENCODER_STEPS_PER_DETENT  4  // 每格步数
