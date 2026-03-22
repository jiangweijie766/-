# Arduino IDE 手把手输入代码（完整源码）

> 适合第一次上手 Arduino IDE 的新手：你可以跟着下面的步骤，**一行不差地把全部代码输入到 IDE 里**。  
> 注意：所有文件必须在同一个文件夹中（同一工程目录），文件名必须完全一致。

---

## ✅ 第 1 步：新建工程文件夹

1. 打开 Arduino IDE
2. `文件 > 新建`
3. 立刻 `文件 > 另存为...`
4. 文件夹名称建议：`music_box`
5. 这一步会创建一个 `music_box.ino` 文件

---

## ✅ 第 2 步：创建所有需要的文件（标签页）

在 Arduino IDE 顶部标签栏右侧点击 **“+”** → **“New Tab”**，依次创建这些文件（文件名必须完全一致）：

```
pins.h
config.h
display.h
audio_player.h
nfc_reader.h
sd_card.h
tag_map.h
pdm_audio.h
```

> 说明：`pdm_audio.h` 只有在你选择无 DAC 的 PDM 方案时才会用到，但建议也创建它，避免编译找不到文件。

---

## ✅ 第 3 步：逐个文件复制粘贴源码

下面每个小节都是 **完整代码**，请逐个复制到对应文件中。

---

### ✅ 1）pins.h

```cpp
#pragma once

// ============================================================
//  ESP32-S3-DevKitC-1 引脚定义
//  硬件：GC9A01 + PN532 + ZK-502C + SD 卡
//  音频输出：PCM5102A I2S DAC（推荐）或 PDM+RC 滤波（无模块方案）
// ============================================================

// ---------- GC9A01 圆形屏（SPI） ----------
#define PIN_TFT_MOSI  11
#define PIN_TFT_SCLK  12
#define PIN_TFT_CS    10
#define PIN_TFT_DC     9
#define PIN_TFT_RST    8
#define PIN_TFT_BL    46   // 背光控制（HIGH = 开启）
// 注：部分 6Pin GC9A01 模块仅引出 VCC/GND/SCL/SDA/DC/RST，背光已接 VCC。
//     · CS 若只留在板背焊盘，请飞线到 PIN_TFT_CS（与 SD 共总线必须有独立 CS）。
//     · 若模块无 BL 引脚，可将 PIN_TFT_BL 悬空（屏幕默认常亮）。

// ---------- SD 卡模块（SPI，与 TFT 共享总线） ----------
#define PIN_SD_MOSI   11   // 共享
#define PIN_SD_MISO   13
#define PIN_SD_SCLK   12   // 共享
#define PIN_SD_CS      1

// ---------- PN532 NFC 模块（I2C） ----------
#define PIN_NFC_SDA    5
#define PIN_NFC_SCL    6

// ---------- PCM5102A I2S DAC（AUDIO_MODE_I2S_DAC 方案）----------
#define PIN_I2S_BCK   15   // 位时钟 (BCLK)
#define PIN_I2S_LRCK  16   // 左右声道时钟 (LRCK)
#define PIN_I2S_DATA  17   // 串行数据 (DIN)

// ---------- PDM 音频输出（AUDIO_MODE_PDM 方案，无需 DAC 模块）----------
// GPIO 14 → 2级 RC 低通滤波器 → ZK-502C AUX 输入
// RC 参数：R=1kΩ, C=10nF（2级），截止频率 ≈ 16kHz
#define PIN_PDM_OUT   14

// ---------- 音量旋转编码器（可选） ----------
#define PIN_ENC_A     18
#define PIN_ENC_B     19
#define PIN_ENC_BTN   20

// ---------- 功能按键 ----------
#define PIN_BTN_PREV  21   // 上一首
#define PIN_BTN_NEXT  47   // 下一首
#define PIN_BTN_PLAY  48   // 播放/暂停（长按 BT_LONGPRESS_MS → 切换蓝牙模式）

// ---------- 状态 LED（可选，WS2812B 或普通 LED） ----------
#define PIN_STATUS_LED 38
```

---

### ✅ 2）config.h

```cpp
#pragma once

// ============================================================
//  全局配置
// ============================================================

// ---------- 音频输出模式 ----------
// ESP32-S3 没有内置 DAC，需要选择一种方案将数字信号转为模拟信号输入 ZK-502C：
//
//   AUDIO_MODE_I2S_DAC (推荐)：外接 PCM5102A I2S DAC 模块
//     优点：音质最佳（SNR >100dB）、接线简单
//     需要：PCM5102A 模块（约 ¥15–25）
//
//   AUDIO_MODE_PDM (无需模块)：使用 ESP32-S3 I2S PDM-TX 输出 + RC 低通滤波
//     优点：无需额外模块，仅需 2 个电阻 + 2 个电容（约 ¥0.5）
//     缺点：输出为单声道，底噪略高于 I2S DAC 方案
//     接法：参见 README.md 中的"无 DAC 模块方案（PDM）"章节
//
#define AUDIO_MODE_I2S_DAC  0
#define AUDIO_MODE_PDM      1
#ifndef AUDIO_MODE
  #define AUDIO_MODE  AUDIO_MODE_I2S_DAC   // 修改此行切换方案
#endif

// ---------- 音频 ----------
#define AUDIO_VOLUME_DEFAULT  70    // 默认音量 (0–100)
#define AUDIO_SAMPLE_RATE  44100    // 采样率 (Hz)

// ---------- NFC ----------
#define NFC_POLL_INTERVAL_MS  300UL  // NFC 轮询间隔（ms）
#define NFC_DEBOUNCE_MS      1500UL  // 同一标签再次触发的冷却时间（ms）
#define NFC_UID_MAX_LEN          7   // UID 最大字节数（ISO14443A 最长 7 字节）

// ---------- 显示屏 ----------
#define TFT_BG_COLOR       0x0000   // 背景色（黑）
#define TFT_TEXT_COLOR     0xFFFF   // 文字色（白）
#define TFT_ACCENT_COLOR   0xF800   // 强调色（红）

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

// ---------- JSON 文档 ----------
#define JSON_DOC_SIZE_TAGS  4096     // tags.json 解析缓冲（字节）；若标签超过 ~50 条请调大

// ---------- 蓝牙模式 ----------
// ESP32-S3 无内置经典蓝牙；蓝牙音频由 ZK-502C 板载 BT 5.0 接收器处理。
// 固件负责切换 UI 状态并停止 I2S 输出，用户需手动将 ZK-502C 拨至 BT 挡。
#define BT_LONGPRESS_MS       1500UL  // 长按播放键时间（ms）切换蓝牙模式

// ---------- 黑胶唱片动画 ----------
#define VINYL_CX               120   // 唱片圆心 X
#define VINYL_CY               120   // 唱片圆心 Y
#define VINYL_DISC_R           108   // 唱片外径（px）
#define VINYL_LABEL_R           50   // 中心标签（专辑图）半径（px）
#define VINYL_HOLE_R             5   // 中心孔半径（px）
#define VINYL_DEG_PER_FRAME      6   // 每帧旋转角度（6° @ 15fps ≈ 45RPM）
#define TFT_ANIM_FPS            15   // 动画帧率（覆盖原 30fps，留足 CPU 时间旋转像素）

// ---------- 音量浮层 ----------
#define VOL_OVERLAY_MS        2000UL  // 调节音量后浮层显示时间 (ms)
```

> 注：`TFT_ANIM_FPS` 注释里提到的“覆盖原 30fps”是指早期版本的默认值，保持为 15fps 更稳。

