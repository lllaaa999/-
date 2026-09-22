/**
 ******************************************************************************
 * @file           : gait.c
 * @brief          : 四足步态发生器、动作机及视线跟随姿态生成
 ******************************************************************************
 */

#include "gait.h"
#include "uart_comm.h"
#include <math.h>

#define PI 3.1415926535f

static GaitState_t s_current_state = GAIT_STATE_STAND;
static uint8_t     s_remaining_steps = 0;
static uint32_t    s_tick_counter = 0;
static bool        s_action_completed = true;

/* 追踪控制偏差变量 */
static int8_t      s_track_offset_x = 0;
static int8_t      s_track_offset_y = 0;
static uint32_t    s_track_cooldown = 0;

/* 步态摆幅常数 */
#define WALK_AMP       22.0f    /* 正常行走对角摆幅 (度) */
#define TURN_AMP       25.0f    /* 原地转向摆幅 (度) */
#define GAIT_PERIOD    30       /* 一个完整步态周期的 Tick 数 (每Tick约20ms，即600ms一步) */

void Gait_Init(void) {
    s_current_state = GAIT_STATE_STAND;
    s_remaining_steps = 0;
    s_tick_counter = 0;
    s_action_completed = true;
    Servo_SetTargetAngleAll(90.0f, 90.0f, 90.0f, 90.0f);
}

void Gait_ExecuteCommand(GaitState_t state, uint8_t steps) {
    s_current_state = state;
    s_remaining_steps = (steps > 0) ? steps : 1;
    s_tick_counter = 0;
    s_action_completed = false;
}

void Gait_UpdateTrackOffset(int8_t offset_x, int8_t offset_y) {
    s_track_offset_x = offset_x;
    s_track_offset_y = offset_y;
    s_current_state = GAIT_STATE_TRACKING;
}

