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
