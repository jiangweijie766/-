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
