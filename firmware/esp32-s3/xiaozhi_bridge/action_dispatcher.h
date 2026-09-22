/**
 ******************************************************************************
 * @file           : action_dispatcher.h
 * @brief          : 小智 AI 语音大模型意图转 STM32 动作指令调度器
 ******************************************************************************
 */

#ifndef __ACTION_DISPATCHER_H
#define __ACTION_DISPATCHER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化身脑串口调度器 (配置 GPIO 43 为 TX, GPIO 44 为 RX, 波特率 115200)
 */
void action_dispatcher_init(void);

/**
 * @brief 根据大模型返回的动作意图名称与参数，组装并向 STM32 发送动作帧
 * @param action_name 如 "FORWARD", "BACK", "TURN_L", "TURN_R", "HELLO", "SIT", "STAND"
 * @param step_or_val 步数或动作参数 (如 1~10)
 */
void action_dispatcher_send_action(const char* action_name, int step_or_val);

/**
 * @brief 直接发送人脸追踪偏移坐标
 * @param offset_x 归一化水平偏差 (-100 ~ 100)
 * @param offset_y 归一化垂直偏差 (-100 ~ 100)
 */
void action_dispatcher_send_track(int offset_x, int offset_y);

/**
 * @brief 简单的自然语言意图关键词匹配解析（可作为端侧兜底逻辑）
 * @param text 大模型输出或 ASR 识别出的文本 (如 "向前走两步", "趴下", "打招呼")
 * @return 是否成功识别并发送了动作
 */
bool action_dispatcher_parse_natural_language(const char* text);

/**
 * @brief 接收并检查 STM32 回传的状态帧 ($STA,OK# / $STA,DONE# 等)
 * @param out_msg 输出接收到的消息内容
 * @param max_len 缓冲区最大长度
 * @return 是否收到了有效应答
 */
bool action_dispatcher_check_feedback(char* out_msg, int max_len);

#ifdef __cplusplus
}
#endif

#endif /* __ACTION_DISPATCHER_H */
