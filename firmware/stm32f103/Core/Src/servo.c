/**
 ******************************************************************************
 * @file           : servo.c
 * @brief          : 4路 SG90 舵机硬件 PWM 输出及平滑插值实现
 ******************************************************************************
 */

#include "servo.h"
#include <math.h>

/* 舵机角度追踪变量 */
static float s_target_angles[SERVO_COUNT]  = {90.0f, 90.0f, 90.0f, 90.0f};
static float s_current_angles[SERVO_COUNT] = {90.0f, 90.0f, 90.0f, 90.0f};

/**
 * @brief  将角度值转换为 TIM2 比较寄存器 CCR 值 (500us ~ 2500us)
 */
static uint32_t Angle_To_CCR(float angle) {
    if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
    if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
    
    // 脉宽微秒 = 500 + (angle / 180) * 2000
    // 定时器时钟为 1MHz，1 计数 = 1 微秒
    return (uint32_t)(SERVO_MIN_US + (angle / 180.0f) * (SERVO_MAX_US - SERVO_MIN_US));
}

/**
 * @brief  向指定通道底层寄存器写入脉宽 CCR
 */
static void Set_Hardware_PWM(ServoID_t id, uint32_t ccr) {
    switch (id) {
        case SERVO_FL:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
            break;
        case SERVO_FR:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, ccr);
            break;
        case SERVO_BL:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, ccr);
            break;
        case SERVO_BR:
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, ccr);
            break;
        default:
            break;
    }
}

void Servo_Init(void) {
    // 开启 TIM2 的 4 个 PWM 通道输出
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);

    // 默认回位到 90 度中立零点
    for (int i = 0; i < SERVO_COUNT; i++) {
        s_target_angles[i] = 90.0f;
        s_current_angles[i] = 90.0f;
        Set_Hardware_PWM((ServoID_t)i, Angle_To_CCR(90.0f));
    }
}

void Servo_SetTargetAngle(ServoID_t id, float angle) {
    if (id < SERVO_COUNT) {
        if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
        if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
        s_target_angles[id] = angle;
    }
}

void Servo_SetTargetAngleAll(float fl, float fr, float bl, float br) {
    Servo_SetTargetAngle(SERVO_FL, fl);
    Servo_SetTargetAngle(SERVO_FR, fr);
    Servo_SetTargetAngle(SERVO_BL, bl);
    Servo_SetTargetAngle(SERVO_BR, br);
}

void Servo_SetAngleImmediate(ServoID_t id, float angle) {
    if (id < SERVO_COUNT) {
        if (angle < SERVO_MIN_ANGLE) angle = SERVO_MIN_ANGLE;
        if (angle > SERVO_MAX_ANGLE) angle = SERVO_MAX_ANGLE;
        s_target_angles[id] = angle;
        s_current_angles[id] = angle;
        Set_Hardware_PWM(id, Angle_To_CCR(angle));
    }
}

void Servo_UpdateSmooth(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        float diff = s_target_angles[i] - s_current_angles[i];
        if (fabsf(diff) > 0.1f) {
            if (fabsf(diff) <= SERVO_MAX_STEP_DEG) {
                s_current_angles[i] = s_target_angles[i];
            } else {
                s_current_angles[i] += (diff > 0.0f) ? SERVO_MAX_STEP_DEG : -SERVO_MAX_STEP_DEG;
            }
            Set_Hardware_PWM((ServoID_t)i, Angle_To_CCR(s_current_angles[i]));
        }
    }
}

float Servo_GetCurrentAngle(ServoID_t id) {
    if (id < SERVO_COUNT) {
        return s_current_angles[id];
    }
    return 90.0f;
}

bool Servo_IsAllArrived(void) {
    for (int i = 0; i < SERVO_COUNT; i++) {
        if (fabsf(s_target_angles[i] - s_current_angles[i]) > 0.5f) {
            return false;
        }
    }
    return true;
}
