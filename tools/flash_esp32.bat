@echo off
chcp 65001 > nul
echo =======================================================
echo 《复苏计划》- ESP32-S3 大脑固件一键烧录脚本 (esptool)
echo =======================================================
echo.

set /p COM_PORT="请输入 ESP32-S3 对应的串口号 (例如 COM3): "
if "%COM_PORT%"=="" (
    echo [错误] 串口号不能为空！
    pause
    exit /b 1
)

echo.
echo 准备向 %COM_PORT% 烧录固件...
echo 芯片型号: ESP32-S3 (N16R8, 16MB Flash)
echo.

python -m esptool --chip esp32s3 -p %COM_PORT% -b 921600 --before default_reset --after hard_reset write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m 0x0 bootloader.bin 0x8000 partition-table.bin 0x10000 firmware.bin

if %errorlevel% neq 0 (
    echo.
    echo [烧录失败] 请检查：
    echo 1. 串口号是否正确，是否被串口监视器占用；
    echo 2. 是否安装了 esptool (pip install esptool)；
    echo 3. 固件文件 bootloader.bin / firmware.bin 是否放在当前目录下。
) else (
    echo.
    echo [烧录成功！] ESP32-S3 已自动重启，请检查串口或 OLED 表情屏幕！
)

echo.
pause
