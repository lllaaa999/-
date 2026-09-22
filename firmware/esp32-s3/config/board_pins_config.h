/**
 ******************************************************************************
 * @file           : board_pins_config.h
 * @brief          : 《复苏计划》ESP32-S3-CAM 大脑引脚适配头文件 (适配小智 AI 固件)
 * @target         : ESP32-S3-CAM N16R8
 ******************************************************************************
 */

#ifndef __BOARD_PINS_CONFIG_H
#define __BOARD_PINS_CONFIG_H

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/* 1. I2S 数字音频输入 - INMP441 麦克风 (I2S0)                               */
/* ========================================================================= */
#define I2S_MIC_SD_PIN          GPIO_NUM_4      /* 串行数据输入 */
#define I2S_MIC_WS_PIN          GPIO_NUM_5      /* 采样时钟 (LRCK) */
#define I2S_MIC_SCK_PIN         GPIO_NUM_6      /* 位时钟 (BCLK) */

/* ========================================================================= */
/* 2. I2S 数字音频输出 - MAX98357A 功放 (I2S1)                               */
/* ========================================================================= */
#define I2S_SPK_DOUT_PIN        GPIO_NUM_7      /* 音频数据输出 */
#define I2S_SPK_LRCK_PIN        GPIO_NUM_1      /* 声道时钟 (LRCK) */
#define I2S_SPK_BCLK_PIN        GPIO_NUM_2      /* 位时钟 (BCLK) */

/* ========================================================================= */
/* 3. I2C 表情显示屏 - 0.96寸 SSD1306 OLED                                   */
/* ========================================================================= */
#define I2C_OLED_SCL_PIN        GPIO_NUM_8      /* I2C 时钟线 */
#define I2C_OLED_SDA_PIN        GPIO_NUM_9      /* I2C 数据线 */
#define OLED_WIDTH              128
#define OLED_HEIGHT             64

/* ========================================================================= */
/* 4. UART 身脑协同串口 - 连接 STM32F103 (USART1: PA9/PA10)                 */
/* ========================================================================= */
#define UART_BODY_TX_PIN        GPIO_NUM_43     /* 发送至 STM32 PA10 (RX) */
#define UART_BODY_RX_PIN        GPIO_NUM_44     /* 接收自 STM32 PA9  (TX) */
#define UART_BODY_BAUDRATE      115200

/* ========================================================================= */
/* 5. OV2640 DVP 摄像头引脚定义 (ESP32-S3-CAM 专有 DVP 总线)                 */
/* ========================================================================= */
#define CAM_PIN_PWDN            -1
#define CAM_PIN_RESET           -1
#define CAM_PIN_XCLK            GPIO_NUM_15
#define CAM_PIN_SIOD            GPIO_NUM_4      /* 与某些板型 I2C 复用或板载专线 */
#define CAM_PIN_SIOC            GPIO_NUM_5
#define CAM_PIN_D7              GPIO_NUM_16
#define CAM_PIN_D6              GPIO_NUM_17
#define CAM_PIN_D5              GPIO_NUM_18
#define CAM_PIN_D4              GPIO_NUM_12
#define CAM_PIN_D3              GPIO_NUM_10
#define CAM_PIN_D2              GPIO_NUM_8
#define CAM_PIN_D1              GPIO_NUM_9
#define CAM_PIN_D0              GPIO_NUM_11
#define CAM_PIN_VSYNC           GPIO_NUM_6
#define CAM_PIN_HREF            GPIO_NUM_7
#define CAM_PIN_PCLK            GPIO_NUM_13

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_PINS_CONFIG_H */
