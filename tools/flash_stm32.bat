@echo off
chcp 65001 > nul
echo =======================================================
echo 《复苏计划》- STM32F103C8T6 下位机 ST-Link 烧录脚本
echo =======================================================
echo.

set TARGET_BIN=..\firmware\stm32f103\.pio\build\bluepill_f103c8\firmware.bin
if not exist "%TARGET_BIN%" (
    set TARGET_BIN=firmware.bin
)

echo 目标固件路径: %TARGET_BIN%
echo.

where STM32_Programmer_CLI >nul 2>nul
if %errorlevel% equ 0 (
    echo [检测到 STM32CubeProgrammer CLI] 正在通过 ST-Link 烧录...
    STM32_Programmer_CLI -c port=SWD mode=UR -w "%TARGET_BIN%" 0x08000000 -v -rst
    goto END
)

where ST-LINK_CLI >nul 2>nul
if %errorlevel% equ 0 (
    echo [检测到 ST-LINK Utility CLI] 正在通过 ST-Link 烧录...
    ST-LINK_CLI -c SWD -P "%TARGET_BIN%" 0x08000000 -V -Rst
    goto END
)

echo [提示] 系统未检测到全局 STM32_Programmer_CLI 或 ST-LINK_CLI。
echo 推荐使用以下方式一键编译烧录：
echo 1. 在 VSCode 中打开 firmware\stm32f103，点击 PlatformIO 底部的 Upload (上传) 按钮；
echo 2. 在 Keil MDK 中打开工程，点击 Download 按钮 (F8)；
echo 3. 安装 STM32CubeProgrammer 官方上位机软件，选择固件后点击 Download。

:END
echo.
pause