---

### ✅ 3）music_box.ino

```cpp
/*
 * ESP32-S3 NFC 音乐盒 - 主程序
 *
 * 硬件：
 *   - ESP32-S3-DevKitC-1
 *   - GC9A01  1.28" 240×240 圆形 TFT 屏（SPI）
 *   - PN532   NFC 读卡模块（I2C）
 *   - ZK-502C 功放（TPA3116D2，板载蓝牙 5.0 + AUX 模拟输入）
 *   - PCM5102A I2S DAC（ESP32 I2S → ZK-502C AUX）
 *   - SD 卡读写模块（SPI）
 *   - 德力普 12V 10800mAh 锂电池 + 12V→5V 降压模块
 *   - 高音喇叭 4Ω 11W × 2，全频喇叭 4Ω 10W × 2
 *   - 外接 12V 电量指示模块（直接并联在电池正负极，无需接 ESP32）
 *
 * 依赖库（Arduino IDE 库管理器安装）：
 *   - TFT_eSPI              (GC9A01 显示)
 *   - Adafruit PN532        (NFC)
 *   - ESP8266Audio          (音频播放)
 *   - ArduinoJson           (JSON 解析)
 *   - SD (内置)
 *
 * 功能：
 *   · NFC 刷卡选歌
 *   · SD 卡顺序播放（上/下一首，播放/暂停）
 *   · 旋转编码器调节音量（屏幕浮层显示）
 *   · 黑胶唱片旋转动画 + 专辑封面（BMP）
 *   · 蓝牙模式（长按播放键切换；音频由 ZK-502C 板载 BT 接收）
 */

#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "sd_card.h"
#include "display.h"
#include "audio_player.h"
#include "nfc_reader.h"
#include "tag_map.h"

// ——— 状态机 ———
enum State {
    STATE_IDLE,      // 待机（等待 NFC 或按键）
    STATE_PLAYING,   // 播放中
    STATE_PAUSED,    // 已暂停
    STATE_BT_MODE    // 蓝牙模式（ZK-502C 使用板载 BT，ESP32 停止 I2S）
};
static State g_state = STATE_IDLE;

// ——— 当前歌曲信息 ———
static char g_currentTitle[64]  = "";
static char g_currentArtist[64] = "";
static char g_currentPath[128]  = "";
static char g_currentCover[128] = "";   // /cover/xxx.bmp

// ——— SD 卡音乐列表（上/下一首循环）———
static char g_musicList[128][64];
static int  g_musicCount = 0;
static int  g_musicIndex = 0;

// ——— 黑胶唱片旋转角度 ———
static int           g_vinylAngle    = 0;
static unsigned long g_lastAnimFrame = 0;

// ——— 按键去抖 ———
static unsigned long g_lastBtnPrev = 0;
static unsigned long g_lastBtnNext = 0;
#define BTN_DEBOUNCE_MS 200UL

// ——— 播放键：短按 = 播放/暂停；长按 = 切换蓝牙模式 ———
static unsigned long g_btnPlayDown   = 0;  // 按键按下时刻（0 = 未按下）
static bool          g_btnLongFired  = false;

// ——— NFC 冷却 ———
static unsigned long g_lastNfcTime = 0;
static char          g_lastNfcUid[24] = "";

// ——— 旋转编码器（音量）———
static int g_encLastA = HIGH;

// ——— 前向声明 ———
void on_nfc_detected(const char *uidStr);
void on_btn_playpause();
void on_btn_bt_toggle();
void play_file(const char *path, const char *coverPath = nullptr);
void play_by_index(int index);
void play_next();
void play_prev();
void compute_cover_path(const char *musicFilename, char *out, size_t outLen);

// ============================================================
void setup() {
    Serial.begin(115200);
    Serial.println("\n[Boot] ESP32-S3 音乐盒启动");

    // 按键引脚
    pinMode(PIN_BTN_PLAY, INPUT_PULLUP);
    pinMode(PIN_BTN_PREV, INPUT_PULLUP);
    pinMode(PIN_BTN_NEXT, INPUT_PULLUP);

    // 编码器引脚
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    pinMode(PIN_ENC_BTN, INPUT_PULLUP);

    // 初始化显示屏
    display_init();
    display_idle();

    // 初始化 SD 卡
    if (!sdcard_init()) {
        display_error("SD卡失败");
        while (1) delay(1000);
    }

    // 加载 NFC 标签映射
    if (!tags_load()) {
        Serial.println("[Boot] tags.json 加载失败，仅支持文件名模式");
    }

    // 扫描 SD 卡音乐列表（用于上/下曲切换）
    g_musicCount = sdcard_list_dir(SD_MUSIC_DIR, g_musicList, 128);
    Serial.printf("[Boot] 发现 %d 首音乐\n", g_musicCount);

    // 初始化音频输出（I2S → PCM5102A → ZK-502C AUX）
    audio_init();
    audio_set_volume(AUDIO_VOLUME_DEFAULT);

    // 初始化 NFC
    if (!nfc_init()) {
        display_error("NFC失败");
        // NFC 失败不阻止启动，仍可通过按键播放 SD 列表
        delay(1500);
        display_idle();
    }

    Serial.println("[Boot] 初始化完成");
}

// ============================================================
void loop() {
    unsigned long now = millis();

    // —— 音频驱动（必须频繁调用）——
    if (g_state == STATE_PLAYING) {
        bool stillPlaying = audio_loop();
        if (!stillPlaying) {
            Serial.println("[Loop] 播放结束，自动下一首");
            play_next();
        }
    }

    // —— NFC 轮询（蓝牙模式下跳过）——
    if (g_state != STATE_BT_MODE &&
        now - g_lastNfcTime > NFC_POLL_INTERVAL_MS) {
        char uidStr[24];
        if (nfc_poll(uidStr, sizeof(uidStr))) {
            bool sameCard = (strcmp(uidStr, g_lastNfcUid) == 0);
            bool cooled   = (now - g_lastNfcTime > NFC_DEBOUNCE_MS);
            if (!sameCard || cooled) {
                strncpy(g_lastNfcUid, uidStr, sizeof(g_lastNfcUid) - 1);
                g_lastNfcTime = now;
                on_nfc_detected(uidStr);
            }
        }
    }

    // —— 播放键（短按=播放/暂停，长按=切换蓝牙模式）——
    bool btnPlayNow = (digitalRead(PIN_BTN_PLAY) == LOW);
    if (btnPlayNow && g_btnPlayDown == 0) {
        // 按下
        g_btnPlayDown  = now;
        g_btnLongFired = false;
    } else if (!btnPlayNow && g_btnPlayDown > 0) {
        // 释放：若未触发长按则视为短按
        if (!g_btnLongFired) {
            on_btn_playpause();
        }
        g_btnPlayDown = 0;
    } else if (btnPlayNow && !g_btnLongFired &&
               g_btnPlayDown > 0 && (now - g_btnPlayDown >= BT_LONGPRESS_MS)) {
        // 长按达到阈值
        g_btnLongFired = true;
        on_btn_bt_toggle();
    }

    // —— 上一首按键 ——
    if (digitalRead(PIN_BTN_PREV) == LOW &&
        now - g_lastBtnPrev > BTN_DEBOUNCE_MS &&
        g_state != STATE_BT_MODE) {
        g_lastBtnPrev = now;
        play_prev();
    }

    // —— 下一首按键 ——
    if (digitalRead(PIN_BTN_NEXT) == LOW &&
        now - g_lastBtnNext > BTN_DEBOUNCE_MS &&
        g_state != STATE_BT_MODE) {
        g_lastBtnNext = now;
        play_next();
    }

    // —— 旋转编码器（音量）——
    int encA = digitalRead(PIN_ENC_A);
    if (encA != g_encLastA && encA == LOW) {
        int encB = digitalRead(PIN_ENC_B);
        if (encB == HIGH) {
            audio_volume_up();
        } else {
            audio_volume_down();
        }
        // 触发音量浮层（播放中或暂停中均显示）
        if (g_state == STATE_PLAYING || g_state == STATE_PAUSED) {
            display_trigger_volume_overlay(audio_get_volume());
        }
    }
    g_encLastA = encA;

    // —— 黑胶动画帧（播放或暂停时显示静止唱片）——
    if ((g_state == STATE_PLAYING || g_state == STATE_PAUSED) &&
        now - g_lastAnimFrame > (1000UL / TFT_ANIM_FPS)) {
        g_lastAnimFrame = now;
        if (g_state == STATE_PLAYING) {
            g_vinylAngle = (g_vinylAngle + VINYL_DEG_PER_FRAME) % 360;
        }
        display_vinyl_frame(g_vinylAngle, g_state == STATE_PAUSED);
    }
}

// ============================================================
//  事件处理
// ============================================================

// NFC 刷卡
void on_nfc_detected(const char *uidStr) {
    Serial.printf("[NFC] 检测到标签: %s\n", uidStr);
    display_nfc_detected();

    const SongInfo *song = tags_find(uidStr);
    if (song) {
        char musicPath[128];
        if (sdcard_find_music(song->id, musicPath, sizeof(musicPath))) {
            strncpy(g_currentTitle,  song->title,  sizeof(g_currentTitle) - 1);
            strncpy(g_currentArtist, song->artist, sizeof(g_currentArtist) - 1);
            // 封面路径
            char cover[128];
            snprintf(cover, sizeof(cover), "%s/%s.bmp", SD_COVER_DIR, song->id);
            play_file(musicPath, cover);
        } else {
            Serial.printf("[NFC] 未找到歌曲文件: id=%s\n", song->id);
            display_error("文件未找到");
            delay(1500);
            display_idle();
        }
    } else {
        Serial.printf("[NFC] UID 未映射: %s\n", uidStr);
        display_error("标签未注册");
        delay(1500);
        display_idle();
    }
}

// 短按：播放 / 暂停 / 恢复
void on_btn_playpause() {
    if (g_state == STATE_PLAYING) {
        // → 暂停
        audio_pause();
        g_state = STATE_PAUSED;
        Serial.println("[Btn] 已暂停");
        display_vinyl_frame(g_vinylAngle, true);

    } else if (g_state == STATE_PAUSED && g_currentPath[0] != '\0') {
        // → 断点续播
        if (audio_resume(g_currentPath)) {
            g_state = STATE_PLAYING;
            Serial.println("[Btn] 续播");
        }

    } else if (g_state == STATE_IDLE && g_musicCount > 0) {
        // → 从 SD 列表第一首开始
        play_by_index(0);
    }
}

// 长按：切换蓝牙模式
void on_btn_bt_toggle() {
    if (g_state == STATE_BT_MODE) {
        // 退出蓝牙模式
        Serial.println("[BT] 退出蓝牙模式");
        g_state = STATE_IDLE;
        display_idle();
    } else {
        // 进入蓝牙模式：停止 SD 播放
        if (g_state == STATE_PLAYING || g_state == STATE_PAUSED) {
            audio_stop();
            display_vinyl_free();
        }
        g_state = STATE_BT_MODE;
        Serial.println("[BT] 进入蓝牙模式 — ZK-502C 请拨至 BT 挡");
        display_bluetooth();
    }
}

// 播放指定文件（可指定封面路径）
void play_file(const char *path, const char *coverPath) {
    strncpy(g_currentPath, path, sizeof(g_currentPath) - 1);
    if (coverPath) {
        strncpy(g_currentCover, coverPath, sizeof(g_currentCover) - 1);
    } else {
        g_currentCover[0] = '\0';
    }

    display_vinyl_setup(g_currentTitle, g_currentArtist,
                        (g_currentCover[0] ? g_currentCover : nullptr));

    if (audio_play(path)) {
        g_state      = STATE_PLAYING;
        g_vinylAngle = 0;
        g_lastAnimFrame = millis();

        // 同步 g_musicIndex 到当前文件（便于上/下曲）
        const char *fn = strrchr(path, '/');
        if (fn) {
            fn++;
            for (int i = 0; i < g_musicCount; i++) {
                if (strcmp(g_musicList[i], fn) == 0) {
                    g_musicIndex = i;
                    break;
                }
            }
        }
    } else {
        g_state = STATE_IDLE;
        display_vinyl_free();
        display_error("播放失败");
        delay(1500);
        display_idle();
    }
}

// 按列表下标播放（环形）
void play_by_index(int index) {
    if (g_musicCount == 0) return;
    g_musicIndex = ((index % g_musicCount) + g_musicCount) % g_musicCount;

    char path[128];
    snprintf(path, sizeof(path), "%s/%s", SD_MUSIC_DIR, g_musicList[g_musicIndex]);

    // 从文件名推导标题（去扩展名）
    char title[64];
    strncpy(title, g_musicList[g_musicIndex], sizeof(title) - 1);
    char *dot = strrchr(title, '.');
    if (dot) *dot = '\0';
    strncpy(g_currentTitle,  title, sizeof(g_currentTitle) - 1);
    strncpy(g_currentArtist, "",    sizeof(g_currentArtist) - 1);

    // 从文件名推导封面路径
    char cover[128];
    compute_cover_path(g_musicList[g_musicIndex], cover, sizeof(cover));

    play_file(path, cover);
}

// 下一首
void play_next() { play_by_index(g_musicIndex + 1); }
// 上一首
void play_prev() { play_by_index(g_musicIndex - 1); }

// 根据音乐文件名推导封面路径（"/cover/<名称无扩展名>.bmp"）
void compute_cover_path(const char *musicFilename, char *out, size_t outLen) {
    char id[64] = {0};
    strncpy(id, musicFilename, sizeof(id) - 1);
    char *dot = strrchr(id, '.');
    if (dot) *dot = '\0';
    snprintf(out, outLen, "%s/%s.bmp", SD_COVER_DIR, id);
}
```

