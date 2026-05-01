#!/bin/bash

# ============================================
# ESP32-C6 Matter Fan - Native Linux Installation Script
# ============================================
# Script này sẽ cài đặt toàn bộ môi trường để build và flash firmware
# cho ESP32-C6 Matter Fan project trên Linux.
#
# Yêu cầu:
# - Chạy script với sudo: sudo bash install_native.sh
# - Hoặc chạy từng bước không có sudo nếu có quyền
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
echo " ESP32-C6 Matter Fan - Native Linux Setup"
echo "============================================"
echo ""

# ============================================
# Bước 0: Kiểm tra quyền sudo
# ============================================
echo "[0/8] Checking permissions..."
if [ "$EUID" -ne 0 ]; then 
    log_error "Please run this script with sudo: sudo bash install_native.sh"
    exit 1
fi
log_success "Running as root"
echo ""

# ============================================
# Bước 1: Cài đặt dependencies
# ============================================
echo "[1/8] Installing dependencies..."
apt-get update
apt-get install -y \
    git \
    curl \
    wget \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    pipx \
    python3-venv \
    python3-full \
    python3-setuptools \
    libssl-dev \
    libffi-dev \
    pkg-config \
    xz-utils \
    libglib2.0-dev \
    libdbus-1-dev \
    libavahi-client-dev \
    libavahi-common-dev \
    libreadline-dev \
    device-tree-compiler \
    openocd \
    esptool \
    dfu-util \
    udev

log_success "Dependencies installed!"
echo ""

# ============================================
# Bước 2: Thêm user vào group dialout
# ============================================
echo "[2/8] Configuring USB permissions..."
if ! groups $USER_NAME | grep -q dialout; then
    usermod -a -G dialout $USER_NAME
    log_info "Added user '$USER_NAME' to 'dialout' group"
else
    log_info "User '$USER_NAME' is already in 'dialout' group"
fi

# Cấu hình udev rules cho ESP32
cat > /etc/udev/rules.d/99-esp32.rules << EOF
# ESP32-Venom
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1004", MODE="0666"
# ESP32-S3
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="1001", MODE="0666"
# ESP32-C3
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", ATTRS{idProduct}=="0059", MODE="0666"
# ESP32
SUBSYSTEM=="tty", ATTRS{idVendor}":"303a", ATTRS{idProduct}=="0001", MODE="0666"
# CH340
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="5512", MODE="0666"
# CP2102
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE="0666"
# FTDI
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
# Generic ESP32
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", MODE="0666"
EOF

udevadm control --reload-rules
udevadm trigger

log_success "USB permissions configured!"
echo ""

# ============================================
# Bước 3: Clone và cài đặt ESP-IDF v5.5.4
# ============================================
echo "[3/8] Setting up ESP-IDF v5.5.4..."
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
echo "[4/8] Setting up ESP-Matter SDK..."

# QUAN TRỌNG: Cho phép pip cài package system-wide (PEP 668 override)
# Python 3.12+ không cho phép cài package system-wide bằng pip
# Cần set biến này để ESP-Matter install.sh hoạt động
export PIP_BREAK_SYSTEM_PACKAGES=1
log_info "Set PIP_BREAK_SYSTEM_PACKAGES=1 to allow system-wide pip installs"

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
echo "[5/8] Configuring environment variables..."

# Thêm vào ~/.bashrc
BASHRC="$HOME/.bashrc"

# Kiểm tra nếu đã có cấu hình ESP-IDF
if grep -q "# ESP-IDF Configuration" "$BASHRC" 2>/dev/null; then
    log_info "ESP-IDF configuration already exists in .bashrc"
else
    cat >> "$BASHRC" << EOF

# ESP-IDF Configuration
export IDF_PATH=$ESP_IDF_PATH
source $ESP_IDF_PATH/export.sh

# ESP-Matter Configuration
export ESP_MATTER_PATH=$ESP_MATTER_PATH
EOF
    log_success "Environment variables added to $BASHRC"
fi

# Thêm vào ~/.profile cho login shell
PROFILE="$HOME/.profile"
if grep -q "# ESP-IDF Configuration" "$PROFILE" 2>/dev/null; then
    log_info "ESP-IDF configuration already exists in .profile"
else
    cat >> "$PROFILE" << EOF

# ESP-IDF Configuration
export IDF_PATH=$ESP_IDF_PATH
source $ESP_IDF_PATH/export.sh

# ESP-Matter Configuration
export ESP_MATTER_PATH=$ESP_MATTER_PATH
EOF
    log_success "Environment variables added to $PROFILE"
fi

# Source ngay để sử dụng
source "$ESP_IDF_PATH/export.sh"
export ESP_MATTER_PATH=$ESP_MATTER_PATH

log_success "Environment variables configured!"
echo ""

# ============================================
# Bước 6: Kiểm tra ESP32-C6 connection
# ============================================
echo "[6/8] Checking ESP32-C6 connection..."
if [ -e "$ESP32_C6_PORT" ]; then
    log_success "ESP32-C6 detected at $ESP32_C6_PORT"
    
    # Kiểm tra permission
    if [ -w "$ESP32_C6_PORT" ]; then
        log_success "Write permission granted for $ESP32_C6_PORT"
    else
        log_warn "No write permission for $ESP32_C6_PORT, fixing..."
        sudo chmod 666 "$ESP32_C6_PORT"
        log_success "Permission fixed!"
    fi
else
    log_warn "ESP32-C6 not found at $ESP32_C6_PORT"
    log_info "Available serial ports:"
    ls -la /dev/ttyACM* /dev/ttyUSB* 2>/dev/null || echo "  No serial devices found"
    echo ""
    log_info "Please connect ESP32-C6 via USB and try again."
fi
echo ""

# ============================================
# Bước 7: Build firmware
# ============================================
echo "[7/8] Building firmware..."
cd "$PROJECT_DIR"

# Source environment
source "$ESP_IDF_PATH/export.sh"
export ESP_MATTER_PATH=$ESP_MATTER_PATH

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
# Bước 8: Flash firmware
# ============================================
echo "[8/8] Flashing firmware..."

if [ -e "$ESP32_C6_PORT" ]; then
    log_info "Flashing firmware to $ESP32_C6_PORT..."
    idf.py -p $ESP32_C6_PORT flash
    log_success "Flash successful!"
else
    log_warn "ESP32-C6 not connected. Flash skipped."
    log_info "To flash manually later:"
    log_info "  cd $PROJECT_DIR"
    log_info "  source $ESP_IDF_PATH/export.sh"
    log_info "  idf.py -p $ESP32_C6_PORT flash"
fi
echo ""

# ============================================
# Hoàn thành
# ============================================
echo "============================================"
echo " Setup Complete!"
echo "============================================"
echo ""
echo "Environment setup:"
echo "  ESP-IDF:  $ESP_IDF_PATH"
echo "  ESP-Matter: $ESP_MATTER_PATH"
echo ""
echo "Firmware:"
echo "  Location: $PROJECT_DIR/build/esp32_c6_matter_fan.bin"
echo ""
echo "Next time, to build and flash:"
echo "  cd $PROJECT_DIR"
echo "  source $ESP_IDF_PATH/export.sh"
echo "  idf.py build flash"
echo ""
echo "To monitor serial output:"
echo "  idf.py -p $ESP32_C6_PORT monitor"
echo ""
echo "Note: You need to logout and login again for dialout group to take effect."
echo ""
