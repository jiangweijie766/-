/*
 * ESP32-S3 NFC 音乐盒 - 主程序
 *
 * 硬件：
 *   - ESP32-S3-DevKitC-1
 *   - GC9A01  1.28" 240×240 圆形 TFT 屏（SPI）
 *   - PN532   NFC 读卡模块（I2C）
 *   - ZK-502C 功放（TPA3116D2，模拟输入）
 *   - PCM5102A I2S DAC（桥接 ESP32 I2S 与 ZK-502C 模拟输入）
 *   - SD 卡读写模块（SPI）
 *   - 德力普 12V 10800mAh 锂电池 + 12V→5V 降压模块
 *   - 高音喇叭 4Ω 11W × 2，全频喇叭 4Ω 10W × 2
 *
 * 依赖库（Arduino IDE 库管理器安装）：
 *   - TFT_eSPI              (GC9A01 显示)
 *   - Adafruit PN532        (NFC)
 *   - ESP8266Audio / ESP32-audioI2S (音频播放)
 *   - ArduinoJson           (JSON 解析)
 *   - SD (内置)
 */

#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "display.h"
#include "nfc_reader.h"
#include "sd_card.h"
#include "audio_player.h"
#include "tag_map.h"

// ——— 状态机 ———
enum State { STATE_IDLE, STATE_PLAYING, STATE_PAUSED };
static State g_state = STATE_IDLE;

// ——— 当前歌曲信息 ———
static char g_currentTitle[64]  = "";
static char g_currentArtist[64] = "";
static char g_currentPath[128]  = "";

// ——— 按键去抖 ———
static unsigned long g_lastBtnPlay = 0;
static unsigned long g_lastBtnPrev = 0;
static unsigned long g_lastBtnNext = 0;
#define BTN_DEBOUNCE_MS 200

// ——— NFC 冷却 ———
static unsigned long g_lastNfcTime = 0;
static char g_lastNfcUid[24] = "";

// ——— 音量旋转编码器 ———
static int g_encLastA = HIGH;

// ——— 前向声明 ———
void on_nfc_detected(const char *uidStr);
void on_btn_playpause();
void play_file(const char *path);
void play_by_index(int index);
void play_next();
void play_prev();

// SD 卡中音乐列表（用于上一首/下一首）
static char g_musicList[128][64];
static int  g_musicCount = 0;
static int  g_musicIndex = 0;

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

    // 扫描 SD 卡音乐列表（用于上下曲切换）
    g_musicCount = sdcard_list_dir(SD_MUSIC_DIR, g_musicList, 128);
    Serial.printf("[Boot] 发现 %d 首音乐\n", g_musicCount);

    // 初始化音频输出（I2S → PCM5102A → ZK-502C）
    audio_init();
    audio_set_volume(AUDIO_VOLUME_DEFAULT);

    // 初始化 NFC
    if (!nfc_init()) {
        display_error("NFC失败");
        // NFC 失败不阻止启动，仍可通过按键操作
    }

    display_idle();
    Serial.println("[Boot] 初始化完成，等待 NFC 刷卡...");
}

// ============================================================
void loop() {
    unsigned long now = millis();

    // —— 音频驱动（必须在主循环频繁调用）——
    bool stillPlaying = audio_loop();
    if (!stillPlaying && g_state == STATE_PLAYING) {
        // 当前歌曲播放结束，自动播放下一首
        Serial.println("[Loop] 播放结束，自动下一首");
        play_next();
    }

    // —— NFC 轮询 ——
    if (now - g_lastNfcTime > NFC_POLL_INTERVAL_MS) {
        char uidStr[24];
        if (nfc_poll(uidStr, sizeof(uidStr))) {
            // 冷却：同一张卡不重复触发
            bool sameCard = (strcmp(uidStr, g_lastNfcUid) == 0);
            bool cooled   = (now - g_lastNfcTime > NFC_DEBOUNCE_MS);
            if (!sameCard || cooled) {
                strncpy(g_lastNfcUid, uidStr, sizeof(g_lastNfcUid) - 1);
                g_lastNfcTime = now;
                on_nfc_detected(uidStr);
            }
        }
    }

    // —— 按键处理 ——
    // 播放/暂停
    if (digitalRead(PIN_BTN_PLAY) == LOW && now - g_lastBtnPlay > BTN_DEBOUNCE_MS) {
        g_lastBtnPlay = now;
        on_btn_playpause();
    }
    // 上一首
    if (digitalRead(PIN_BTN_PREV) == LOW && now - g_lastBtnPrev > BTN_DEBOUNCE_MS) {
        g_lastBtnPrev = now;
        play_prev();
    }
    // 下一首
    if (digitalRead(PIN_BTN_NEXT) == LOW && now - g_lastBtnNext > BTN_DEBOUNCE_MS) {
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
        if (g_state == STATE_PLAYING) {
            display_volume_bar(audio_get_volume());
        }
    }
    g_encLastA = encA;

    // —— 旋转动画（播放中每帧刷新）——
    static unsigned long lastAnimFrame = 0;
    static int animFrame = 0;
    if (g_state == STATE_PLAYING && now - lastAnimFrame > (1000 / TFT_ANIM_FPS)) {
        lastAnimFrame = now;
        display_spin_frame(animFrame++);
    }
}