---

### ✅ 4）display.h

```cpp
#pragma once

#include <TFT_eSPI.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  GC9A01 圆形屏显示模块（240×240，圆形显示区）
//
//  架构：用一个 240×240 的 TFT_eSprite 作全屏帧缓冲，
//        每帧统一绘制后一次性 pushSprite，消除撕裂与闪烁。
//
//  功能：
//    · display_idle()               待机界面（NFC 刷卡提示）
//    · display_error()              错误信息
//    · display_nfc_detected()       NFC 感应反馈
//    · display_vinyl_setup()        播放界面初始化（加载专辑图）
//    · display_vinyl_frame()        黑胶唱片旋转帧（主循环调用）
//    · display_vinyl_free()         释放专辑图内存
//    · display_bluetooth()          蓝牙模式界面
// ============================================================

static TFT_eSPI tft = TFT_eSPI();

// ——— 帧缓冲精灵（240×240，分配于 PSRAM）———
static TFT_eSprite g_spr(&tft);
static bool        g_sprCreated = false;

// ——— 专辑封面像素缓冲（BMP 解码后 RGB565，PSRAM）———
static uint16_t *g_coverBuf = nullptr;
static int       g_coverW   = 0;
static int       g_coverH   = 0;

// ——— 当前播放歌曲信息（由 display_vinyl_setup 设置）———
static char g_vTitle[64]  = "";
static char g_vArtist[64] = "";

// ——— 音量浮层 ———
static bool          g_volOverlay      = false;
static unsigned long g_volOverlayStart = 0;
static int           g_volOverlayVal   = 0;

// ============================================================
//  内部辅助：确保帧缓冲精灵已创建
// ============================================================
static void _spr_ensure() {
    if (!g_sprCreated) {
        g_spr.setColorDepth(16);
        g_spr.createSprite(240, 240);   // 自动使用 PSRAM（若可用）
        g_sprCreated = true;
    }
}

// ——— 无封面时占位色块的默认色种子（ASCII 'B'，确保无标题时仍生成有颜色的扇区）———
#define COVER_DEFAULT_COLOR_SEED  0x42   // 'B' for default

// ============================================================
//  内部辅助：在精灵上绘制旋转后的专辑封面（或占位图案）
//  cx,cy = 标签圆心；radius = 标签半径；angleDeg = 旋转角度
// ============================================================
static void _draw_cover_rotated(TFT_eSprite &spr, int cx, int cy,
                                int radius, float angleDeg) {
    float rad  = angleDeg * (float)M_PI / 180.0f;
    float sinA = sinf(rad);
    float cosA = cosf(rad);
    int   r2   = radius * radius;

    if (g_coverBuf && g_coverW > 0 && g_coverH > 0) {
        // ——— 有专辑图：做像素级反向旋转采样 ———
        float pcx = g_coverW * 0.5f - 0.5f;
        float pcy = g_coverH * 0.5f - 0.5f;

        for (int sy = cy - radius; sy <= cy + radius; sy++) {
            int dy   = sy - cy;
            int maxDx = (int)sqrtf((float)(r2 - dy * dy));
            for (int sx = cx - maxDx; sx <= cx + maxDx; sx++) {
                int dx = sx - cx;
                // 反向旋转找到源像素坐标
                float srcX = cosA * dx + sinA * dy + pcx;
                float srcY = -sinA * dx + cosA * dy + pcy;
                int px = (int)(srcX + 0.5f);
                int py = (int)(srcY + 0.5f);
                uint16_t color;
                if (px >= 0 && px < g_coverW && py >= 0 && py < g_coverH) {
                    color = g_coverBuf[py * g_coverW + px];
                } else {
                    color = 0x2104; // 越界用深色填充
                }
                spr.drawPixel(sx, sy, color);
            }
        }
    } else {
        // ——— 无专辑图：绘制旋转扇形占位图案 ———
        // 用标题字符推导色相（两种主色交替扇区）
        uint8_t  seed = g_vTitle[0] ? (uint8_t)g_vTitle[0] : COVER_DEFAULT_COLOR_SEED;
        uint16_t col1 = (uint16_t)((seed * 0x421) & 0xFFFF) | 0x0800;  // 保证有颜色
        uint16_t col2 = col1 ^ 0x780F;

        for (int sy = cy - radius; sy <= cy + radius; sy++) {
            int dy   = sy - cy;
            int maxDx = (int)sqrtf((float)(r2 - dy * dy));
            for (int sx = cx - maxDx; sx <= cx + maxDx; sx++) {
                int dx = sx - cx;
                // 旋转后的角度（用于扇区颜色选择）
                float a = atan2f((float)dy, (float)dx) + rad;
                // 将角度量化为 6 个扇区
                int sector = (int)(a * (float)(3.0 / M_PI)) & 5;
                spr.drawPixel(sx, sy, (sector & 1) ? col1 : col2);
            }
        }

        // 在占位圆中心绘制音符（叠加在扇区上）
        spr.setTextDatum(MC_DATUM);
        spr.setTextColor(TFT_WHITE, col1);
        spr.setTextSize(3);
        spr.drawString("♪", cx, cy);
    }
}

// ============================================================
//  内部辅助：在精灵上绘制唱片主体（唱片盘 + 纹路 + 针臂）
// ============================================================
static void _draw_vinyl_disc(TFT_eSprite &spr, int angle) {
    const int cx = VINYL_CX, cy = VINYL_CY;

    // —— 1. 唱片主体（深灰圆）——
    spr.fillCircle(cx, cy, VINYL_DISC_R, 0x0841);

    // —— 2. 唱片纹路（同心圆，由内而外逐渐加深）——
    for (int r = VINYL_LABEL_R + 6; r <= VINYL_DISC_R - 4; r += 4) {
        uint16_t gColor = (r % 8 == 0) ? 0x2104 : 0x18E3;
        spr.drawCircle(cx, cy, r, gColor);
    }

    // —— 3. 旋转光晕（8 条放射线随 angle 旋转，模拟唱片自转）——
    for (int i = 0; i < 8; i++) {
        float a = ((angle + i * 45) % 360) * (float)M_PI / 180.0f;
        // 亮光线（2 条）vs 暗辐射线（6 条）
        uint16_t lineCol = (i < 2) ? 0x4A69 : 0x18E3;
        int r1 = VINYL_LABEL_R + 8, r2 = VINYL_DISC_R - 6;
        int x1 = cx + (int)(r1 * cosf(a));
        int y1 = cy + (int)(r1 * sinf(a));
        int x2 = cx + (int)(r2 * cosf(a));
        int y2 = cy + (int)(r2 * sinf(a));
        spr.drawLine(x1, y1, x2, y2, lineCol);
    }

    // —— 4. 标签外环（深色描边）——
    spr.drawCircle(cx, cy, VINYL_LABEL_R + 2, 0x4208);
    spr.drawCircle(cx, cy, VINYL_LABEL_R + 1, 0x2104);

    // —— 5. 旋转专辑封面 ——
    _draw_cover_rotated(spr, cx, cy, VINYL_LABEL_R, (float)angle);

    // —— 6. 中心孔 ——
    spr.fillCircle(cx, cy, VINYL_HOLE_R, TFT_BLACK);
    spr.drawCircle(cx, cy, VINYL_HOLE_R, 0x8430);

    // —— 7. 唱针臂（从右上角斜伸到唱片边缘）——
    // 臂主体：枢轴(207,16) → 接触点(162,70)
    spr.drawLine(207, 16, 164, 72, 0x8430);
    spr.drawLine(206, 17, 163, 73, 0x9CB2); // 高光
    // 针头小线段
    spr.drawLine(164, 72, 158, 82, 0xC618);
    // 枢轴圆点
    spr.fillCircle(207, 16, 4, 0xCE79);
    spr.drawCircle(207, 16, 4, TFT_WHITE);
    // 针尖红点
    spr.fillCircle(158, 82, 3, 0xF800);
}

// ============================================================
//  内部辅助：在精灵底部绘制歌曲信息条（标题 + 歌手 + 进度状态）
// ============================================================
static void _draw_song_info(TFT_eSprite &spr, bool paused) {
    // 底部信息区（在圆形屏可见范围内：y=172–218）
    spr.setTextDatum(MC_DATUM);

    // 标题（白色，较大）
    char shortTitle[20];
    strncpy(shortTitle, g_vTitle, 19);
    shortTitle[19] = '\0';
    spr.setTextColor(TFT_WHITE, TFT_BLACK);
    spr.setTextSize(2);
    spr.drawString(shortTitle, 120, 176);

    // 歌手（浅灰，较小）
    char shortArtist[24];
    strncpy(shortArtist, g_vArtist, 23);
    shortArtist[23] = '\0';
    spr.setTextColor(0xAD55, TFT_BLACK);
    spr.setTextSize(1);
    spr.drawString(shortArtist, 120, 193);

    // 播放状态图标
    spr.setTextColor(paused ? 0xFFE0 : 0x07E0, TFT_BLACK);
    spr.drawString(paused ? "II" : ">", 120, 208);
}

// ============================================================
//  内部辅助：音量浮层（中心大字+进度条，覆盖在唱片上方）
// ============================================================
static void _draw_volume_overlay(TFT_eSprite &spr, int vol) {
    // 半透明感：画一个深色圆角矩形
    spr.fillRoundRect(55, 82, 130, 76, 10, 0x1082);
    spr.drawRoundRect(55, 82, 130, 76, 10, 0x4208);

    spr.setTextDatum(MC_DATUM);
    spr.setTextColor(0xAD55, 0x1082);
    spr.setTextSize(1);
    spr.drawString("VOL", 120, 96);

    // 大号百分比
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", vol);
    spr.setTextColor(TFT_WHITE, 0x1082);
    spr.setTextSize(4);
    spr.drawString(buf, 120, 120);

    // 进度条
    int barX = 68, barY = 145, barW = 104, barH = 8;
    spr.drawRect(barX, barY, barW, barH, 0x4208);
    int fill = (int)(vol * (barW - 4) / 100);
    if (fill > 0)
        spr.fillRect(barX + 2, barY + 2, fill, barH - 4, TFT_ACCENT_COLOR);
}

// ============================================================
//  公共 API：初始化显示屏
// ============================================================
void display_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BG_COLOR);
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
    _spr_ensure();
}

// ============================================================
//  公共 API：待机界面（NFC 刷卡提示）
// ============================================================
void display_idle() {
    _spr_ensure();
    g_spr.fillSprite(TFT_BG_COLOR);

    // 大圆装饰
    g_spr.drawCircle(120, 120, 100, 0x2104);
    g_spr.drawCircle(120, 120,  96, 0x18E3);

    // NFC 波纹（三层圆弧示意）
    g_spr.drawCircle(120, 108, 20, 0x4208);
    g_spr.drawCircle(120, 108, 32, 0x2104);
    g_spr.drawCircle(120, 108, 44, 0x18E3);

    g_spr.setTextDatum(MC_DATUM);
    g_spr.setTextColor(TFT_TEXT_COLOR, TFT_BG_COLOR);
    g_spr.setTextSize(2);
    g_spr.drawString("NFC Music Box", 120, 150);

    g_spr.setTextSize(1);
    g_spr.setTextColor(0x7BEF, TFT_BG_COLOR);
    g_spr.drawString("请刷 NFC 标签选歌", 120, 170);
    g_spr.drawString("或按 ▶ 播放 SD 列表", 120, 184);

    g_spr.pushSprite(0, 0);
}

// ============================================================
//  公共 API：错误信息
// ============================================================
void display_error(const char *msg) {
    _spr_ensure();
    g_spr.fillSprite(TFT_BG_COLOR);
    g_spr.setTextDatum(MC_DATUM);
    g_spr.setTextColor(TFT_ACCENT_COLOR, TFT_BG_COLOR);
    g_spr.setTextSize(2);
    g_spr.drawString("Error", 120, 100);
    g_spr.setTextSize(1);
    g_spr.setTextColor(TFT_TEXT_COLOR, TFT_BG_COLOR);
    g_spr.drawString(msg, 120, 130);
    g_spr.pushSprite(0, 0);
}

// ============================================================
//  公共 API：NFC 刷卡反馈（短闪）
// ============================================================
void display_nfc_detected() {
    tft.fillCircle(120, 120, 40, TFT_ACCENT_COLOR);
    tft.setTextColor(TFT_WHITE, TFT_ACCENT_COLOR);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("识别中...", 120, 120);
    delay(300);
}

// ============================================================
//  公共 API：初始化播放界面
//  title      = 歌曲名
//  artist     = 歌手名
//  coverPath  = SD 卡 BMP 路径（nullptr 或不存在则显示占位图案）
// ============================================================
void display_vinyl_setup(const char *title, const char *artist,
                         const char *coverPath) {
    strncpy(g_vTitle,  title,  sizeof(g_vTitle) - 1);
    g_vTitle[sizeof(g_vTitle) - 1] = '\0';
    strncpy(g_vArtist, artist, sizeof(g_vArtist) - 1);
    g_vArtist[sizeof(g_vArtist) - 1] = '\0';

    // 释放旧专辑图缓冲
    if (g_coverBuf) { free(g_coverBuf); g_coverBuf = nullptr; }
    g_coverW = g_coverH = 0;

    // 加载专辑封面（期望 100×100 BMP，自动裁剪到 100×100）
    if (coverPath && coverPath[0] != '\0') {
        g_coverBuf = sdcard_load_bmp(coverPath, &g_coverW, &g_coverH, 100, 100);
    }

    _spr_ensure();
}

// ============================================================
//  公共 API：黑胶唱片旋转帧（主循环每帧调用）
//  angle    = 当前旋转角度 (0–359)
//  paused   = 是否暂停
// ============================================================
void display_vinyl_frame(int angle, bool paused) {
    _spr_ensure();
    g_spr.fillSprite(TFT_BLACK);

    // 绘制唱片（含封面旋转）
    _draw_vinyl_disc(g_spr, angle);

    // 歌曲信息（底部）
    _draw_song_info(g_spr, paused);

    // 音量浮层（如果激活）
    unsigned long now = millis();
    if (g_volOverlay) {
        if (now - g_volOverlayStart < (unsigned long)VOL_OVERLAY_MS) {
            _draw_volume_overlay(g_spr, g_volOverlayVal);
        } else {
            g_volOverlay = false;
        }
    }

    g_spr.pushSprite(0, 0);
}

// ============================================================
//  公共 API：释放播放界面资源（停止播放时调用）
// ============================================================
void display_vinyl_free() {
    if (g_coverBuf) { free(g_coverBuf); g_coverBuf = nullptr; }
    g_coverW = g_coverH = 0;
}

// ============================================================
//  公共 API：触发音量浮层（调节音量时调用）
// ============================================================
void display_trigger_volume_overlay(int vol) {
    g_volOverlay      = true;
    g_volOverlayStart = millis();
    g_volOverlayVal   = vol;
}

// ============================================================
//  公共 API：蓝牙模式界面
// ============================================================
void display_bluetooth() {
    _spr_ensure();
    g_spr.fillSprite(TFT_BLACK);

    // 背景同心圆
    g_spr.drawCircle(120, 120, 108, 0x001F);
    g_spr.drawCircle(120, 120, 104, 0x0010);

    // 蓝牙符号（手工绘制，中心 120,100，高度 60）
    int bx = 120, by = 100, bh = 30;
    // 竖线
    g_spr.drawLine(bx, by - bh, bx, by + bh, 0x03EF);
    g_spr.drawLine(bx + 1, by - bh, bx + 1, by + bh, 0x03EF);
    // 上半钻石
    g_spr.drawLine(bx, by - bh, bx + bh * 2 / 3, by - bh / 3, 0x03EF);
    g_spr.drawLine(bx + bh * 2 / 3, by - bh / 3, bx, by, 0x03EF);
    // 下半钻石
    g_spr.drawLine(bx, by, bx + bh * 2 / 3, by + bh / 3, 0x03EF);
    g_spr.drawLine(bx + bh * 2 / 3, by + bh / 3, bx, by + bh, 0x03EF);

    // 文字
    g_spr.setTextDatum(MC_DATUM);
    g_spr.setTextColor(0x03EF, TFT_BLACK);
    g_spr.setTextSize(2);
    g_spr.drawString("蓝牙模式", 120, 155);

    g_spr.setTextSize(1);
    g_spr.setTextColor(0xAD55, TFT_BLACK);
    g_spr.drawString("手机搜索连接 ZK-502C", 120, 176);
    g_spr.drawString("将功放拨至 BT 挡", 120, 192);

    g_spr.setTextColor(0x7BEF, TFT_BLACK);
    g_spr.drawString("长按 ▶ 退出蓝牙模式", 120, 212);

    g_spr.pushSprite(0, 0);
}
```

