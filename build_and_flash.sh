#!/bin/bash

# ============================================
# ESP32-C6 Smart Fan - Build & Flash Script
# ============================================
# Script này sẽ:
# 1. Kiểm tra ESP-IDF
# 2. Build firmware
# 3. Flash vào ESP32-C6
# 4. Mở monitor để xem log
# ============================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ESP_IDF_PATH="$HOME/esp/esp-idf"
ESP32_C6_PORT="/dev/ttyACM0"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "============================================"
echo " ESP32-C6 Smart Fan - Build & Flash"
echo "============================================"
echo ""

# ============================================
# Bước 0: Kiểm tra ESP-IDF
# ============================================
echo "[0/4] Checking ESP-IDF installation..."

if [ ! -d "$ESP_IDF_PATH" ]; then
    echo -e "${RED}ESP-IDF not found at $ESP_IDF_PATH${NC}"
    echo "Please install ESP-IDF first:"
    echo "  cd ~/esp"
    echo "  git clone --recursive --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git"
    echo "  cd esp-idf && ./install.sh esp32c6"
    echo "  source export.sh"
    exit 1
fi

if [ ! -f "$ESP_IDF_PATH/export.sh" ]; then
    echo -e "${RED}ESP-IDF export.sh not found at $ESP_IDF_PATH${NC}"
    exit 1
fi

echo -e "${GREEN}ESP-IDF found at $ESP_IDF_PATH${NC}"

# ============================================
# Bước 1: Setup Environment
# ============================================
echo ""
echo "[1/4] Setting up environment..."

source "$ESP_IDF_PATH/export.sh"

# Set ESP-Matter path
export ESP_MATTER_PATH="$HOME/esp/esp-matter"

# Set target to ESP32-C6
cd "$PROJECT_DIR"
idf.py set-target esp32c6 > /dev/null 2>&1

echo -e "${GREEN}Environment ready!${NC}"

# ============================================
# Bước 2: Build Firmware
# ============================================
echo ""
echo "[2/4] Building firmware..."

idf.py build

if [ ! -f "$PROJECT_DIR/build/esp32-c6_matter_fan.bin" ]; then
    echo -e "${RED}Build failed! Binary not found.${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful!${NC}"
echo "Firmware: $PROJECT_DIR/build/esp32-c6_matter_fan.bin"

# ============================================
# Bước 3: Flash Firmware
# ============================================
echo ""
echo "[3/4] Flashing firmware to ESP32-C6..."

# Detect serial port
if [ ! -e "$ESP32_C6_PORT" ]; then
    # Try to auto-detect
    ESP32_C6_PORT=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1)
    if [ -z "$ESP32_C6_PORT" ]; then
        echo -e "${RED}No ESP32-C6 device found!${NC}"
        echo "Please connect ESP32-C6 via USB and try again."
        echo "Available serial ports:"
        ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  (none found)"
        exit 1
    fi
fi

echo "Target device: $ESP32_C6_PORT"

# Flash firmware
idf.py -p "$ESP32_C6_PORT" flash

echo -e "${GREEN}Flash successful!${NC}"

# ============================================
# Bước 4: Monitor (Optional)
# ============================================
echo ""
echo "[4/4] Opening monitor (Ctrl+] to exit)..."
echo ""
echo "============================================"
echo " Device Output:"
echo "============================================"

idf.py -p "$ESP32_C6_PORT" monitor
