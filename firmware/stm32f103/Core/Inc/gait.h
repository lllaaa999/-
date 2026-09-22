/**
 ******************************************************************************
 * @file           : gait.h
 * @brief          : 四足步态发生器与视线追踪姿态解算引擎
 ******************************************************************************
 */

#ifndef __GAIT_H
#define __GAIT_H

#include "main.h"
#include "servo.h"

/* 步态指令枚举 */
typedef enum {
    GAIT_STATE_IDLE = 0,    /* 空闲停歇 */
    GAIT_STATE_STAND,       /* 90度标称站立 */
    GAIT_STATE_FORWARD,     /* 前进行走 */
    GAIT_STATE_BACK,        /* 后退行走 */
    GAIT_STATE_TURN_L,      /* 对角踏步左转 */
    GAIT_STATE_TURN_R,      /* 对角踏步右转 */
    GAIT_STATE_HELLO,       /* 抬腿招手打招呼 */
    GAIT_STATE_SIT,         /* 坐下休息姿态 */
    GAIT_STATE_TRACKING     /* 视线闭环跟随微调 */
} GaitState_t;

/**
 * @brief  初始化步态引擎并将四足归位到初始站立姿态
 */
void Gait_Init(void);

/**
 * @brief  触发执行步态动作
 * @param  state 动作类型 (前进、后退、转弯、打招呼等)
 * @param  steps 步数 (对于前进/后退/转弯有效，如 1~20 步)
 */
void Gait_ExecuteCommand(GaitState_t state, uint8_t steps);

/**
 * @brief  更新视线追踪偏差输入（闭环跟随）
 * @param  offset_x 画面水平偏差 (-100 ~ 100)
 * @param  offset_y 画面垂直偏差 (-100 ~ 100)
 */
void Gait_UpdateTrackOffset(int8_t offset_x, int8_t offset_y);

/**
 * @brief  步态引擎周期性轮询更新（建议在 20ms 定时器中调用）
 */
void Gait_Update(void);

/**
 * @brief  查询当前步态动作是否已全部完成
 */
bool Gait_IsActionCompleted(void);

/**
 * @brief  获取当前步态引擎状态
 */
GaitState_t Gait_GetState(void);

#endif /* __GAIT_H */
