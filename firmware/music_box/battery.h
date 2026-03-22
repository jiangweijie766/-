#pragma once

#include <Arduino.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  电池电量监测模块
//
//  硬件接线（分压器）：
//    12V 电池正极
//        │
//       [R1 220kΩ]
//        │
//        ├──→ PIN_BATT_ADC (GPIO7)
//        │
//       [R2 47kΩ]
//        │
//       GND
//
//  分压后最高电压：12.6V × 47/(220+47) ≈ 2.22V（在 ADC_11db 量程内 ✓）
// ============================================================

static int  g_battPct = 100;
static bool g_battLow = false;

// 初始化电池 ADC 通道
void battery_init() {
    analogReadResolution(12);                             // 12-bit，0–4095
    analogSetPinAttenuation(PIN_BATT_ADC, ADC_11db);     // 0–3.3V 量程
}

// 读取电池电压（mV），取 8 次平均减少噪声
int battery_read_mv() {
    long sum = 0;
    for (int i = 0; i < 8; i++) {
        sum += analogRead(PIN_BATT_ADC);
        delay(2);
    }
    int raw = (int)(sum / 8);
    // ADC 电压 (mV)
    int adcMv = (int)((long)raw * BATT_ADC_VREF_MV / 4095);
    // 还原电池电压：V_batt = V_adc / (R2 / (R1 + R2))
    int battMv = (int)((long)adcMv * (BATT_R1_KOHM + BATT_R2_KOHM) / BATT_R2_KOHM);
    return battMv;
}

// 电压转百分比（线性映射，3S 锂电：9.0V=0%，12.6V=100%）
int battery_mv_to_pct(int mv) {
    if (mv >= BATT_FULL_MV)  return 100;
    if (mv <= BATT_EMPTY_MV) return 0;
    return (int)((long)(mv - BATT_EMPTY_MV) * 100 / (BATT_FULL_MV - BATT_EMPTY_MV));
}

// 刷新全局电量（在主循环按 BATT_UPDATE_MS 周期调用）
void battery_update() {
    int mv    = battery_read_mv();
    g_battPct = battery_mv_to_pct(mv);
    g_battLow = (g_battPct <= BATT_LOW_PERCENT);
    Serial.printf("[Batt] %d mV -> %d%%%s\n", mv, g_battPct, g_battLow ? " ⚠低电量" : "");
}

int  battery_get_pct() { return g_battPct; }
bool battery_is_low()  { return g_battLow; }
