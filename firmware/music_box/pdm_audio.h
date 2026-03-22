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

// 辅助宏：将宏参数转为字符串（用于 Serial 打印引脚号）
#define XSTR(x) #x
#define STR(x)  XSTR(x)

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
};
