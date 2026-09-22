/**
 ******************************************************************************
 * @file           : action_dispatcher.cpp
 * @brief          : 小智 AI 语音大模型意图转 STM32 动作指令调度器实现
 ******************************************************************************
 */

#include "action_dispatcher.h"
#include "board_pins_config.h"
#include <stdio.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"

static const char *TAG = "ACTION_DISPATCH";
#define BODY_UART_PORT   UART_NUM_1
#define BUF_SIZE         256

void action_dispatcher_init(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BODY_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(BODY_UART_PORT, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(BODY_UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(BODY_UART_PORT, UART_BODY_TX_PIN, UART_BODY_RX_PIN, 
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "Body UART dispatcher initialized on TX:%d RX:%d @ %d bps",
             UART_BODY_TX_PIN, UART_BODY_RX_PIN, UART_BODY_BAUDRATE);
}

void action_dispatcher_send_action(const char* action_name, int step_or_val) {
    if (!action_name) return;

    char cmd[64];
    if (step_or_val > 0) {
        snprintf(cmd, sizeof(cmd), "$ACT,%s,%d#\n", action_name, step_or_val);
    } else {
        snprintf(cmd, sizeof(cmd), "$ACT,%s#\n", action_name);
    }

    uart_write_bytes(BODY_UART_PORT, cmd, strlen(cmd));
    ESP_LOGI(TAG, "Sent to STM32: %s", cmd);
}

void action_dispatcher_send_track(int offset_x, int offset_y) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "$TRACK,%d,%d#\n", offset_x, offset_y);
    uart_write_bytes(BODY_UART_PORT, cmd, strlen(cmd));
}

bool action_dispatcher_parse_natural_language(const char* text) {
    if (!text) return false;

    // 前进意图
    if (strstr(text, "向前") || strstr(text, "往前") || strstr(text, "走两步") || strstr(text, "前进")) {
        action_dispatcher_send_action("FORWARD", 3);
        return true;
    }
    // 后退意图
    if (strstr(text, "后退") || strstr(text, "往后") || strstr(text, "退后")) {
        action_dispatcher_send_action("BACK", 3);
        return true;
    }
    // 左转意图
    if (strstr(text, "左转") || strstr(text, "向左")) {
        action_dispatcher_send_action("TURN_L", 3);
        return true;
    }
    // 右转意图
    if (strstr(text, "右转") || strstr(text, "向右")) {
        action_dispatcher_send_action("TURN_R", 3);
        return true;
    }
    // 打招呼 / 握手意图
    if (strstr(text, "打招呼") || strstr(text, "握手") || strstr(text, "招手") || strstr(text, "你好")) {
        action_dispatcher_send_action("HELLO", 0);
        return true;
    }
    // 坐下 / 趴下意图
    if (strstr(text, "坐下") || strstr(text, "趴下") || strstr(text, "休息")) {
        action_dispatcher_send_action("SIT", 0);
        return true;
    }
    // 站立 / 立正意图
    if (strstr(text, "站立") || strstr(text, "起来") || strstr(text, "立正")) {
        action_dispatcher_send_action("STAND", 0);
        return true;
    }

    return false;
}

bool action_dispatcher_check_feedback(char* out_msg, int max_len) {
    int length = 0;
    ESP_ERROR_CHECK(uart_get_buffered_data_len(BODY_UART_PORT, (size_t*)&length));
    if (length > 0) {
        int read_bytes = uart_read_bytes(BODY_UART_PORT, (uint8_t*)out_msg, max_len - 1, 10 / portTICK_PERIOD_MS);
        if (read_bytes > 0) {
            out_msg[read_bytes] = '\0';
            ESP_LOGI(TAG, "Received feedback from STM32: %s", out_msg);
            return true;
        }
    }
    return false;
}