> 注：`COVER_DEFAULT_COLOR_SEED` 用 `'B'` 作为默认种子，是为了在标题为空时仍能生成明显的占位色块。

---

### ✅ 5）audio_player.h

```cpp
#pragma once

#include <Arduino.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioGeneratorFLAC.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  音频播放模块
//
//  根据 config.h 中的 AUDIO_MODE 选择输出后端：
//    AUDIO_MODE_I2S_DAC（默认）：AudioOutputI2S → PCM5102A → ZK-502C
//    AUDIO_MODE_PDM：           AudioOutputPDM  → RC滤波  → ZK-502C
// ============================================================

#if AUDIO_MODE == AUDIO_MODE_PDM
  #include "pdm_audio.h"
  static AudioOutputPDM *g_audioOut = nullptr;
#else
  #include <AudioOutputI2S.h>
  static AudioOutputI2S *g_audioOut = nullptr;
#endif

static AudioFileSourceSD  *g_audioSource = nullptr;
static AudioGeneratorMP3  *g_mp3         = nullptr;
static AudioGeneratorWAV  *g_wav         = nullptr;
static AudioGeneratorFLAC *g_flac        = nullptr;
static int    g_volume   = AUDIO_VOLUME_DEFAULT;
static size_t g_pausePos = 0;   // 暂停时保存的文件字节位置，用于断点续播

// 当前使用的解码器类型
enum AudioType { AUDIO_NONE, AUDIO_MP3, AUDIO_WAV, AUDIO_FLAC };
static AudioType g_audioType = AUDIO_NONE;

// 初始化音频输出
void audio_init() {
#if AUDIO_MODE == AUDIO_MODE_PDM
    g_audioOut = new AudioOutputPDM();
    g_audioOut->begin();
    g_audioOut->SetGain((float)g_volume / 100.0f);
    Serial.println("[Audio] 模式：PDM（无 DAC 模块，单声道）");
#else
    g_audioOut = new AudioOutputI2S();
    g_audioOut->SetPinout(PIN_I2S_BCK, PIN_I2S_LRCK, PIN_I2S_DATA);
    g_audioOut->SetOutputModeMono(false); // 立体声
    g_audioOut->SetGain((float)g_volume / 100.0f);
    Serial.println("[Audio] 模式：I2S DAC（PCM5102A，立体声）");
#endif
}

// 停止当前播放（重置位置）
void audio_stop() {
    g_pausePos = 0;
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

// 根据文件扩展名启动播放（可从指定字节偏移处开始，0 = 从头）
bool audio_play(const char *filePath, size_t startPos = 0) {
    audio_stop();

    g_audioSource = new AudioFileSourceSD(filePath);
    if (!g_audioSource) {
        Serial.println("[Audio] 无法创建音频源");
        return false;
    }

    // 断点续播：seek 到暂停时的文件位置
    if (startPos > 0) {
        g_audioSource->seek((int32_t)startPos, SEEK_SET);
    }

    // 根据扩展名选择解码器
    const char *ext = strrchr(filePath, '.');
    if (ext && strcasecmp(ext, ".mp3") == 0) {
        g_mp3 = new AudioGeneratorMP3();
        if (!g_mp3->begin(g_audioSource, g_audioOut)) {
            Serial.println("[Audio] MP3 启动失败");
            audio_stop();
            return false;
        }
        g_audioType = AUDIO_MP3;
    } else if (ext && strcasecmp(ext, ".wav") == 0) {
        g_wav = new AudioGeneratorWAV();
        if (!g_wav->begin(g_audioSource, g_audioOut)) {
            Serial.println("[Audio] WAV 启动失败");
            audio_stop();
            return false;
        }
        g_audioType = AUDIO_WAV;
    } else if (ext && strcasecmp(ext, ".flac") == 0) {
        g_flac = new AudioGeneratorFLAC();
        if (!g_flac->begin(g_audioSource, g_audioOut)) {
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
    if (g_audioOut) {
        g_audioOut->SetGain((float)g_volume / 100.0f);
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

// 暂停：保存当前文件位置，停止解码器
// 返回暂停位置（字节），用于 audio_resume()
size_t audio_pause() {
    if (g_audioSource) {
        int32_t pos = g_audioSource->getPos();
        g_pausePos = (pos >= 0) ? (size_t)pos : 0;
    }
    // 停止解码但不重置 g_pausePos（audio_stop 会重置）
    if (g_mp3  && g_mp3->isRunning())  g_mp3->stop();
    if (g_wav  && g_wav->isRunning())  g_wav->stop();
    if (g_flac && g_flac->isRunning()) g_flac->stop();
    if (g_audioSource) { delete g_audioSource; g_audioSource = nullptr; }
    if (g_mp3)   { delete g_mp3;   g_mp3   = nullptr; }
    if (g_wav)   { delete g_wav;   g_wav   = nullptr; }
    if (g_flac)  { delete g_flac;  g_flac  = nullptr; }
    g_audioType = AUDIO_NONE;
    Serial.printf("[Audio] 已暂停，位置: %u 字节\n", (unsigned)g_pausePos);
    return g_pausePos;
}

// 断点续播：从 audio_pause() 保存的位置恢复
bool audio_resume(const char *filePath) {
    bool ok = audio_play(filePath, g_pausePos);
    if (ok) Serial.printf("[Audio] 从 %u 字节续播: %s\n", (unsigned)g_pausePos, filePath);
    return ok;
}
```

