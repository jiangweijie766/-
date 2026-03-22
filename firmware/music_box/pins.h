#pragma once

// ============================================================
//  ESP32-S3-DevKitC-1 引脚定义
//  硬件：GC9A01 + PN532 + ZK-502C + PCM5102A + SD 卡
// ============================================================

// ---------- GC9A01 圆形屏（SPI） ----------
#define PIN_TFT_MOSI  11
#define PIN_TFT_SCLK  12
#define PIN_TFT_CS    10
#define PIN_TFT_DC     9
#define PIN_TFT_RST    8
#define PIN_TFT_BL    46   // 背光控制（HIGH = 开启）

// ---------- SD 卡模块（SPI，与 TFT 共享总线） ----------
#define PIN_SD_MOSI   11   // 共享
#define PIN_SD_MISO   13
#define PIN_SD_SCLK   12   // 共享
#define PIN_SD_CS      1

// ---------- PN532 NFC 模块（I2C） ----------
#define PIN_NFC_SDA    5
#define PIN_NFC_SCL    6

// ---------- PCM5102A I2S DAC ----------
#define PIN_I2S_BCK   15   // 位时钟
#define PIN_I2S_LRCK  16   // 左右声道时钟
#define PIN_I2S_DATA  17   // 串行数据

// ---------- 音量旋转编码器（可选） ----------
#define PIN_ENC_A     18
#define PIN_ENC_B     19
#define PIN_ENC_BTN   20

// ---------- 功能按键 ----------
#define PIN_BTN_PREV  21   // 上一首
#define PIN_BTN_NEXT  47   // 下一首
#define PIN_BTN_PLAY  48   // 播放/暂停

// ---------- 状态 LED（可选，WS2812B 或普通 LED） ----------
#define PIN_STATUS_LED 38
