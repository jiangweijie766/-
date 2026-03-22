# ESP32-S3 NFC 音乐盒

基于 ESP32-S3-DevKitC-1 的 NFC 刷卡点歌音乐盒。使用 GC9A01 圆形屏显示歌曲信息，PN532 识别 NFC 标签，ZK-502C 功放驱动四只扬声器，音乐存储在 SD 卡中，由 12V 锂电池供电。

## 硬件清单

| 组件 | 型号/规格 |
|------|-----------|
| 主控 | ESP32-S3-DevKitC-1 |
| 显示屏 | GC9A01 1.28寸 240×240 圆形 TFT 彩屏（SPI） |
| 功放 | ZK-502C（TPA3116D2 芯片，双声道 D 类） |
| NFC | PN532 识别模块（I2C 接口） |
| 高音喇叭 | 4Ω 11W × 2 |
| 全频喇叭 | 4Ω 10W × 2 |
| 存储 | SD 卡读写模块（SPI 接口）+ MicroSD 卡 |
| 电池 | 德力普 12V 10800mAh 可充放电锂电池 |
| 降压模块 | 12V → 5V DC-DC 降压（输出 ≥ 3A） |

---

## 🤔 常见问题：必须要 DAC 模块吗？

**短答案：不一定需要买模块，但必须有某种方法把数字信号转为模拟信号。**

### 为什么需要转换？

| 设备 | 音频接口类型 |
|------|------------|
| ESP32-S3 | **只有数字输出**（I2S）；与原版 ESP32 不同，S3 没有内置 DAC |
| ZK-502C（TPA3116D2）| **只接受模拟输入**（AUX 3.5mm 口或板载蓝牙） |

两者之间存在"数字↔模拟"的鸿沟，必须跨越它。

> **关于 ZK-502C 的蓝牙：** 该模块使用经典蓝牙（A2DP），而 ESP32-S3 只有蓝牙 LE，两者不兼容，无法用蓝牙绕过。

---

### 两种方案对比

| | 方案 A（推荐）| 方案 B（无模块）|
|-|-------------|----------------|
| **原理** | I2S 数字信号 → PCM5102A DAC → 模拟输出 | I2S PDM-TX 数字信号 → RC 低通滤波 → 模拟输出 |
| **额外器件** | PCM5102A 模块（约 ¥15–25）| 1kΩ 电阻 ×2 + 10nF 电容 ×2（约 ¥0.5）|
| **音质** | 优（SNR >100dB，立体声）| 良（底噪略高，单声道）|
| **接线复杂度** | 简单（3 根信号线）| 简单（1 根信号线 + RC 滤波器）|
| **固件修改** | 无需（默认）| 修改 `config.h` 一行 |
| **适合场景** | 追求音质、已有模块 | 手边无模块、想快速验证 |

---

### 方案 A：PCM5102A I2S DAC（推荐）

这是默认方案，固件无需修改。

**接线：**

| PCM5102A 引脚 | ESP32-S3 GPIO |
|-------------|--------------|
| BCK (BCLK)  | GPIO 15      |
| LCK (LRCK)  | GPIO 16      |
| DIN (DATA)  | GPIO 17      |
| VCC         | 3.3V         |
| GND         | GND          |
| FLT         | GND          |
| DEMP        | GND          |
| XSMT        | 3.3V（静音控制，高电平 = 工作）|
| FMT         | GND          |

PCM5102A 的 LOUT/ROUT → ZK-502C 的 L-IN/R-IN。

---

### 方案 B：PDM 输出 + RC 低通滤波（无需 DAC 模块）

只需 **2 个电阻 + 2 个电容**（共约 ¥0.5），ESP32-S3 GPIO 14 直接驱动滤波器后送入 ZK-502C AUX。

**修改 `config.h`（只改一行）：**

```cpp
#define AUDIO_MODE  AUDIO_MODE_PDM   // 将默认的 AUDIO_MODE_I2S_DAC 改为此行
```

**硬件电路（2 级 RC 低通滤波器）：**

```
ESP32-S3
GPIO 14 ──┤ R1 1kΩ ├──┬──┤ R2 1kΩ ├──┬── ZK-502C L-IN
                       │              │   （同时接 R-IN，双声道播相同内容）
                      C1             C2
                     10nF           10nF
                       │              │
                      GND            GND
```

截止频率 ≈ 1/(2π × 1kΩ × 10nF) ≈ **16kHz**（足以覆盖人耳听觉范围）。

> **注意**：PDM 方案输出为单声道，L-IN 和 R-IN 接同一信号，
> 喇叭仍然各自独立工作（左右声道内容相同）。

---

## 引脚分配

### GC9A01 圆形屏（SPI）

| 屏幕引脚 | ESP32-S3 GPIO |
|---------|--------------|
| MOSI    | GPIO 11      |
| SCLK    | GPIO 12      |
| CS      | GPIO 10      |
| DC      | GPIO 9       |
| RST     | GPIO 8       |
| BL      | GPIO 46      |
| VCC     | 3.3V         |
| GND     | GND          |

### SD 卡模块（SPI，与屏幕共享总线）

| SD 引脚 | ESP32-S3 GPIO |
|--------|--------------|
| MOSI   | GPIO 11      |
| MISO   | GPIO 13      |
| SCLK   | GPIO 12      |
| CS     | GPIO 1       |
| VCC    | 3.3V         |
| GND    | GND          |