---

### ✅ 6）nfc_reader.h

```cpp
#pragma once

#include <Wire.h>
#include <Adafruit_PN532.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  PN532 NFC 读卡模块（I2C 接口）
// ============================================================

static Adafruit_PN532 nfc(PIN_NFC_SDA, PIN_NFC_SCL);

// NFC 模块初始化
bool nfc_init() {
    Wire.begin(PIN_NFC_SDA, PIN_NFC_SCL);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[NFC] 未找到 PN532 模块");
        return false;
    }
    Serial.printf("[NFC] 固件版本: v%d.%d\n",
                  (versiondata >> 16) & 0xFF,
                  (versiondata >> 8) & 0xFF);

    nfc.SAMConfig();
    Serial.println("[NFC] PN532 初始化成功");
    return true;
}

// 将 UID 字节数组转为十六进制字符串，例如 "04:AB:CD:EF"
void nfc_uid_to_str(uint8_t *uid, uint8_t uidLen, char *outStr, size_t outSize) {
    outStr[0] = '\0';
    for (uint8_t i = 0; i < uidLen && i < NFC_UID_MAX_LEN; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", uid[i]);
        strncat(outStr, buf, outSize - strlen(outStr) - 1);
        if (i < uidLen - 1) {
            strncat(outStr, ":", outSize - strlen(outStr) - 1);
        }
    }
}

// 非阻塞轮询：若检测到卡片返回 true，并填充 uidStr
// uidStr 缓冲区大小至少 24 字节
bool nfc_poll(char *uidStr, size_t uidStrSize) {
    uint8_t uid[7];
    uint8_t uidLen;

    bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 50);
    if (found) {
        nfc_uid_to_str(uid, uidLen, uidStr, uidStrSize);
        return true;
    }
    return false;
}
```

