# STM32F103 身体控制下位机固件与烧录指南

本目录包含《复苏计划》四足机器人下位机（身体）的完整控制源码。  
负责 4 路 SG90 舵机 50Hz 硬件 PWM 输出、步态加减速插值防扫齿、身脑串口通信解析与对角小跑/原地自转步态生成。

---

## 🛠️ 编译与烧录方式

### 方式 A：使用 VSCode + PlatformIO（推荐）
1. 安装 **VSCode** 并安装 **PlatformIO IDE** 插件；
2. 在 VSCode 中打开本目录 `firmware/stm32f103/`；
3. 将 ST-Link V2 插上电脑 USB，并将 4 根线连接至核心板 SWD 接口：
   * `3.3V` -> `3.3V`
   * `GND` -> `GND`
   * `SWDIO` -> `SWDIO`
   * `SWCLK` -> `SWCLK`
4. 点击 PlatformIO 底栏的 **Build (编译)** 图标（对勾）；
5. 点击 **Upload (下载)** 图标（向右箭头），固件将自动通过 ST-Link 烧录入芯片并运行。

---

### 方式 B：使用 Keil MDK (uVision5)
1. 打开 Keil MDK，新建或打开工程并选择目标芯片为 `STM32F103C8`；
2. 将 `Core/Src/` 下的 `main.c`、`servo.c`、`gait.c`、`uart_comm.c` 以及 HAL 库文件添加至工程；
3. 将 `Core/Inc/` 包含到头文件搜索路径中；
4. 在 Project Options -> Debug 中选择 **ST-Link Debugger**；
5. 点击 **Rebuild** 编译，点击 **Download**（F8）烧录。

---

### 方式 C：使用命令行一键烧录
如果你已有编译好的二进制文件 `firmware.bin`，直接进入仓库根目录下的 `tools/` 运行：
```cmd
tools\flash_stm32.bat
```

---

## 🔌 硬件引脚分配表

| 信号名称 | STM32 引脚 | 定时器/外设通道 | 说明 |
|---|---|---|---|
| **前左腿 (FL) 舵机** | **PA0** | TIM2_CH1 | 输出 50Hz PWM，接 SG90 黄色信号线 |
| **前右腿 (FR) 舵机** | **PA1** | TIM2_CH2 | 输出 50Hz PWM，接 SG90 黄色信号线 |
| **后左腿 (BL) 舵机** | **PA2** | TIM2_CH3 | 输出 50Hz PWM，接 SG90 黄色信号线 |
| **后右腿 (BR) 舵机** | **PA3** | TIM2_CH4 | 输出 50Hz PWM，接 SG90 黄色信号线 |
| **身脑串口 TX** | **PA9** | USART1_TX | 接 ESP32-S3 GPIO 44 (U0RXD) |
| **身脑串口 RX** | **PA10** | USART1_RX | 接 ESP32-S3 GPIO 43 (U0TXD) |
| **板载状态指示灯** | **PC13** | GPIO | 500ms 心跳闪烁 |

---

## 📡 串口调试自测命令 (波特率 115200)

用 USB 转串口模块接 PA9/PA10，通过串口助手发送以下 ASCII 文本即可手动触发动作测试：
* `$ACT,STAND#` -> 小狗四腿归位到 90 度标称中位；
* `$ACT,FORWARD,5#` -> 小狗向前对角小跑 5 步；
* `$ACT,BACK,3#` -> 小狗向后退 3 步；
* `$ACT,TURN_L,4#` -> 小狗原地差动踏步左转；
* `$ACT,TURN_R,4#` -> 小狗原地差动踏步右转；
* `$ACT,HELLO#` -> 小狗前左腿抬起欢快招手；
* `$ACT,SIT#` -> 小狗后腿蹲下坐立；
* `$TRACK,30,-10#` -> 模拟上位机视觉追踪偏差输入。
