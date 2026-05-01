#!/bin/bash

# ============================================
# ESP32-C6 Matter Fan - Setup & Build Script
# ============================================
# Script này giúp:
# 1. Cài đặt dependencies
# 2. Clone và cài đặt ESP-IDF v5.5.4
# 3. Build firmware
# 4. Flash vào ESP32-C6
# ============================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ESP_IDF_PATH="$HOME/esp/esp-idf"
ESP32_C6_PORT="/dev/ttyACM0"

echo "============================================"
echo " ESP32-C6 Matter Fan - Setup & Build"
echo "============================================"
echo ""

# ============================================
# Bước 1: Cài đặt dependencies
# ============================================
echo "[1/4] Installing dependencies..."
if command -v apt-get &> /dev/null; then
    # Bỏ qua lỗi repository không tồn tại (CD-ROM, iBus Bamboo, v.v.)
    sudo apt-get update --allow-unauthenticated 2>/dev/null || true
    # Hoặc dùng: sudo apt-get update -o Dir::Etc::sourcelist=sources.list.d -o Dir::Etc::sourceparts=/dev/null
    sudo apt-get install -y git curl wget cmake ninja-build python3 python3-pip pipx || true
    echo "✅ Dependencies installed!"
elif command -v pacman &> /dev/null; then
    sudo pacman -S git curl wget cmake ninja python3 pipx --noconfirm
    echo "✅ Dependencies installed!"
else
    echo "⚠️  Please install dependencies manually:"
    echo "   Ubuntu/Debian: sudo apt-get install git curl wget cmake ninja-build python3 python3-pip"
    echo "   Fedora: sudo dnf install git curl wget cmake ninja-build python3"
    echo "   Arch: sudo pacman -S git curl wget cmake ninja python3"
fi
echo ""

# ============================================
# Bước 2: Clone ESP-IDF v5.5.4
# ============================================
echo "[2/4] Setting up ESP-IDF v5.5.4..."
if [ -d "$ESP_IDF_PATH" ]; then
    echo "ℹ️  ESP-IDF already exists at $ESP_IDF_PATH"
    read -p "Reinstall ESP-IDF? (y/N): " reinstall
    if [ "$reinstall" != "y" ] && [ "$reinstall" != "Y" ]; then
        echo "✅ Skipping ESP-IDF installation"
    else
        rm -rf "$ESP_IDF_PATH"
        git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$ESP_IDF_PATH"
        cd "$ESP_IDF_PATH"
        ./install.sh esp32c6
        echo "✅ ESP-IDF installed!"
    fi
else
    echo "Cloning ESP-IDF v5.5.4..."
    git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$ESP_IDF_PATH"
    cd "$ESP_IDF_PATH"
    ./install.sh esp32c6
    echo "✅ ESP-IDF installed!"
fi
echo ""

# ============================================
# Bước 3: Source environment và build
# ============================================
echo "[3/4] Building firmware..."
source "$ESP_IDF_PATH/export.sh"
cd "$PROJECT_DIR"

# Xóa build cũ
idf.py fullclean

# Build firmware
echo "Building firmware..."
idf.py build

echo "✅ Build successful!"
echo ""

# ============================================
# Bước 4: Flash firmware
# ============================================
echo "[4/4] Flashing firmware to $ESP32_C6_PORT..."
read -p "Flash firmware now? (y/N): " flash
if [ "$flash" = "y" ] || [ "$flash" = "Y" ]; then
    idf.py -p $ESP32_C6_PORT flash
    
    echo ""
    echo "✅ Flash successful!"
    echo ""
    echo "Running monitor..."
    idf.py -p $ESP32_C6_PORT monitor
fi

echo ""
echo "============================================"
echo " Done!"
echo "============================================"
