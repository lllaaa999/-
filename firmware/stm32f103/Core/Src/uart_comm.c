/**
 ******************************************************************************
 * @file           : uart_comm.c
 * @brief          : 身脑协议解析与 USART1 通信管理
 ******************************************************************************
 */

#include "uart_comm.h"
#include "gait.h"
#include <stdlib.h>
#include <string.h>

static uint8_t  s_rx_char;
static char     s_frame_buf[RX_BUFFER_SIZE];
static uint8_t  s_frame_idx = 0;
static bool     s_in_frame  = false;

void UART_Comm_Init(void) {
    s_frame_idx = 0;
    s_in_frame  = false;
    // 启动中断单字节接收
    HAL_UART_Receive_IT(&huart1, &s_rx_char, 1);
}

void UART_Comm_OnRxByte(uint8_t byte) {
    if (byte == '$') {
        s_in_frame = true;
        s_frame_idx = 0;
        s_frame_buf[s_frame_idx++] = byte;
    } else if (s_in_frame) {
        if (s_frame_idx < RX_BUFFER_SIZE - 1) {
            s_frame_buf[s_frame_idx++] = (char)byte;
            if (byte == '#') {
                s_frame_buf[s_frame_idx] = '\0';
                s_in_frame = false;
                // 标记已接收完整一帧，由主循环提取解析
            }
        } else {
            // 缓冲区溢出保护
            s_in_frame = false;
            s_frame_idx = 0;
        }
    }
}

// HAL 库串口接收完成中断回调
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        UART_Comm_OnRxByte(s_rx_char);
        // 继续挂起下一次接收中断
        HAL_UART_Receive_IT(&huart1, &s_rx_char, 1);
    }
}

/**
 * @brief  解析单条完整的身脑协议帧
 */
static void Parse_Frame(char* frame) {
    // 校验合法性：必须以 '$' 开头，以 '#' 结尾
    int len = strlen(frame);
    if (len < 4 || frame[0] != '$' || frame[len - 1] != '#') {
        return;
    }

    // 去掉尾部的 '#'
    frame[len - 1] = '\0';

    // 格式 1: $ACT,动作名称[,步数]
    if (strncmp(frame, "$ACT,", 5) == 0) {
        char* token = strtok(frame + 5, ",");
        if (token == NULL) return;

        if (strcmp(token, "STAND") == 0) {
            Gait_ExecuteCommand(GAIT_STATE_STAND, 0);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "FORWARD") == 0) {
            char* step_str = strtok(NULL, ",");
            uint8_t steps = step_str ? (uint8_t)atoi(step_str) : 4;
            Gait_ExecuteCommand(GAIT_STATE_FORWARD, steps);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "BACK") == 0) {
            char* step_str = strtok(NULL, ",");
            uint8_t steps = step_str ? (uint8_t)atoi(step_str) : 4;
            Gait_ExecuteCommand(GAIT_STATE_BACK, steps);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "TURN_L") == 0) {
            char* step_str = strtok(NULL, ",");
            uint8_t steps = step_str ? (uint8_t)atoi(step_str) : 3;
            Gait_ExecuteCommand(GAIT_STATE_TURN_L, steps);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "TURN_R") == 0) {
            char* step_str = strtok(NULL, ",");
            uint8_t steps = step_str ? (uint8_t)atoi(step_str) : 3;
            Gait_ExecuteCommand(GAIT_STATE_TURN_R, steps);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "HELLO") == 0) {
            Gait_ExecuteCommand(GAIT_STATE_HELLO, 0);
            UART_Comm_Send("$STA,OK#\n");
        } else if (strcmp(token, "SIT") == 0) {
            Gait_ExecuteCommand(GAIT_STATE_SIT, 0);
            UART_Comm_Send("$STA,OK#\n");
        }
    }
    // 格式 2: $TRACK,offset_x,offset_y
    else if (strncmp(frame, "$TRACK,", 7) == 0) {
        char* x_str = strtok(frame + 7, ",");
        char* y_str = strtok(NULL, ",");
        if (x_str && y_str) {
            int8_t ox = (int8_t)atoi(x_str);
            int8_t oy = (int8_t)atoi(y_str);
            Gait_UpdateTrackOffset(ox, oy);
            // 视线追踪为高频控制流，不强制回传 STA 避免占用总线
        }
    }
}

void UART_Comm_Process(void) {
    if (!s_in_frame && s_frame_idx > 0 && s_frame_buf[s_frame_idx - 1] == '#') {
        Parse_Frame(s_frame_buf);
        s_frame_idx = 0;
    }
}

void UART_Comm_Send(const char* msg) {
    if (msg) {
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
}
