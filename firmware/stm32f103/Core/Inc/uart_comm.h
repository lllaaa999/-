/**
 ******************************************************************************
 * @file           : uart_comm.h
 * @brief          : 身脑协同 UART 通信协议解析与应答器
 ******************************************************************************
 */

#ifndef __UART_COMM_H
#define __UART_COMM_H

#include "main.h"

#define RX_BUFFER_SIZE     128
#define TX_BUFFER_SIZE     128

/**
 * @brief  初始化身脑协同串口通信
 */
void UART_Comm_Init(void);

/**
 * @brief  串口字节接收中断回调入口
 * @param  byte 接收到的单字节
 */
void UART_Comm_OnRxByte(uint8_t byte);

/**
 * @brief  串口协议帧解析主处理函数（在主循环中轮询调用）
 */
void UART_Comm_Process(void);

/**
 * @brief  向上位机 ESP32-S3 发送状态/事件响应帧
 * @param  msg 响应内容，如 "$STA,OK#\n", "$STA,DONE#\n"
 */
void UART_Comm_Send(const char* msg);

#endif /* __UART_COMM_H */
