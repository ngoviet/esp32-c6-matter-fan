#!/bin/bash

# ============================================
# ESP32-C6 Matter Fan - Auto Setup & Build
# ============================================
# Tự động:
# 1. Sửa repository lỗi
# 2. Cài đặt dependencies
# 3. Clone và cài đặt ESP-IDF v5.5.4
# 4. Build firmware
# 5. Flash vào ESP32-C6
# ============================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ESP_IDF_PATH="$HOME/esp/esp-idf"
ESP32_C6_PORT="/dev/ttyACM0"

echo "============================================"
echo " ESP32-C6 Matter Fan - Auto Setup"
echo "============================================"
echo ""

# ============================================
# Bước 0: Sửa repository lỗi
# ============================================
echo "[0/5] Fixing apt repositories..."

# Xóa các repository lỗi
sudo rm -f /etc/apt/sources.list.d/cdrom.list 2>/dev/null || true
sudo rm -f /etc/apt/sources.list.d/bamboo-engine-*.list 2>/dev/null || true

# Tìm và xóa các repository lỗi
for f in /etc/apt/sources.list.d/*.list; do
    if grep -q "cdrom" "$f" 2>/dev/null; then
        echo "  Removing CD-ROM repo: $f"
        sudo rm -f "$f"
    fi
    if grep -q "ppa.launchpadcontent.net/bamboo-engine" "$f" 2>/dev/null; then
        echo "  Removing iBus Bamboo repo: $f"
        sudo rm -f "$f"
    fi
done

echo "✅ Repositories fixed!"
echo ""

# ============================================
# Bước 1: Cài đặt dependencies
# ============================================
echo "[1/5] Installing dependencies..."
sudo apt-get update -y
sudo apt-get install -y git curl wget cmake ninja-build python3 python3-pip pipx

# Kiểm tra git
if ! command -v git &> /dev/null; then
    echo "❌ Git not found! Please install git manually."
    exit 1
fi

echo "✅ Dependencies installed!"
echo ""

# ============================================
# Bước 2: Clone và cài đặt ESP-IDF v5.5.4
# ============================================
echo "[2/5] Setting up ESP-IDF v5.5.4..."

if [ -d "$ESP_IDF_PATH" ]; then
    echo "ℹ️  ESP-IDF exists at $ESP_IDF_PATH"
    read -p "Reinstall? (y/N): " reinstall
    if [ "$reinstall" = "y" ] || [ "$reinstall" = "Y" ]; then
        rm -rf "$ESP_IDF_PATH"
    else
        echo "✅ Skipping ESP-IDF installation"
    fi
else
    echo "Cloning ESP-IDF v5.5.4..."
    git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$ESP_IDF_PATH"
fi

if [ -d "$ESP_IDF_PATH" ]; then
    cd "$ESP_IDF_PATH"
    echo "Installing ESP-IDF tools for ESP32-C6..."
    ./install.sh esp32c6
    echo "✅ ESP-IDF installed!"
fi
echo ""

# ============================================
# Bước 3: Source environment
# ============================================
echo "[3/5] Setting up environment..."
source "$ESP_IDF_PATH/export.sh"
echo "✅ Environment ready!"
echo ""

# ============================================
# Bước 4: Build firmware
# ============================================
echo "[4/5] Building firmware..."
cd "$PROJECT_DIR"

echo "Cleaning old build..."
idf.py fullclean

echo "Building..."
idf.py build

echo "✅ Build successful!"
echo ""
echo "Firmware location:"
echo "  $PROJECT_DIR/build/esp32_c6_matter_fan.bin"
echo ""

# ============================================
# Bước 5: Flash firmware
# ============================================
echo "[5/5] Flashing firmware..."

# Kiểm tra port
if [ ! -e "$ESP32_C6_PORT" ]; then
    echo "⚠️  Port $ESP32_C6_PORT not found!"
    echo "Available serial ports:"
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  No serial devices found"
    echo ""
    read -p "Enter port path (default: $ESP32_C6_PORT): " port
    ESP32_C6_PORT="${port:-$ESP32_C6_PORT}"
fi

# Kiểm tra permission
if ! sudo test -w "$ESP32_C6_PORT"; then
    echo "⚠️  No write permission for $ESP32_C6_PORT"
    echo "Adding user to dialout group..."
    sudo usermod -a -G dialout $USER
    echo "⚠️  Please logout and login again, then run:"
    echo "   idf.py -p $ESP32_C6_PORT flash"
    echo ""
    echo "For now, using sudo to flash..."
    sudo idf.py -p $ESP32_C6_PORT flash
else
    idf.py -p $ESP32_C6_PORT flash
fi

echo ""
echo "✅ Flash complete!"
echo ""

# ============================================
# Hỏi có muốn xem log không
# ============================================
read -p "Start serial monitor? (y/N): " monitor
if [ "$monitor" = "y" ] || [ "$monitor" = "Y" ]; then
    echo "Starting monitor (Ctrl+] to exit)..."
    echo ""
    idf.py -p $ESP32_C6_PORT monitor
fi

echo ""
echo "============================================"
echo " Setup Complete!"
echo "============================================"
echo ""
echo "Next time, to build and flash only:"
echo "  cd $PROJECT_DIR"
echo "  source $ESP_IDF_PATH/export.sh"
echo "  idf.py build flash"
echo ""