---

### ✅ 7）sd_card.h

```cpp
#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  SD 卡模块（SPI 接口）
// ============================================================

// SD 卡初始化
bool sdcard_init() {
    SPI.begin(PIN_SD_SCLK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
    if (!SD.begin(PIN_SD_CS)) {
        Serial.println("[SD] SD 卡初始化失败，检查接线或卡是否插入");
        return false;
    }
    Serial.printf("[SD] SD 卡容量: %llu MB\n", SD.cardSize() / (1024ULL * 1024));
    return true;
}

// 检查文件是否存在
bool sdcard_file_exists(const char *path) {
    return SD.exists(path);
}

// 读取整个文件内容到动态分配的缓冲区（小文件用）
// 调用者必须在使用完毕后调用 free(*outBuf) 释放内存
// 返回字节数，失败返回 -1
int sdcard_read_file(const char *path, char **outBuf) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[SD] 无法打开文件: %s\n", path);
        return -1;
    }
    size_t sz = f.size();
    *outBuf = (char *)malloc(sz + 1);
    if (!*outBuf) {
        f.close();
        return -1;
    }
    f.read((uint8_t *)*outBuf, sz);
    (*outBuf)[sz] = '\0';
    f.close();
    return (int)sz;
}

// 列出目录下所有文件（仅文件名），填充数组，返回文件数量
int sdcard_list_dir(const char *dirPath, char names[][64], int maxCount) {
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory()) return 0;
    int count = 0;
    File entry;
    while ((entry = dir.openNextFile()) && count < maxCount) {
        if (!entry.isDirectory()) {
            strncpy(names[count], entry.name(), 63);
            names[count][63] = '\0';
            count++;
        }
        entry.close();
    }
    dir.close();
    return count;
}

// 根据歌曲 ID 构造 MP3 路径，例如 id="001" → "/music/001.mp3"
// 按优先级尝试 .mp3 / .wav / .flac，将路径写入 outPath
bool sdcard_find_music(const char *id, char *outPath, size_t outSize) {
    const char *exts[] = {AUDIO_EXT_MP3, AUDIO_EXT_WAV, AUDIO_EXT_FLAC};
    for (int i = 0; i < 3; i++) {
        snprintf(outPath, outSize, "%s/%s%s", SD_MUSIC_DIR, id, exts[i]);
        if (SD.exists(outPath)) {
            return true;
        }
    }
    outPath[0] = '\0';
    return false;
}

// ============================================================
//  BMP 文件加载（24-bit BMP → RGB565 像素缓冲）
//
//  返回：动态分配的 uint16_t 数组（RGB565），宽 × 高，行优先
//        调用者负责 free()。失败返回 nullptr。
//  outW/outH：实际读取的像素尺寸（不超过 maxW/maxH）。
//
//  支持：标准 24-bit BMP（RGB888），自动转 RGB565；
//        底朝上（默认 BMP 存储方向）和顶朝上（负高度头）均支持。
//  不支持：RLE 压缩、16-bit 及其他 bit-depth BMP。
// ============================================================
uint16_t *sdcard_load_bmp(const char *path, int *outW, int *outH, int maxW, int maxH) {
    File f = SD.open(path, FILE_READ);
    if (!f) {
        Serial.printf("[BMP] 无法打开: %s\n", path);
        return nullptr;
    }

    // — 读取文件头（14 字节）+ DIB 头（40 字节）—
    uint8_t hdr[54];
    if (f.read(hdr, 54) != 54 || hdr[0] != 'B' || hdr[1] != 'M') {
        Serial.println("[BMP] 非有效 BMP 文件");
        f.close();
        return nullptr;
    }

    uint32_t pixOffset = hdr[10] | ((uint32_t)hdr[11] << 8) |
                         ((uint32_t)hdr[12] << 16) | ((uint32_t)hdr[13] << 24);
    int32_t  bmpW     = hdr[18] | ((int32_t)hdr[19] << 8) |
                         ((int32_t)hdr[20] << 16) | ((int32_t)hdr[21] << 24);
    int32_t  bmpH     = hdr[22] | ((int32_t)hdr[23] << 8) |
                         ((int32_t)hdr[24] << 16) | ((int32_t)hdr[25] << 24);
    uint16_t bitCount = hdr[28] | ((uint16_t)hdr[29] << 8);

    if (bitCount != 24) {
        Serial.printf("[BMP] 仅支持 24-bit BMP（当前 %d-bit）\n", bitCount);
        f.close();
        return nullptr;
    }

    bool topDown = (bmpH < 0);
    if (topDown) bmpH = -bmpH;

    // 裁剪到最大尺寸（不缩放，居中裁剪）
    int w = (bmpW < maxW) ? (int)bmpW : maxW;
    int h = (bmpH < maxH) ? (int)bmpH : maxH;
    *outW = w;
    *outH = h;

    // 分配像素缓冲区（优先 PSRAM）
    uint16_t *buf = (uint16_t *)ps_malloc((size_t)w * h * 2);
    if (!buf)  buf = (uint16_t *)malloc((size_t)w * h * 2);
    if (!buf) {
        Serial.println("[BMP] 内存不足");
        f.close();
        return nullptr;
    }

    // 行缓冲（24-bit 每像素 3 字节，4 字节对齐）
    int rowBytes = (int)(((bmpW * 3) + 3) / 4) * 4;
    uint8_t *row = (uint8_t *)malloc(rowBytes);
    if (!row) {
        free(buf);
        f.close();
        return nullptr;
    }

    f.seek(pixOffset);

    for (int srcRow = 0; srcRow < (int)bmpH; srcRow++) {
        if (f.read(row, rowBytes) != (size_t)rowBytes) break;

        // BMP 默认底朝上：第 0 行对应图像最底行
        int destRow = topDown ? srcRow : (h - 1 - srcRow);
        if (destRow < 0 || destRow >= h) continue;

        for (int col = 0; col < w; col++) {
            uint8_t b = row[col * 3];
            uint8_t g = row[col * 3 + 1];
            uint8_t r = row[col * 3 + 2];
            // RGB888 → RGB565
            buf[destRow * w + col] =
                ((uint16_t)(r & 0xF8) << 8) |
                ((uint16_t)(g & 0xFC) << 3) |
                (b >> 3);
        }
    }

    free(row);
    f.close();
    Serial.printf("[BMP] 加载 %s (%d×%d)\n", path, w, h);
    return buf;
}
```

