#!/bin/bash
# ============================================
# ESP32-C6 Smart Fan - Automated Build Script
# ============================================

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export IDF_PATH="/home/vokupt/esp/esp-idf"
export ESP_MATTER_PATH="/home/vokupt/esp/esp-matter"

echo "============================================"
echo " ESP32-C6 Smart Fan - Build"
echo "============================================"
echo ""

# Check ESP-IDF
if [ ! -f "$IDF_PATH/export.sh" ]; then
    echo "ERROR: ESP-IDF not found at $IDF_PATH"
    exit 1
fi

# Check ESP-Matter
if [ ! -d "$ESP_MATTER_PATH" ]; then
    echo "ERROR: ESP-Matter not found at $ESP_MATTER_PATH"
    exit 1
fi

echo "[1/3] ESP-IDF: $IDF_PATH"
echo "[2/3] ESP-Matter: $ESP_MATTER_PATH"
echo ""

# Setup environment
echo "[1/3] Setting up environment..."
source "$IDF_PATH/export.sh"
echo "Environment ready!"
echo ""

# Set target
echo "[2/3] Setting target to ESP32-C6..."
cd "$SCRIPT_DIR"
idf.py set-target esp32c6

# Build
echo ""
echo "[3/3] Building firmware..."
idf.py build

# Check result
if [ -f "$SCRIPT_DIR/build/bootloader/bootloader.bin" ]; then
    echo ""
    echo "============================================"
    echo " Build Successful!"
    echo "============================================"
    echo "Firmware files:"
    find "$SCRIPT_DIR/build" -name "*.bin" -type f | head -20
    echo ""
    echo "To flash, run:"
    echo "  cd $SCRIPT_DIR && idf.py flash"
    echo ""
else
    echo ""
    echo "ERROR: Build failed - no output files found"
    exit 1
fi
