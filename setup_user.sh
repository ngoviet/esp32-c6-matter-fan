#!/bin/bash

# ============================================
# ESP32-C6 Matter Fan - User Setup Script
# ============================================
# Script này chạy KHÔNG CẦN sudo
# Cài đặt ESP-IDF và ESP-Matter vào $HOME/esp/
# ============================================

set -e

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ESP_IDF_PATH="$HOME/esp/esp-idf"
ESP_MATTER_PATH="$HOME/esp/esp-matter"
ESP32_C6_PORT="/dev/ttyACM0"
USER_NAME=$(whoami)

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

echo "============================================"
echo " ESP32-C6 Matter Fan - User Setup"
echo "============================================"
echo ""

# ============================================
# Bước 1: Kiểm tra dependencies
# ============================================
echo "[1/6] Checking dependencies..."

MISSING_DEPS=()

for cmd in git python3 pipx; do
    if ! command -v $cmd &> /dev/null; then
        MISSING_DEPS+=($cmd)
    fi
done

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    log_warn "Missing dependencies: ${MISSING_DEPS[*]}"
    echo ""
    echo "Please install missing dependencies manually:"
    echo "  sudo apt-get update"
    echo "  sudo apt-get install -y git python3 python3-pip pipx python3-venv"
    echo ""
    echo "Then run this script again."
    exit 1
fi

# Kiểm tra cmake và ninja
for cmd in cmake ninja; do
    if ! command -v $cmd &> /dev/null; then
        log_warn "$cmd not found - sẽ được cài đặt bởi ESP-IDF"
    fi
done

log_success "Core dependencies found!"
echo ""

# ============================================
# Bước 2: Cài đặt dependencies bổ sung
# ============================================
echo "[2/6] Installing additional dependencies..."

# Chỉ cài nếu chưa có, không dùng sudo cho apt
if ! command -v cmake &> /dev/null || ! command -v ninja &> /dev/null; then
    echo ""
    log_warn "cmake or ninja not found!"
    echo ""
    echo "Please run these commands with sudo first:"
    echo ""
    echo "  sudo apt-get update"
    echo "  sudo apt-get install -y cmake ninja-build"
    echo ""
    echo "Then run this script again."
    echo ""
    exit 1
else
    log_success "cmake and ninja found!"
fi

# Cài đặt các package pip cần thiết cho user
log_info "Installing pip packages for user..."
pipx install --force cffi 2>/dev/null || true
log_success "Additional dependencies ready!"
echo ""

# ============================================
# Bước 3: Clone và cài đặt ESP-IDF v5.5.4
# ============================================
echo "[3/6] Setting up ESP-IDF v5.5.4..."
mkdir -p $HOME/esp

if [ -d "$ESP_IDF_PATH" ]; then
    log_info "ESP-IDF already exists at $ESP_IDF_PATH"
    read -p "Reinstall ESP-IDF? (y/N): " reinstall
    if [ "$reinstall" != "y" ] && [ "$reinstall" != "Y" ]; then
        log_info "Skipping ESP-IDF installation"
    else
        rm -rf "$ESP_IDF_PATH"
        git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$ESP_IDF_PATH"
        cd "$ESP_IDF_PATH"
        ./install.sh esp32c6
        log_success "ESP-IDF installed!"
    fi
else
    log_info "Cloning ESP-IDF v5.5.4..."
    git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git "$ESP_IDF_PATH"
    cd "$ESP_IDF_PATH"
    log_info "Installing ESP-IDF tools for ESP32-C6..."
    ./install.sh esp32c6
    log_success "ESP-IDF installed!"
fi
echo ""

# ============================================
# Bước 4: Clone và cài đặt ESP-Matter SDK
# ============================================
echo "[4/6] Setting up ESP-Matter SDK..."

# QUAN TRỌNG: Set biến môi trường để ghi đè PEP 668
export PIP_BREAK_SYSTEM_PACKAGES=1
log_info "Set PIP_BREAK_SYSTEM_PACKAGES=1 (required for Python 3.12+)"

if [ -d "$ESP_MATTER_PATH" ]; then
    log_info "ESP-Matter already exists at $ESP_MATTER_PATH"
    read -p "Reinstall ESP-Matter? (y/N): " reinstall_matter
    if [ "$reinstall_matter" != "y" ] && [ "$reinstall_matter" != "Y" ]; then
        log_info "Skipping ESP-Matter installation"
    else
        rm -rf "$ESP_MATTER_PATH"
        git clone --depth 1 --recursive https://github.com/espressif/esp-matter.git "$ESP_MATTER_PATH"
        cd "$ESP_MATTER_PATH"
        ./install.sh
        log_success "ESP-Matter installed!"
    fi