---

### ✅ 8）tag_map.h

```cpp
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "sd_card.h"
#include "config.h"

// ============================================================
//  NFC UID → 歌曲信息 映射
//  从 SD 卡 /tags.json 加载
// ============================================================

#define MAX_TAGS 64

struct SongInfo {
    char uid[24];     // NFC UID 字符串，如 "04:AB:CD:EF"
    char id[16];      // 歌曲 ID，如 "001"
    char title[64];   // 歌曲名称
    char artist[64];  // 歌手名称
};

static SongInfo g_tags[MAX_TAGS];
static int g_tagCount = 0;

// 从 SD 卡加载 tags.json
bool tags_load() {
    char *buf = nullptr;
    int sz = sdcard_read_file(SD_TAGS_FILE, &buf);
    if (sz < 0 || !buf) {
        Serial.println("[Tags] 无法读取 tags.json");
        return false;
    }

    DynamicJsonDocument doc(JSON_DOC_SIZE_TAGS);
    DeserializationError err = deserializeJson(doc, buf);
    free(buf);

    if (err) {
        Serial.printf("[Tags] JSON 解析失败: %s\n", err.c_str());
        return false;
    }

    g_tagCount = 0;
    for (JsonPair kv : doc.as<JsonObject>()) {
        if (g_tagCount >= MAX_TAGS) break;
        SongInfo &s = g_tags[g_tagCount];
        strncpy(s.uid, kv.key().c_str(), sizeof(s.uid) - 1);

        // "id" is required; skip entry if missing
        const char *idVal = kv.value()["id"].as<const char*>();
        if (!idVal) continue;
        strncpy(s.id, idVal, sizeof(s.id) - 1);

        const char *titleVal  = kv.value()["title"].as<const char*>();
        const char *artistVal = kv.value()["artist"].as<const char*>();
        strncpy(s.title,  titleVal  ? titleVal  : "未知歌曲", sizeof(s.title)  - 1);
        strncpy(s.artist, artistVal ? artistVal : "未知歌手", sizeof(s.artist) - 1);
        s.uid[sizeof(s.uid)-1]       = '\0';
        s.id[sizeof(s.id)-1]         = '\0';
        s.title[sizeof(s.title)-1]   = '\0';
        s.artist[sizeof(s.artist)-1] = '\0';
        g_tagCount++;
    }
    Serial.printf("[Tags] 加载了 %d 条 NFC 映射\n", g_tagCount);
    return true;
}

// 根据 UID 字符串查找歌曲信息，找到返回指针，未找到返回 nullptr
const SongInfo *tags_find(const char *uidStr) {
    for (int i = 0; i < g_tagCount; i++) {
        if (strcasecmp(g_tags[i].uid, uidStr) == 0) {
            return &g_tags[i];
        }
    }
    return nullptr;
}
```

