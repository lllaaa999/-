/**
 ******************************************************************************
 * @file           : servo.h
 * @brief          : 4路 SG90 舵机硬件 PWM 驱动与防扫齿平滑插值引擎
 ******************************************************************************
 */

#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

#define SERVO_COUNT        4

/* 舵机编号枚举 */
typedef enum {
    SERVO_FL = 0,   /* 前左腿 Front Left  - PA0 (TIM2_CH1) */
    SERVO_FR = 1,   /* 前右腿 Front Right - PA1 (TIM2_CH2) */
    SERVO_BL = 2,   /* 后左腿 Back Left   - PA2 (TIM2_CH3) */
    SERVO_BR = 3    /* 后右腿 Back Right  - PA3 (TIM2_CH4) */
} ServoID_t;

/* SG90 舵机物理限位与脉宽 (单位: 微秒 us) */
#define SERVO_MIN_US       500      /* 0 度对应高电平脉宽 */
#define SERVO_MID_US       1500     /* 90 度中位脉宽 */
#define SERVO_MAX_US       2500     /* 180 度对应高电平脉宽 */
#define SERVO_MIN_ANGLE    0.0f
#define SERVO_MAX_ANGLE    180.0f

/* 防扫齿平滑插值步进限制 (单位: 度/周期，在20ms控制周期中步进最大2度，防止齿轮刚性冲击) */
#define SERVO_MAX_STEP_DEG 2.5f

/**
 * @brief  初始化 4 路舵机 PWM 定时器并使能通道
 */
void Servo_Init(void);

/**
 * @brief  设置指定舵机的目标角度（平滑过渡模式）
 * @param  id 舵机索引 (SERVO_FL, SERVO_FR, SERVO_BL, SERVO_BR)
 * @param  angle 目标角度 (0.0 ~ 180.0)
 */
void Servo_SetTargetAngle(ServoID_t id, float angle);

/**
 * @brief  同时设置 4 腿舵机目标角度
 */
void Servo_SetTargetAngleAll(float fl, float fr, float bl, float br);

/**
 * @brief  立即设置舵机角度（无缓动插值，通常仅在初始零点校准时使用）
 */
void Servo_SetAngleImmediate(ServoID_t id, float angle);

/**
 * @brief  舵机平滑更新心跳函数（需在 20ms 主循环或定时器中周期调用）
 *         使用线性插值逼近目标角度，有效规避 SG90 塑料齿轮起步扫齿
 */
void Servo_UpdateSmooth(void);

/**
 * @brief  获取当前舵机实际瞬态角度
 */
float Servo_GetCurrentAngle(ServoID_t id);

/**
 * @brief  检查当前所有舵机是否已平滑移动到目标位置
 * @return true: 全部到位, false: 仍在过渡中
 */
bool Servo_IsAllArrived(void);

#endif /* __SERVO_H */
