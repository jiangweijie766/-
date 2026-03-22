#pragma once

#include <TFT_eSPI.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  GC9A01 圆形屏显示模块
//  使用 TFT_eSPI 库（需在 User_Setup.h 中配置 GC9A01）
// ============================================================

static TFT_eSPI tft = TFT_eSPI();

// 初始化显示屏
void display_init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BG_COLOR);
    // 开启背光
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
}

// 清屏
void display_clear() {
    tft.fillScreen(TFT_BG_COLOR);
}

// 显示待机界面
void display_idle() {
    display_clear();
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_TEXT_COLOR, TFT_BG_COLOR);
    tft.setTextSize(2);
    tft.drawString("NFC Music Box", 120, 100);
    tft.setTextSize(1);
    tft.setTextColor(0x7BEF, TFT_BG_COLOR);
    tft.drawString("请刷 NFC 标签选歌", 120, 140);
}

// 显示正在播放界面
void display_now_playing(const char *title, const char *artist, int volume) {
    display_clear();

    // 外圆装饰
    tft.drawCircle(120, 120, 118, 0x4208);
    tft.drawCircle(120, 120, 115, 0x2104);

    // 音符图标（简单绘制）
    tft.setTextColor(TFT_ACCENT_COLOR, TFT_BG_COLOR);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(3);
    tft.drawString("♪", 120, 60);

    // 歌曲标题
    tft.setTextColor(TFT_TEXT_COLOR, TFT_BG_COLOR);
    tft.setTextSize(2);
    // 截断过长标题
    char shortTitle[20];
    strncpy(shortTitle, title, 19);
    shortTitle[19] = '\0';
    tft.drawString(shortTitle, 120, 105);

    // 歌手
    tft.setTextSize(1);
    tft.setTextColor(0xAD55, TFT_BG_COLOR);
    char shortArtist[24];
    strncpy(shortArtist, artist, 23);
    shortArtist[23] = '\0';
    tft.drawString(shortArtist, 120, 130);

    // 音量条
    display_volume_bar(volume);
}

// 绘制音量条（底部弧形）
void display_volume_bar(int volume) {
    // 清除旧音量区域
    tft.fillRect(20, 165, 200, 20, TFT_BG_COLOR);

    tft.setTextColor(0x7BEF, TFT_BG_COLOR);
    tft.setTextDatum(ML_DATUM);
    tft.setTextSize(1);
    tft.drawString("VOL", 22, 175);

    // 进度条背景
    tft.drawRect(52, 169, 148, 10, 0x4208);
    // 进度条填充
    int barWidth = (volume * 144) / 100;
    if (barWidth > 0) {
        tft.fillRect(54, 171, barWidth, 6, TFT_ACCENT_COLOR);
    }

    // 数字
    char volStr[8];
    snprintf(volStr, sizeof(volStr), "%d%%", volume);
    tft.setTextDatum(MR_DATUM);
    tft.drawString(volStr, 218, 175);
}

// 显示 NFC 刷卡提示（短暂闪烁）
void display_nfc_detected() {
    tft.fillCircle(120, 120, 40, TFT_ACCENT_COLOR);
    tft.setTextColor(TFT_TEXT_COLOR, TFT_ACCENT_COLOR);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1);
    tft.drawString("识别中...", 120, 120);
    delay(300);
}

// 显示错误信息
void display_error(const char *msg) {
    display_clear();
    tft.setTextColor(TFT_ACCENT_COLOR, TFT_BG_COLOR);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Error", 120, 100);
    tft.setTextSize(1);
    tft.setTextColor(TFT_TEXT_COLOR, TFT_BG_COLOR);
    tft.drawString(msg, 120, 130);
}

// 旋转动画帧（播放时在屏幕中间显示旋转圆盘）
void display_spin_frame(int frame) {
    static int lastAngle = 0;
    int angle = (frame * 6) % 360;
    // 擦除旧指针
    int x1 = 120 + 25 * cos((lastAngle - 90) * PI / 180);
    int y1 = 120 + 25 * sin((lastAngle - 90) * PI / 180);
    tft.drawLine(120, 120, x1, y1, TFT_BG_COLOR);
    // 画新指针
    int x2 = 120 + 25 * cos((angle - 90) * PI / 180);
    int y2 = 120 + 25 * sin((angle - 90) * PI / 180);
    tft.drawLine(120, 120, x2, y2, TFT_ACCENT_COLOR);
    lastAngle = angle;
}
