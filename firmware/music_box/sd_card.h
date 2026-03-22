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
