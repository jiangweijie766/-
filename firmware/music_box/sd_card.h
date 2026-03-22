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
