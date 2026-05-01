@echo off
setlocal enabledelayedexpansion

REM ========================================
REM ESP32-C6 Matter Fan - Docker Build & Flash Script
REM ========================================
REM Script này giúp build và flash firmware vào ESP32-C6
REM sử dụng Docker để tránh lỗi độ dài đường dẫn trong Windows.
REM
REM Sử dụng:
REM   esp_build.bat          - Build, Flash và Monitor
REM   esp_build.bat build    - Chỉ build
REM   esp_build.bat flash    - Chỉ flash
REM   esp_build.bat monitor  - Chỉ xem log serial
REM ========================================

cd /d "%~dp0"

REM Kiểm tra Docker có đang chạy không
docker info >nul 2>&1
if %errorlevel% neq 0 (
    echo.
    echo ========================================
    echo ❌ LỖI: Docker không đang chạy!
    echo ========================================
    echo.
    echo Vui lòng khởi động Docker Desktop và thử lại.
    echo.
    pause
    exit /b 1
)

REM Hàm xử lý build
:do_build
echo.
echo ========================================
echo [1/2] BUILDING FIRMWARE...
echo ========================================
docker-compose run --rm build
if %errorlevel% neq 0 (
    echo.
    echo ❌ BUILD FAILED!
    echo Vui lòng kiểm tra lỗi và thử lại.
    echo.
    pause
    exit /b 1
)
echo.
echo ✅ BUILD SUCCESSFUL!
echo.
echo Firmware: build\esp32_c6_matter_fan.bin
echo.
goto :end

REM Hàm xử lý flash (sử dụng esptool.exe trực tiếp trên Windows)
:do_flash
echo.
echo ========================================
echo [1/1] FLASHING TO ESP32-C6 (COM3)...
echo ========================================
if not exist "esptool_bin\esptool-win64\esptool.exe" (
    echo.
    echo ❌ LỖI: Không tìm thấy esptool.exe!
    echo Vui lòng đảm bảo thư mục esptool_bin tồn tại.
    echo.
    pause
    exit /b 1
)
esptool_bin\esptool-win64\esptool.exe ^
    --port COM3 ^
    --baud 921600 ^
    write_flash ^
    0x00000000 build\esp32_c6_matter_fan.bin
if %errorlevel% neq 0 (
    echo.
    echo ❌ FLASH FAILED!
    echo.
    echo Các nguyên nhân có thể:
    echo   - ESP32 không được kết nối
    echo   - Cổng COM3 không tồn tại
    echo   - Driver CH340/CP2102 chưa cài
    echo.
    pause
    exit /b 1
)
echo.
echo ✅ FLASH SUCCESSFUL!
echo.
goto :end

REM Hàm xử lý monitor (sử dụng screen từ Python)
:do_monitor
echo.
echo ========================================
echo SERIAL MONITOR (COM3) - 115200 baud
echo ========================================
echo Nhấn Ctrl+C để thoát
echo.
python -m serial.tools.miniterm --port COM3 --baud 115200
goto :end

REM Hàm xử lý clean
:do_clean
echo.
echo ========================================
echo CLEANING BUILD...
echo ========================================
docker-compose run --rm clean
echo.
echo ✅ CLEAN SUCCESSFUL!
echo.
goto :end

REM Xử lý tham số dòng lệnh
if "%~1"=="build" goto :do_build
if "%~1"=="flash" goto :do_flash
if "%~1"=="monitor" goto :do_monitor
if "%~1"=="clean" goto :do_clean
if "%~1"=="fullflash" (
    echo.
    echo ========================================
    echo FULL FLASH (Build + Flash)
    echo ========================================
    call :do_build
    if %errorlevel% neq 0 (
        pause
        exit /b 1
    )
    call :do_flash
    if %errorlevel% neq 0 (
        pause
        exit /b 1
    )
    echo.
    echo ✅ FULL FLASH SUCCESSFUL!
    echo.
    goto :end
)

REM Mặc định: Build + Flash + Monitor
:default_action
echo.
echo ========================================
echo ESP32-C6 SMART FAN - DOCKER BUILD
echo ========================================
echo.

REM Bước 1: Build
echo [1/3] Building firmware...
call :do_build
if %errorlevel% neq 0 (
    pause
    exit /b 1
)

echo ========================================
echo.

REM Bước 2: Flash
echo [2/3] Flashing to ESP32-C6 (COM3)...
echo.
set /p confirm="Nhấn Enter để flash vào ESP32... (hoặc nhập 'n' để thoát)"
if /i "!confirm!"=="n" (
    echo.
    echo Bỏ qua flash.
    goto :end
)
call :do_flash
if %errorlevel% neq 0 (
    pause
    exit /b 1
)

echo ========================================
echo.

REM Bước 3: Monitor
echo [3/3] Opening serial monitor...
echo.
set /p confirm="Nhấn Enter để mở serial monitor... (hoặc nhập 'n' để thoát)"
if /i "!confirm!"=="n" (
    echo.
    echo Bỏ qua monitor.
    goto :end
)
call :do_monitor

:end
echo.
echo ========================================
echo DONE
echo ========================================
echo.
pause
