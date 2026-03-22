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
 *   - 电池分压电阻：R1=220kΩ, R2=47kΩ → GPIO7
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
 *   · 电池电量显示 + 低电量告警（< 20%）
 *   · 蓝牙模式（长按播放键切换；音频由 ZK-502C 板载 BT 接收）
 */

#include <Arduino.h>
#include "pins.h"
#include "config.h"
#include "sd_card.h"      // 必须在 battery.h / display.h 之前（BMP 加载函数）
#include "battery.h"      // 必须在 display.h 之前（display_idle 引用 g_battPct）
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

// ——— 低电量告警（每次电量更新仅告警一次）———
static bool g_lowBattAlerted = false;

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

// ——— 定时器 ———
static unsigned long g_lastBattUpdate = 0;

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

    // 电池 ADC
    battery_init();
    battery_update();               // 启动时立刻读一次
    g_lastBattUpdate = millis();

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

    // —— 电量定时刷新 ——
    if (now - g_lastBattUpdate >= BATT_UPDATE_MS) {
        g_lastBattUpdate = now;
        battery_update();

        // 低电量首次告警（STATE_PLAYING / IDLE 下均提示）
        if (battery_is_low() && !g_lowBattAlerted && g_state != STATE_BT_MODE) {
            g_lowBattAlerted = true;
            display_battery_warning(battery_get_pct());
            // 告警后恢复当前界面
            if (g_state == STATE_PLAYING) {
                display_vinyl_frame(g_vinylAngle, battery_get_pct(), false);
            } else if (g_state == STATE_PAUSED) {
                display_vinyl_frame(g_vinylAngle, battery_get_pct(), true);
            } else if (g_state == STATE_BT_MODE) {
                display_bluetooth(battery_get_pct());
            } else {
                display_idle();
            }
        }
        // 电量回升后重置告警标志
        if (!battery_is_low()) g_lowBattAlerted = false;
    }

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
        display_vinyl_frame(g_vinylAngle, battery_get_pct(),
                            g_state == STATE_PAUSED);
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
        display_vinyl_frame(g_vinylAngle, battery_get_pct(), true);

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
        display_bluetooth(battery_get_pct());
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
