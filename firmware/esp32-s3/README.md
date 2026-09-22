# ESP32-S3 大脑上位机固件与烧录指南

本目录包含《复苏计划》四足机器人大脑上位机（ESP32-S3-CAM）的固件配置、引脚适配文件与身脑联动代码。

---

## 🚀 方式一：小智 AI 固件 Web 网页一键免环境烧录（强烈推荐）

官方小智 AI（xiaozhi-esp32）提供了免配编译环境的浏览器 Web 烧录工具，插上 USB 即可一键刷入：

1. 使用 **Chrome** 或 **Edge** 浏览器访问小智官方烧录页面：  
   👉 [https://xiaozhi.me](https://xiaozhi.me) 或官方固件刷机入口
2. 用 Type-C 数据线将 ESP32-S3-CAM 连接电脑；
3. 点击页面上的 **“连接设备 / 一键烧录”**，在弹出的串口列表中选择对应的 COM 口；
4. 选择固件型号为 **ESP32-S3 (16MB Flash + 8MB PSRAM)**；
5. 在高级引脚自定义或配置文件导入中，将 `config/xiaozhi_custom_pins.json` 的引脚参数导入（或手动核对引脚）：
   * **麦克风 (INMP441)**：`SD=4`, `WS=5`, `SCK=6`
   * **功放 (MAX98357A)**：`DOUT=7`, `LRC=1`, `BCLK=2`
   * **屏幕 (SSD1306)**：`SCL=8`, `SDA=9`
   * **身脑串口**：`TX=43`, `RX=44` (波特率 115200)
6. 点击烧录，等待进度条走完（约 1~2 分钟）。
7. 烧录完成后，设备重启进入配网热点（如 `Xiaozhi-Setup`），手机连接并输入 Wi-Fi 密码即可连接云端大模型！

---

## 🛠️ 方式二：本地 `esptool.py` 一键命令行烧录

如果你已下载了编译好的小智二进制包（`bootloader.bin`, `partition-table.bin`, `firmware.bin`）：

1. 将固件二进制文件放置在 `tools/` 目录下；
2. 运行仓库根目录下的批处理脚本：
   ```cmd
   tools\flash_esp32.bat
   ```
3. 脚本会自动使用 `esptool.py` 将固件写入 `0x0`, `0x8000`, `0x10000` 地址。

---

## 📷 方式三：视觉追踪独立测试工程 (`vision_tracker/`)

在进行 Phase 3（视觉人脸与视线追踪）联调时，无需每次都跑整个大模型固件，可以使用轻量测试工程：

1. 打开 **Arduino IDE**；
2. 安装 **esp32 by Espressif** 板卡支持包（版本建议 2.0.11 或 3.0+）；
3. 开发板选择：**ESP32S3 Dev Module**；
   * **PSRAM**：`OPI PSRAM`
   * **Flash Size**：`16MB (128Mb)`
4. 打开 `firmware/esp32-s3/vision_tracker/vision_tracker.ino`；
5. 点击 **上传**；
6. 打开串口监视器，即可看到摄像头检测并向 STM32 实时发送 `$TRACK,<x>,<y>#` 姿态调整帧！