---

### ✅ 9）pdm_audio.h

```cpp
#pragma once

/*
 * AudioOutputPDM - ESP32-S3 PDM-TX 音频输出
 *
 * 无需外接 DAC 模块！使用 ESP32-S3 I2S0 的 PDM-TX 功能，
 * 配合 2 级 RC 低通滤波器，将数字音频输出为模拟信号送入 ZK-502C AUX 输入。
 *
 * 硬件连接（2 级 RC 滤波，截止频率 ≈ 16kHz）：
 *
 *   GPIO14 ──┤R1 1kΩ├──┬──┤R2 1kΩ├──┬── ZK-502C L-IN（同时接 R-IN）
 *                       │             │
 *                      C1            C2
 *                     10nF           10nF
 *                       │             │
 *                      GND           GND
 *
 * 注：PDM 输出为单声道；将同一输出同时接到 ZK-502C 的 L-IN 和 R-IN，
 *     两个声道播放相同内容（双声道并联，喇叭照常独立工作）。
 *
 * 所需元件（约 ¥0.5）：
 *   - 1kΩ 电阻 × 2
 *   - 10nF 瓷片电容 × 2
 *
 * 兼容：ESP32-S3 + Arduino ESP32 3.x（基于 ESP-IDF 5.x）
 */

#include <Arduino.h>
#include <AudioOutput.h>         // ESP8266Audio 库的抽象基类
#include "driver/i2s_pdm.h"     // ESP-IDF 5.x PDM TX API
#include "pins.h"
#include "config.h"

class AudioOutputPDM : public AudioOutput {
public:
    AudioOutputPDM() : tx_handle(nullptr), g_gain(1.0f) {}

    ~AudioOutputPDM() { stop(); }

    // 初始化 PDM TX 通道
    bool begin() override {
        i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
        chan_cfg.auto_clear = true;

        esp_err_t err = i2s_new_channel(&chan_cfg, &tx_handle, nullptr);
        if (err != ESP_OK) {
            Serial.printf("[PDM] i2s_new_channel 失败: 0x%x\n", err);
            tx_handle = nullptr;
            return false;
        }

        i2s_pdm_tx_config_t pdm_cfg = {
            .clk_cfg  = I2S_PDM_TX_CLK_DEFAULT_CONFIG(AUDIO_SAMPLE_RATE),
            // 使用单声道；PCM 样本将在 ConsumeSample 中混为单声道后写入
            .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(
                            I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
            .gpio_cfg = {
                .clk  = I2S_GPIO_UNUSED,          // PDM 不需要外部时钟引脚
                .dout = (gpio_num_t)PIN_PDM_OUT,
                .invert_flags = { .clk_inv = false },
            },
        };

        err = i2s_channel_init_pdm_tx_mode(tx_handle, &pdm_cfg);
        if (err != ESP_OK) {
            Serial.printf("[PDM] init_pdm_tx_mode 失败: 0x%x\n", err);
            i2s_del_channel(tx_handle);
            tx_handle = nullptr;
            return false;
        }

        err = i2s_channel_enable(tx_handle);
        if (err != ESP_OK) {
            Serial.printf("[PDM] channel_enable 失败: 0x%x\n", err);
            i2s_del_channel(tx_handle);
            tx_handle = nullptr;
            return false;
        }

        Serial.println("[PDM] PDM TX 初始化成功（GPIO " STR(PIN_PDM_OUT) "）");
        return true;
    }

    // 接收一个立体声帧 sample[0]=L, sample[1]=R，混为单声道后写入 PDM
    bool ConsumeSample(int16_t sample[2]) override {
        if (!tx_handle) return false;

        // 左右混为单声道，并应用增益
        int32_t mixed = ((int32_t)sample[0] + (int32_t)sample[1]) >> 1;
        int16_t out   = (int16_t)constrain((int32_t)(mixed * g_gain), -32768, 32767);

        size_t written = 0;
        esp_err_t err = i2s_channel_write(tx_handle, &out, sizeof(out),
                                          &written, portMAX_DELAY);
        return (err == ESP_OK && written == sizeof(out));
    }

    // 停止并释放 PDM 通道
    bool stop() override {
        if (tx_handle) {
            i2s_channel_disable(tx_handle);
            i2s_del_channel(tx_handle);
            tx_handle = nullptr;
        }
        return true;
    }

    // 设置增益 (0.0–1.0 对应固件音量)
    bool SetGain(float f) override {
        g_gain = f;
        return true;
    }

private:
    i2s_chan_handle_t tx_handle;
    float g_gain;

    // 辅助宏：将宏参数转为字符串（用于 Serial 打印引脚号）
    #define XSTR(x) #x
#define STR(x)  XSTR(x)
};
```

> 注：`STR/XSTR` 宏在文件结尾定义，但它们仅用于同一文件内的字符串拼接，放在后面也能正常编译。

---

## ✅ 最后检查清单

- [ ] 所有 9 个文件都已创建且名称一致  
- [ ] 代码完整复制、没有遗漏  
- [ ] TFT_eSPI 已正确配置（README 有配置示例）  
- [ ] 编译通过后再上传  

完成后，就可以按 README 中的“Arduino IDE 一步一步烧录流程”进行编译和烧录。
