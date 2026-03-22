#pragma once

#include <Wire.h>
#include <Adafruit_PN532.h>
#include "pins.h"
#include "config.h"

// ============================================================
//  PN532 NFC 读卡模块（I2C 接口）
// ============================================================

static Adafruit_PN532 nfc(PIN_NFC_SDA, PIN_NFC_SCL);

// NFC 模块初始化
bool nfc_init() {
    Wire.begin(PIN_NFC_SDA, PIN_NFC_SCL);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[NFC] 未找到 PN532 模块");
        return false;
    }
    Serial.printf("[NFC] 固件版本: v%d.%d\n",
                  (versiondata >> 16) & 0xFF,
                  (versiondata >> 8) & 0xFF);

    nfc.SAMConfig();
    Serial.println("[NFC] PN532 初始化成功");
    return true;
}

// 将 UID 字节数组转为十六进制字符串，例如 "04:AB:CD:EF"
void nfc_uid_to_str(uint8_t *uid, uint8_t uidLen, char *outStr, size_t outSize) {
    outStr[0] = '\0';
    for (uint8_t i = 0; i < uidLen && i < NFC_UID_MAX_LEN; i++) {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X", uid[i]);
        strncat(outStr, buf, outSize - strlen(outStr) - 1);
        if (i < uidLen - 1) {
            strncat(outStr, ":", outSize - strlen(outStr) - 1);
        }
    }
}

// 非阻塞轮询：若检测到卡片返回 true，并填充 uidStr
// uidStr 缓冲区大小至少 24 字节
bool nfc_poll(char *uidStr, size_t uidStrSize) {
    uint8_t uid[7];
    uint8_t uidLen;

    bool found = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 50);
    if (found) {
        nfc_uid_to_str(uid, uidLen, uidStr, uidStrSize);
        return true;
    }
    return false;
}