void Gait_Update(void) {
    s_tick_counter++;

    switch (s_current_state) {
        case GAIT_STATE_STAND:
            Servo_SetTargetAngleAll(90.0f, 90.0f, 90.0f, 90.0f);
            if (Servo_IsAllArrived() && !s_action_completed) {
                s_action_completed = true;
                UART_Comm_Send("$STA,DONE#\n");
            }
            break;

        case GAIT_STATE_FORWARD: {
            // 对角小跑 (Trot) 步态解算
            float phase = (float)(s_tick_counter % GAIT_PERIOD) / (float)GAIT_PERIOD * 2.0f * PI;
            float wave  = sinf(phase) * WALK_AMP;

            // 对角支撑组 1: FL & BR; 对角支撑组 2: FR & BL
            float fl = 90.0f + wave;
            float fr = 90.0f + wave;  // 镜像装配方向补偿
            float bl = 90.0f - wave;
            float br = 90.0f - wave;

            Servo_SetTargetAngleAll(fl, fr, bl, br);

            if ((s_tick_counter % GAIT_PERIOD) == 0) {
                if (s_remaining_steps > 0) {
                    s_remaining_steps--;
                }
                if (s_remaining_steps == 0) {
                    s_current_state = GAIT_STATE_STAND;
                    UART_Comm_Send("$STA,DONE#\n");
                }
            }
            break;
        }

        case GAIT_STATE_BACK: {
            float phase = (float)(s_tick_counter % GAIT_PERIOD) / (float)GAIT_PERIOD * 2.0f * PI;
            float wave  = sinf(phase) * WALK_AMP;

            float fl = 90.0f - wave;
            float fr = 90.0f - wave;
            float bl = 90.0f + wave;
            float br = 90.0f + wave;

            Servo_SetTargetAngleAll(fl, fr, bl, br);

            if ((s_tick_counter % GAIT_PERIOD) == 0) {
                if (s_remaining_steps > 0) {
                    s_remaining_steps--;
                }
                if (s_remaining_steps == 0) {
                    s_current_state = GAIT_STATE_STAND;
                    UART_Comm_Send("$STA,DONE#\n");
                }
            }
            break;
        }

        case GAIT_STATE_TURN_L: {
            // 原地差动踏步左转
            float phase = (float)(s_tick_counter % GAIT_PERIOD) / (float)GAIT_PERIOD * 2.0f * PI;
            float wave  = sinf(phase) * TURN_AMP;

            float fl = 90.0f - wave;
            float fr = 90.0f + wave;
            float bl = 90.0f - wave;
            float br = 90.0f + wave;

            Servo_SetTargetAngleAll(fl, fr, bl, br);

            if ((s_tick_counter % GAIT_PERIOD) == 0) {
                if (s_remaining_steps > 0) {
                    s_remaining_steps--;
                }
                if (s_remaining_steps == 0) {
                    s_current_state = GAIT_STATE_STAND;
                    UART_Comm_Send("$STA,DONE#\n");
                }
            }
            break;
        }

        case GAIT_STATE_TURN_R: {
            // 原地差动踏步右转
            float phase = (float)(s_tick_counter % GAIT_PERIOD) / (float)GAIT_PERIOD * 2.0f * PI;
            float wave  = sinf(phase) * TURN_AMP;

            float fl = 90.0f + wave;
            float fr = 90.0f - wave;
            float bl = 90.0f + wave;
            float br = 90.0f - wave;

            Servo_SetTargetAngleAll(fl, fr, bl, br);

            if ((s_tick_counter % GAIT_PERIOD) == 0) {
                if (s_remaining_steps > 0) {
                    s_remaining_steps--;
                }
                if (s_remaining_steps == 0) {
                    s_current_state = GAIT_STATE_STAND;
                    UART_Comm_Send("$STA,DONE#\n");
                }
            }
            break;
        }

        case GAIT_STATE_HELLO: {
            // 前左腿抬起招手动作 (3条腿三角稳固支撑)
            // FR, BL, BR 保持稳定站立
            Servo_SetTargetAngle(SERVO_FR, 95.0f);
            Servo_SetTargetAngle(SERVO_BL, 85.0f);
            Servo_SetTargetAngle(SERVO_BR, 85.0f);

            // FL 抬高并在 120~150 度之间欢快摇摆
            float wave = sinf((float)s_tick_counter * 0.4f) * 20.0f;
            Servo_SetTargetAngle(SERVO_FL, 135.0f + wave);

            if (s_tick_counter >= 80) { // 约 1.6 秒招手结束
                s_current_state = GAIT_STATE_STAND;
                UART_Comm_Send("$STA,DONE#\n");
            }
            break;
        }

        case GAIT_STATE_SIT: {
            // 坐下姿势：前腿稍挺，后腿折叠蹲下
            Servo_SetTargetAngleAll(105.0f, 105.0f, 55.0f, 55.0f);
            if (Servo_IsAllArrived() && !s_action_completed) {
                s_action_completed = true;
                UART_Comm_Send("$STA,DONE#\n");
            }
            break;
        }

        case GAIT_STATE_TRACKING: {
            // 闭环视线跟随状态
            // 1. 冷却计数防抖
            if (s_track_cooldown > 0) {
                s_track_cooldown--;
                break;
            }

            // 2. 如果水平偏差较大 (大于死区阈值 18%)，触发 1 步原地转向
            if (s_track_offset_x > 18) {
                s_current_state = GAIT_STATE_TURN_R;
                s_remaining_steps = 1;
                s_track_cooldown = GAIT_PERIOD + 5;
            } else if (s_track_offset_x < -18) {
                s_current_state = GAIT_STATE_TURN_L;
                s_remaining_steps = 1;
                s_track_cooldown = GAIT_PERIOD + 5;
            } else {
                // 3. 水平已在中央死区，进行微小的前后俯仰调整 (Pitch 微调)
                // offset_y < 0 说明目标偏上（需要抬头：前腿稍抬，后腿稍蹲）
                float pitch_adj = (float)s_track_offset_y * 0.15f;
                float fl = 90.0f - pitch_adj;
                float fr = 90.0f - pitch_adj;
                float bl = 90.0f + pitch_adj;
                float br = 90.0f + pitch_adj;
                Servo_SetTargetAngleAll(fl, fr, bl, br);
            }
            break;
        }

        default:
            break;
    }
}

bool Gait_IsActionCompleted(void) {
    return s_action_completed;
}

GaitState_t Gait_GetState(void) {
    return s_current_state;
}
