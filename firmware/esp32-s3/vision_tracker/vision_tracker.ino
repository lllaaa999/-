/**
 ******************************************************************************
 * @file           : vision_tracker.ino
 * @brief          : 《复苏计划》Phase 3 视觉追踪与视线偏差提取独立测试工程
 * @target         : ESP32-S3-CAM (N16R8, OV2640 120° 广角镜头)
 * @description    : 初始化 OV2640 广角摄像头，捕获 QVGA (320x240) 画面，
 *                   提取画面目标中心偏差，通过串口向上位机/下位机发送 $TRACK,x,y#
 ******************************************************************************
 */

#include "esp_camera.h"
#include <Arduino.h>

// ESP32-S3-CAM 专有 DVP 摄像头引脚定义
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM      4
#define SIOC_GPIO_NUM      5

#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM        8
#define Y3_GPIO_NUM        9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM     6
#define HREF_GPIO_NUM      7
#define PCLK_GPIO_NUM     13

// 身脑通信串口引脚定义
#define BODY_UART_TX_PIN  43
#define BODY_UART_RX_PIN  44
#define BODY_UART_BAUD    115200

// 画面几何尺寸 (QVGA)
#define FRAME_WIDTH       320
#define FRAME_HEIGHT      240
#define CENTER_X          (FRAME_WIDTH / 2)
#define CENTER_Y          (FRAME_HEIGHT / 2)

// 死区阈值 (百分比)
#define DEADBAND_PERCENT  8

// 平滑滑动滤波参数
float filtered_ox = 0.0;
float filtered_oy = 0.0;
const float ALPHA = 0.5;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n[Vision Tracker] Booting Recovery-Plan Vision Subsystem...");

    // 初始化身脑串口 (Serial1: RX=44, TX=43)
    Serial1.begin(BODY_UART_BAUD, SERIAL_8N1, BODY_UART_RX_PIN, BODY_UART_TX_PIN);
    Serial.println("[Vision Tracker] Serial1 connected to STM32 on TX:43 RX:44");

    // 摄像头配置参数
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_GRAYSCALE; // 灰度图便于低延时分析
    config.frame_size   = FRAMESIZE_QVGA;     // 320x240
    config.jpeg_quality = 12;
    config.fb_count     = 2;
    config.fb_location  = CAMERA_FB_IN_PSRAM;
    config.grab_mode    = CAMERA_GRAB_LATEST;

    // 摄像头初始化
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("[Vision Tracker] ERROR: Camera init failed with error 0x%x\n", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s != NULL) {
        s->set_vflip(s, 1);    // 根据装配方向垂直翻转
        s->set_hmirror(s, 0);  // 水平镜像设置
    }

    Serial.println("[Vision Tracker] OV2640 120-deg Camera successfully initialized!");
}

void loop() {
    // 捕获一帧图像
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[Vision Tracker] Camera capture failed!");
        delay(50);
        return;
    }

    // 示例目标定位逻辑：寻找画面中最亮或对比度最高区域的中心质心
    // (在集成 ESP-WHO 后，此处替换为 human_face_detect 的 Bounding Box 中心)
    uint32_t sum_x = 0;
    uint32_t sum_y = 0;
    uint32_t count = 0;

    // 采样步长隔行隔列加速计算 (4x4 采样)
    for (int y = 20; y < FRAME_HEIGHT - 20; y += 4) {
        for (int x = 20; x < FRAME_WIDTH - 20; x += 4) {
            uint8_t pixel = fb->buf[y * FRAME_WIDTH + x];
            if (pixel > 200) { // 高亮特征阈值
                sum_x += x;
                sum_y += y;
                count++;
            }
        }
    }

    // 释放图像帧缓冲区
    esp_camera_fb_return(fb);

    if (count > 20) {
        int target_x = sum_x / count;
        int target_y = sum_y / count;

        // 计算相对画面中心的百分比偏差 (-100 ~ 100)
        float raw_ox = (float)(target_x - CENTER_X) / (float)CENTER_X * 100.0f;
        float raw_oy = (float)(target_y - CENTER_Y) / (float)CENTER_Y * 100.0f;

        // 低通平滑滤波
        filtered_ox = ALPHA * raw_ox + (1.0f - ALPHA) * filtered_ox;
        filtered_oy = ALPHA * raw_oy + (1.0f - ALPHA) * filtered_oy;

        int final_ox = (int)filtered_ox;
        int final_oy = (int)filtered_oy;

        // 死区抑制
        if (abs(final_ox) < DEADBAND_PERCENT) final_ox = 0;
        if (abs(final_oy) < DEADBAND_PERCENT) final_oy = 0;

        // 打包身脑通信协议: $TRACK,<ox>,<oy>#
        char track_cmd[32];
        snprintf(track_cmd, sizeof(track_cmd), "$TRACK,%d,%d#\n", final_ox, final_oy);

        // 发送给下位机 STM32
        Serial1.print(track_cmd);
        // 本地调试打印
        Serial.printf("[Track Sent] %s", track_cmd);
    }

    // 控制视觉帧率在 15~20 FPS
    delay(60);
}
