#!/bin/bash

# ============================================
# ESP32-C6 Flash Script - Đảm bảo nhất
# Sử dụng esptool.py chính thức từ Espressif
# ============================================

PROJECT_DIR="$(cd "$(dirname "$0")" && pwd)"
ESP32_C6_PORT="/dev/ttyACM0"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}============================================${NC}"
echo -e "${BLUE} ESP32-C6 Smart Fan - Flash Script${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""

# ============================================
# Bước 1: Kiểm tra serial port
# ============================================
echo -e "${YELLOW}[1/6] Checking serial port...${NC}"

# Tự động phát hiện port
if [ ! -e "$ESP32_C6_PORT" ]; then
    ESP32_C6_PORT=$(ls /dev/ttyACM* /dev/ttyUSB* 2>/dev/null | head -1)
fi

if [ -z "$ESP32_C6_PORT" ]; then
    echo -e "${RED}❌ Không tìm thấy ESP32-C6!${NC}"
    echo ""
    echo "Hãy:"
    echo "  1. Kết nối ESP32-C6 qua USB"
    echo "  2. Đợi driver cài đặt (vài giây)"
    echo "  3. Thử lại"
    echo ""
    echo "Hoặc chỉ định port thủ công:"
    echo "  ./flash_esp32c6.sh /dev/ttyUSB0"
    exit 1
fi

echo -e "${GREEN}✓ Tìm thấy thiết bị: $ESP32_C6_PORT${NC}"

# Cho phép system nhận device
sleep 2

# ============================================
# Bước 2: Kiểm tra kết nối
# ============================================
echo ""
echo -e "${YELLOW}[2/6] Testing connection...${NC}"

if ! command -v esptool.py &> /dev/null; then
    echo -e "${YELLOW}⚠ Installing esptool.py...${NC}"
    pip3 install esptool --break-system-packages 2>/dev/null || pip3 install esptool
fi

if ! esptool.py -p "$ESP32_C6_PORT" chip_id 2>/dev/null; then
    echo -e "${RED}❌ Kết nối thất bại!${NC}"
    echo ""
    echo "Hãy thử:"
    echo "  1. Nhấn nút BOOT trên ESP32-C6"
    echo "  2. Nhấn nút RESET"
    echo "  3. Chạy lại script"
    echo ""
    echo "Hoặc kiểm tra cable USB (phải là data cable)"
    exit 1
fi

echo -e "${GREEN}✓ Kết nối thành công!${NC}"

# ============================================
# Bước 3: Kiểm tra firmware files
# ============================================
echo ""
echo -e "${YELLOW}[3/6] Checking firmware files...${NC}"

BOOTLOADER="$PROJECT_DIR/build/bootloader/bootloader.bin"
PARTITIONS="$PROJECT_DIR/build/partitions.bin"
FIRMWARE="$PROJECT_DIR/build/esp32-c6_matter_fan.bin"

if [ ! -f "$BOOTLOADER" ]; then
    echo -e "${RED}❌ Bootloader not found!${NC}"
    echo "   Cần build project trước: idf.py build"
    exit 1
fi

if [ ! -f "$PARTITIONS" ]; then
    echo -e "${RED}❌ Partitions not found!${NC}"
    echo "   Cần build project trước: idf.py build"
    exit 1
fi

if [ ! -f "$FIRMWARE" ]; then
    echo -e "${RED}❌ Firmware not found!${NC}"
    echo "   Cần build project trước: idf.py build"
    exit 1
fi

echo -e "${GREEN}✓ Bootloader: $BOOTLOADER${NC}"
echo -e "${GREEN}✓ Partitions: $PARTITIONS${NC}"
echo -e "${GREEN}✓ Firmware: $FIRMWARE${NC}"

# ============================================
# Bước 4: Erase flash
# ============================================
echo ""
echo -e "${YELLOW}[4/6] Erasing flash...${NC}"

esptool.py -p "$ESP32_C6_PORT" erase_flash

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Erase thành công!${NC}"
else
    echo -e "${RED}❌ Erase thất bại!${NC}"
    exit 1
fi

# ============================================
# Bước 5: Flash firmware
# ============================================
echo ""
echo -e "${YELLOW}[5/6] Flashing firmware...${NC}"

esptool.py -p "$ESP32_C6_PORT" \
  --before default_reset \
  --after hard_reset \
  write_flash -z \
  --flash_mode dio \
  --flash_freq 80m \
  --flash_size 2MB \
  0x0000 "$BOOTLOADER" \
  0x8000 "$PARTITIONS" \
  0x10000 "$FIRMWARE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Flash thành công!${NC}"
else
    echo -e "${RED}❌ Flash thất bại!${NC}"
    exit 1
fi

# ============================================
# Bước 6: Verify (tùy chọn)
# ============================================
echo ""
echo -e "${YELLOW}[6/6] Verifying flash...${NC}"

VERIFY_FILE=$(mktemp)
esptool.py -p "$ESP32_C6_PORT" read_flash 0x10000 0x10000 "$VERIFY_FILE" 2>/dev/null

if [ $? -eq 0 ]; then
    ORIGINAL_MD5=$(md5sum "$FIRMWARE" | awk '{print $1}')
    VERIFY_MD5=$(md5sum "$VERIFY_FILE" | awk '{print $1}')
    
    rm -f "$VERIFY_FILE"
    
    if [ "$ORIGINAL_MD5" == "$VERIFY_MD5" ]; then
        echo -e "${GREEN}✓ Verify thành công! Firmware đúng 100%${NC}"
    else
        echo -e "${RED}⚠ Verify thất bại! MD5 không khớp${NC}"
        echo "   Original: $ORIGINAL_MD5"
        echo "   Verified: $VERIFY_MD5"
    fi
else
    echo -e "${YELLOW}⚠ Không thể verify (read failed)${NC}"
fi

# ============================================
# Hoàn thành
# ============================================
echo ""
echo -e "${BLUE}============================================${NC}"
echo -e "${GREEN}✓ FLASH HOÀN TẤT!${NC}"
echo -e "${BLUE}============================================${NC}"
echo ""
echo "ESP32-C6 sẽ tự động restart và chạy firmware mới."
echo ""
echo "Để xem log debug, chạy:"
echo "  screen $ESP32_C6_PORT 115200"
echo "  (Thoát: Ctrl+A rồi nhấn K, sau đó Y)"
echo ""
echo "Hoặc:"
echo "  minicom -b 115200 -o -D $ESP32_C6_PORT"
echo "  (Thoát: Ctrl+A rồi nhấn X)"
