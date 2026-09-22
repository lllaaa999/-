/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : 《复苏计划》身体下位机主程序入口
 * @description    : 负责硬件外设初始化、身脑串口通信调度、四足步态周期驱动
 ******************************************************************************
 */

#include "main.h"
#include "servo.h"
#include "gait.h"
#include "uart_comm.h"

UART_HandleTypeDef huart1;
TIM_HandleTypeDef  htim2;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART1_UART_Init(void);

int main(void) {
    /* 初始化 HAL 库并配置系统滴答定时器 */
    HAL_Init();

    /* 配置系统时钟为 72MHz (外部 8MHz 晶振倍频) */
    SystemClock_Config();

    /* 初始化所有配置的外设 */
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART1_UART_Init();

    /* 初始化 4 路 SG90 舵机系统 (默认站立) */
    Servo_Init();

    /* 初始化四足步态发生器 */
    Gait_Init();

    /* 启动身脑协同串口通信接收中断 */
    UART_Comm_Init();

    // 发送开机握手广播
    UART_Comm_Send("$BOOT,RECOVERY_DOG_V1#\n");

    uint32_t last_gait_tick = 0;
    uint32_t last_led_tick  = 0;

    /* 主调度循环 */
    while (1) {
        uint32_t now = HAL_GetTick();

        // 1. 串口协议帧实时解析
        UART_Comm_Process();

        // 2. 20ms 周期控制节拍 (50Hz 控制环路)
        if (now - last_gait_tick >= 20) {
            last_gait_tick = now;

            // 步态引擎解算与状态机流转
            Gait_Update();

            // 舵机平滑线性插值更新 (防扫齿保护)
            Servo_UpdateSmooth();
        }

        // 3. 500ms 板载 LED 心跳闪烁，指示系统健康运行
        if (now - last_led_tick >= 500) {
            last_led_tick = now;
            HAL_GPIO_TogglePin(LED_GPIO_PORT, LED_PIN);
        }
    }
}

/**
 * @brief 系统时钟配置 (72MHz)
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState       = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL     = RCC_PLL_MUL9; // 8MHz * 9 = 72MHz
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                      |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief TIM2 初始化: 4路 PWM 发生器 (50Hz 周期 = 20ms)
 *        72MHz / (71 + 1) = 1MHz 计数频率 (1us / tick)
 *        ARR = 19999 (20000us = 20ms)
 */
static void MX_TIM2_Init(void) {
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler         = 71;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 19999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK) {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK) {
        Error_Handler();
    }
    
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 1500; // 初始 90 度脉宽 1.5ms
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;

    // 配置 4 个通道
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3);
    HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4);
}

/**
 * @brief USART1 初始化 (PA9=TX, PA10=RX, 115200 8-N-1)
 */
static void MX_USART1_UART_Init(void) {
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 115200;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief GPIO 初始化
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 使能 GPIOA, GPIOC 时钟 */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_USART1_CLK_ENABLE();

    /* 配置板载 LED 引脚 (PC13) */
    HAL_GPIO_WritePin(LED_GPIO_PORT, LED_PIN, GPIO_PIN_SET);
    GPIO_InitStruct.Pin   = LED_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED_GPIO_PORT, &GPIO_InitStruct);

    /* 配置 PA0~PA3 为复用推挽输出 (TIM2 CH1~CH4) */
    GPIO_InitStruct.Pin   = SERVO_FL_PIN | SERVO_FR_PIN | SERVO_BL_PIN | SERVO_BR_PIN;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置 PA9(TX) 为复用推挽, PA10(RX) 为浮空输入 */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 配置 USART1 中断优先级并使能 */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

void SysTick_Handler(void) {
    HAL_IncTick();
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {
    }
}
