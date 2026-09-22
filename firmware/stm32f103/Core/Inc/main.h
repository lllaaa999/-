/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : 《复苏计划》身体控制板核心配置头文件
 * @mcu            : STM32F103C8T6 (72MHz, Cortex-M3)
 ******************************************************************************
 */

#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* 引脚定义：4路 SG90 舵机 PWM (TIM2 CH1~CH4) */
#define SERVO_FL_PIN         GPIO_PIN_0
#define SERVO_FL_GPIO_PORT   GPIOA
#define SERVO_FR_PIN         GPIO_PIN_1
#define SERVO_FR_GPIO_PORT   GPIOA
#define SERVO_BL_PIN         GPIO_PIN_2
#define SERVO_BL_GPIO_PORT   GPIOA
#define SERVO_BR_PIN         GPIO_PIN_3
#define SERVO_BR_GPIO_PORT   GPIOA

/* 引脚定义：板载状态 LED (BluePill 典型为 PC13，低电平点亮) */
#define LED_PIN              GPIO_PIN_13
#define LED_GPIO_PORT        GPIOC

/* 身脑协同串口定义 (USART1: PA9=TX, PA10=RX, 115200 8-N-1) */
#define COMM_UART            huart1
extern UART_HandleTypeDef   huart1;
extern TIM_HandleTypeDef    htim2;

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