### PN532 NFC 模块（I2C）

| PN532 引脚 | ESP32-S3 GPIO |
|-----------|--------------|
| SDA       | GPIO 5       |
| SCL       | GPIO 6       |
| VCC       | 3.3V         |
| GND       | GND          |

> 将 PN532 模块上的拨码开关设为 **I2C 模式**（SW1=0, SW2=0）。

### 音频输出引脚（二选一）

**方案 A：PCM5102A DAC（`AUDIO_MODE_I2S_DAC`，默认）**

| PCM5102A 引脚 | ESP32-S3 GPIO |
|-------------|--------------|
| BCK (BCLK)  | GPIO 15      |
| LCK (LRCK)  | GPIO 16      |
| DIN (DATA)  | GPIO 17      |
| FLT / DEMP / FMT | GND   |
| XSMT        | 3.3V         |
| VCC         | 3.3V         |
| GND         | GND          |

PCM5102A 的 LOUT/ROUT 接 ZK-502C 的 L-IN/R-IN。

**方案 B：PDM + RC 滤波（`AUDIO_MODE_PDM`，无需模块）**

| 信号 | ESP32-S3 GPIO |
|------|--------------|
| PDM 数据输出 | GPIO 14 |

GPIO 14 经 2 级 RC 滤波（详见上方电路图）后接 ZK-502C AUX 输入。

### ZK-502C 功放供电与连接

| ZK-502C 引脚 | 连接目标 |
|------------|---------|
| VCC（12V）  | 12V 电池正极 |
| GND         | 公共地  |
| L-IN / R-IN | PCM5102A LOUT/ROUT（方案 A）或 PDM 滤波输出（方案 B）|
| L+ L−       | 全频喇叭（4Ω 10W）|
| R+ R−       | 全频喇叭（4Ω 10W）|

> 高音喇叭（4Ω 11W）通过 **4.7μF 无极性电容** 串联后并联在全频喇叭输出端，起高通分频作用。

### 电源连接

```
12V 电池 ──┬──→ ZK-502C VCC（直接 12V 供电）
           └──→ 12V→5V 降压模块 VIN
                     │
                     └──→ 5V 输出 ──→ ESP32-S3 5V 引脚（USB 口或 5V 排针）
                                    ├──→ PCM5102A（方案 A，3.3V 由 ESP32 板载 LDO 提供）
                                    └──→ PN532 / SD 卡模块（3.3V 由 ESP32 板载 LDO 提供）
```

---

## 软件依赖（Arduino IDE / PlatformIO）

在 Arduino IDE 中安装以下库：

| 库名 | 用途 |
|------|------|
| `TFT_eSPI` | GC9A01 显示屏驱动 |
| `Adafruit PN532` | PN532 NFC 读卡 |
| `ESP8266Audio` | I2S/PDM 音频播放（MP3/WAV/FLAC）|
| `ArduinoJson` | JSON 配置文件解析 |

### TFT_eSPI 配置

修改 `User_Setup.h`（位于 TFT_eSPI 库目录内）：

```cpp
#define GC9A01_DRIVER
#define TFT_WIDTH  240
#define TFT_HEIGHT 240
#define TFT_MOSI  11
#define TFT_SCLK  12
#define TFT_CS    10
#define TFT_DC     9
#define TFT_RST    8
#define TFT_BL    46
#define TFT_BACKLIGHT_ON HIGH
#define SPI_FREQUENCY  40000000
```

---

## SD 卡目录结构

```
/
├── music/
│   ├── 001.mp3        ← NFC 标签 001 对应的歌曲
│   ├── 002.mp3
│   └── ...
├── cover/
│   ├── 001.bmp        ← 240×240 BMP 封面图（可选）
│   └── ...
└── tags.json          ← NFC UID → 歌曲映射表
```

`tags.json` 示例：

```json
{
  "04:AB:CD:EF": {"id": "001", "title": "月亮代表我的心", "artist": "邓丽君"},
  "04:12:34:56": {"id": "002", "title": "夜曲", "artist": "周杰伦"}
}
```

---

## 编译与烧录

1. 在 Arduino IDE 中选择开发板：**ESP32S3 Dev Module**
2. Flash Mode：**QIO 80MHz**
3. Partition Scheme：**Huge APP (3MB No OTA/1MB SPIFFS)**
4. PSRAM：**OPI PSRAM**（如使用 N8R8 版本）
5. 将 `firmware/music_box/` 目录下所有 `.ino`/`.h` 文件放在同名文件夹中编译

---

## 注意事项

1. **共地**：12V 电池、降压模块、ESP32-S3、ZK-502C 必须共地。
2. **电流**：降压模块选用至少 3A 输出，ZK-502C 空载约 300mA，满功率可达 3A+。
3. **散热**：ZK-502C（TPA3116D2）在大音量下会发热，建议加散热片。
4. **NFC 天线距离**：PN532 读卡距离约 3-5cm，保持标签与模块平行。
5. **SD 卡格式**：使用 FAT32 格式，文件名限 8.3 格式（`001.mp3`）。
6. **音量控制**：ZK-502C 板上有电位器可调整最大音量；固件通过 I2S 音量调节软件音量。