else
    log_info "Cloning ESP-Matter SDK..."
    git clone --depth 1 --recursive https://github.com/espressif/esp-matter.git "$ESP_MATTER_PATH"
    cd "$ESP_MATTER_PATH"
    log_info "Bootstrapping ESP-Matter..."
    ./install.sh
    log_success "ESP-Matter installed!"
fi
echo ""

# ============================================
# Bước 5: Cấu hình environment variables
# ============================================
echo "[5/6] Configuring environment variables..."

# Source ESP-IDF
source "$ESP_IDF_PATH/export.sh"
export ESP_MATTER_PATH=$ESP_MATTER_PATH

# Thêm vào ~/.bashrc nếu chưa có
BASHRC="$HOME/.bashrc"

if ! grep -q "# ESP-IDF Configuration" "$BASHRC" 2>/dev/null; then
    cat >> "$BASHRC" << EOF

# ESP-IDF Configuration
export IDF_PATH=$ESP_IDF_PATH
source $ESP_IDF_PATH/export.sh

# ESP-Matter Configuration
export ESP_MATTER_PATH=$ESP_MATTER_PATH

# PEP 668 override for Python 3.12+
export PIP_BREAK_SYSTEM_PACKAGES=1
EOF
    log_success "Environment variables added to $BASHRC"
else
    log_info "ESP-IDF configuration already exists in .bashrc"
fi

# Source lại để sử dụng ngay
source "$BASHRC"

log_success "Environment variables configured!"
echo ""

# ============================================
# Bước 6: Build firmware
# ============================================
echo "[6/6] Building firmware..."
cd "$PROJECT_DIR"

# Clean old build
log_info "Cleaning old build..."
idf.py fullclean

# Build firmware
log_info "Building firmware..."
idf.py build

log_success "Build successful!"
echo ""
log_info "Firmware location: $PROJECT_DIR/build/esp32_c6_matter_fan.bin"
echo ""

# ============================================
# Flash firmware
# ============================================
echo ""
echo "============================================"
echo " Checking ESP32-C6 connection..."
echo "============================================"

if [ -e "$ESP32_C6_PORT" ]; then
    log_success "ESP32-C6 detected at $ESP32_C6_PORT"
    
    # Kiểm tra permission
    if [ -w "$ESP32_C6_PORT" ]; then
        log_success "Write permission granted"
    else
        log_warn "No write permission, trying with sudo..."
        SUDO="sudo"
    fi
    
    echo ""
    read -p "Flash firmware now? (y/N): " flash
    if [ "$flash" = "y" ] || [ "$flash" = "Y" ]; then
        log_info "Flashing firmware..."
        $SUDO idf.py -p $ESP32_C6_PORT flash
        log_success "Flash successful!"
    fi
    
    echo ""
    read -p "Start serial monitor? (y/N): " monitor
    if [ "$monitor" = "y" ] || [ "$monitor" = "Y" ]; then
        log_info "Starting serial monitor (Ctrl+] to exit)..."
        $SUDO idf.py -p $ESP32_C6_PORT monitor
    fi
else
    log_warn "ESP32-C6 not found at $ESP32_C6_PORT"
    log_info "Available serial ports:"
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  No serial devices found"
    echo ""
    log_info "Connect ESP32-C6 via USB and run:"
    log_info "  idf.py -p $ESP32_C6_PORT flash"
fi

# ============================================
# Hoàn thành
# ============================================
echo ""
echo "============================================"
echo " Setup Complete!"
echo "============================================"
echo ""
echo "Environment setup:"
echo "  ESP-IDF:    $ESP_IDF_PATH"
echo "  ESP-Matter: $ESP_MATTER_PATH"
echo ""
echo "Firmware:"
echo "  Location: $PROJECT_DIR/build/esp32_c6_matter_fan.bin"
echo ""
echo "Next time, to build and flash:"
echo "  cd $PROJECT_DIR"
echo "  source $ESP_IDF_PATH/export.sh"
echo "  export ESP_MATTER_PATH=$ESP_MATTER_PATH"
echo "  idf.py build flash"
echo ""
echo "To monitor serial output:"
echo "  idf.py -p $ESP32_C6_PORT monitor"
echo ""
echo "Note: If you don't have dialout group permission, you may need to run:"
echo "  sudo usermod -a -G dialout $USER_NAME"
echo "  (Then logout and login again)"
echo ""