// ============================================================
//  事件处理函数
// ============================================================

// NFC 刷卡事件
void on_nfc_detected(const char *uidStr) {
    Serial.printf("[NFC] 检测到标签: %s\n", uidStr);
    display_nfc_detected();

    const SongInfo *song = tags_find(uidStr);
    if (song) {
        char musicPath[128];
        if (sdcard_find_music(song->id, musicPath, sizeof(musicPath))) {
            strncpy(g_currentTitle,  song->title,  sizeof(g_currentTitle) - 1);
            strncpy(g_currentArtist, song->artist, sizeof(g_currentArtist) - 1);
            play_file(musicPath);
        } else {
            Serial.printf("[NFC] 未找到歌曲文件: id=%s\n", song->id);
            display_error("文件未找到");
        }
    } else {
        Serial.printf("[NFC] UID 未映射: %s\n", uidStr);
        display_error("标签未注册");
        delay(1500);
        display_idle();
    }
}

// 播放/暂停切换
void on_btn_playpause() {
    if (g_state == STATE_PLAYING) {
        audio_stop();
        g_state = STATE_PAUSED;
        Serial.println("[Btn] 已暂停");
        display_idle();
    } else if (g_state == STATE_PAUSED && g_currentPath[0] != '\0') {
        play_file(g_currentPath);
    } else if (g_state == STATE_IDLE && g_musicCount > 0) {
        // 空闲时按播放键，从列表第一首开始
        play_by_index(0);
    }
}

// 播放指定文件路径
void play_file(const char *path) {
    strncpy(g_currentPath, path, sizeof(g_currentPath) - 1);
    if (audio_play(path)) {
        g_state = STATE_PLAYING;
        display_now_playing(g_currentTitle, g_currentArtist, audio_get_volume());
        // 更新 g_musicIndex（如果文件在列表中）
        char filename[64] = {0};
        const char *fn = strrchr(path, '/');
        if (fn) {
            strncpy(filename, fn + 1, sizeof(filename) - 1);
        }
        for (int i = 0; i < g_musicCount; i++) {
            if (strcmp(g_musicList[i], filename) == 0) {
                g_musicIndex = i;
                break;
            }
        }
    } else {
        g_state = STATE_IDLE;
        display_error("播放失败");
        delay(1500);
        display_idle();
    }
}

// 按列表下标播放
void play_by_index(int index) {
    if (g_musicCount == 0) return;
    // 双重取模实现环形索引，同时处理负数（上一首越界）情况
    g_musicIndex = ((index % g_musicCount) + g_musicCount) % g_musicCount;
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", SD_MUSIC_DIR, g_musicList[g_musicIndex]);
    // 使用文件名（去扩展名）作为标题
    char title[64];
    strncpy(title, g_musicList[g_musicIndex], sizeof(title) - 1);
    char *dot = strrchr(title, '.');
    if (dot) *dot = '\0';
    strncpy(g_currentTitle, title, sizeof(g_currentTitle) - 1);
    strncpy(g_currentArtist, "", sizeof(g_currentArtist) - 1);
    play_file(path);
}

// 下一首
void play_next() {
    play_by_index(g_musicIndex + 1);
}

// 上一首
void play_prev() {
    play_by_index(g_musicIndex - 1);
}
