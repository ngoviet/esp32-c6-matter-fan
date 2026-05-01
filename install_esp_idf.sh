#!/bin/bash

# ============================================
# ESP-IDF v5.5.4 Installation Script for ESP32-C6
# ============================================
# Script này sẽ:
# 1. Cài đặt dependencies
# 2. Clone ESP-IDF v5.5.4
# 3. Cài đặt toolchain cho ESP32-C6
# 4. Cấu hình environment
# ============================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE} ESP-IDF v5.5.4 Installation for ESP32-C6${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# ============================================
# Bước 1: Cài đặt dependencies
# ============================================
echo -e "${YELLOW}[1/4] Installing dependencies...${NC}"

if command -v apt-get &> /dev/null; then
    sudo apt-get update || true
    sudo apt-get install -y \
        git \
        wget \
        flex \
        bison \
        gperf \
        python3 \
        python3-pip \
        python3-setuptools \
        cmake \
        ninja-build \
        ccache \
        libffi-dev \
        libssl-dev \
        dfu-util \
        usbutils
elif command -v yum &> /dev/null; then
    sudo yum install -y \
        git \
        wget \
        flex \
        bison \
        gperf \
        python3 \
        cmake \
        ninja-build \
        ccache \
        libffi-devel \
        openssl-devel \
        dfu-util \
        usbutils
else
    echo -e "${RED}Unsupported package manager!${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Dependencies installed!${NC}"

# ============================================
# Bước 2: Clone ESP-IDF v5.5.4
# ============================================
echo ""
echo -e "${YELLOW}[2/4] Cloning ESP-IDF v5.5.4...${NC}"

ESP_IDF_PATH="$HOME/esp/esp-idf"

if [ -d "$ESP_IDF_PATH" ]; then
    echo -e "${YELLOW}⚠ ESP-IDF already exists at $ESP_IDF_PATH${NC}"
    read -p "Do you want to reinstall? (y/N): " REINSTALL
    if [ "$REINSTALL" != "y" ]; then
        echo -e "${GREEN}✓ Skipping clone. Using existing installation.${NC}"
    else
        rm -rf "$ESP_IDF_PATH"
    fi
fi

if [ ! -d "$ESP_IDF_PATH" ]; then
    mkdir -p ~/esp
    cd ~/esp
    git clone --recursive --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git
fi

echo -e "${GREEN}✓ ESP-IDF cloned to $ESP_IDF_PATH${NC}"

# ============================================
# Bước 3: Cài đặt toolchain cho ESP32-C6
# ============================================
echo ""
echo -e "${YELLOW}[3/4] Installing toolchain for ESP32-C6...${NC}"

cd "$ESP_IDF_PATH"
./install.sh esp32c6

echo -e "${GREEN}✓ Toolchain installed!${NC}"

# ============================================
# Bước 4: Cấu hình environment
# ============================================
echo ""
echo -e "${YELLOW}[4/4] Configuring environment...${NC}"

# Thêm vào .bashrc nếu chưa có
if ! grep -q "export ESP_IDF_PATH" ~/.bashrc 2>/dev/null; then
    echo "" >> ~/.bashrc
    echo "# ESP-IDF Configuration" >> ~/.bashrc
    echo "export ESP_IDF_PATH=\"$HOME/esp/esp-idf\"" >> ~/.bashrc
    echo ". \$ESP_IDF_PATH/export.sh" >> ~/.bashrc
    echo -e "${GREEN}✓ Added to ~/.bashrc${NC}"
fi

# Source environment ngay
source "$ESP_IDF_PATH/export.sh"

# ============================================
# Hoàn thành
# ============================================
echo ""
echo -e "${BLUE}============================================${NC}"
echo -e "${GREEN}✓ ESP-IDF v5.5.4 INSTALLED!${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""
echo "Đường dẫn: $ESP_IDF_PATH"
echo ""
echo "Để sử dụng, chạy:"
echo "  source ~/esp/esp-idf/export.sh"
echo ""
echo "Hoặc mở terminal mới (đã tự động cấu hình)"
echo ""

# Kiểm tra version
echo -e "${YELLOW}Kiểm tra version:${NC}"
idf.py --version 2>/dev/null || echo "idf.py not in PATH - run: source ~/esp/esp-idf/export.sh"
