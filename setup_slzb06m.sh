#!/bin/bash
# ============================================================
# SMLIGHT SLZB-06M Thread Border Router Setup Script
# ============================================================
# This script provides instructions and commands to configure
# the SMLIGHT SLZB-06M as a Thread Border Router for the
# ESP32-C6 Smart Fan (Matter over Thread).
#
# Author: ESP32 Smart Fan Project
# Date: 2024
# ============================================================

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}"
echo "============================================================"
echo "  SMLIGHT SLZB-06M Thread Border Router Setup"
echo "  for ESP32-C6 Smart Fan (Matter over Thread)"
echo "============================================================"
echo -e "${NC}"

# ============================================================
# Step 1: Check prerequisites
# ============================================================
echo -e "${YELLOW}[Step 1] Checking prerequisites...${NC}"

# Check if ping command is available
if ! command -v ping &> /dev/null; then
    echo -e "${RED}Error: ping command not found${NC}"
    exit 1
fi

# Ask for SLZB-06M IP address
echo -n "Enter SLZB-06M IP address (default: 192.168.1.1): "
read -r SLZB_IP
SLZB_IP=${SLZB_IP:-192.168.1.1}

echo -e "${GREEN}  SLZB-06M IP: ${SLZB_IP}${NC}"

# ============================================================
# Step 2: Check connectivity
# ============================================================
echo -e "${YELLOW}[Step 2] Checking connectivity to SLZB-06M...${NC}"

if ping -c 2 -W 2 "$SLZB_IP" &> /dev/null; then
    echo -e "${GREEN}  ✓ SLZB-06M is reachable at ${SLZB_IP}${NC}"
else
    echo -e "${RED}  ✗ Cannot reach SLZB-06M at ${SLZB_IP}${NC}"
    echo -e "${YELLOW}  Please check:${NC}"
    echo -e "    1. SLZB-06M is powered on"
    echo -e "    2. SLZB-06M is connected to the same network"
    echo -e "    3. Firewall allows ICMP packets"
    exit 1
fi

# ============================================================
# Step 3: Access SLZB-06M Web UI
# ============================================================
echo -e "${YELLOW}[Step 3] Access SLZB-06M Web UI${NC}"
echo ""
echo -e "  Open a web browser and go to:"
echo -e "  ${GREEN}http://${SLZB_IP}${NC}"
echo ""
echo -e "${YELLOW}  Default login credentials:${NC}"
echo -e "    Username: admin"
echo -e "    Password: admin"
echo ""

# Ask if user has accessed the Web UI
read -p "Have you accessed the Web UI? (y/n): " ACCESS_UI
if [[ "$ACCESS_UI" != "y" && "$ACCESS_UI" != "Y" ]]; then
    echo -e "${YELLOW}  Please access the Web UI first, then run this script again.${NC}"
    exit 0
fi

# ============================================================
# Step 4: Enable Thread Border Router
# ============================================================
echo -e "${YELLOW}[Step 4] Enable Thread Border Router${NC}"
echo ""
echo -e "  In the SLZB-06M Web UI:"
echo -e "  1. Go to ${GREEN}Settings${NC} → ${GREEN}Network${NC}"
echo -e "  2. Enable ${GREEN}Thread${NC} network"
echo -e "  3. Set Thread mode to ${GREEN}Border Router${NC}"
echo -e "  4. Configure Thread network settings:"
echo -e ""
echo -e "     ${BLUE}Thread Network Configuration:${NC}"
echo -e "     - PAN ID: Auto (recommended)"
echo -e "     - Channel: 15 (2.4GHz)"
echo -e "     - Network Key: Auto-generate"
echo -e "     - Extended PAN ID: Auto-generate"
echo -e ""
echo -e "  5. Click ${GREEN}Save${NC}"
echo ""

read -p "Thread Border Router enabled? (y/n): " THREAD_ENABLED
if [[ "$THREAD_ENABLED" != "y" && "$THREAD_ENABLED" != "Y" ]]; then
    echo -e "${YELLOW}  Please enable Thread Border Router in SLZB-06M Web UI first.${NC}"
    exit 0
fi

# ============================================================
# Step 5: Get Thread Dataset (for commissioning)
# ============================================================
echo -e "${YELLOW}[Step 5] Get Thread Network Dataset${NC}"
echo ""
echo -e "  To commission the ESP32-C6 fan, you need the Thread Dataset."
echo -e "  You can get it from the SLZB-06M Web UI or via API."
echo ""
echo -e "${BLUE}Method 1: From Web UI${NC}"
echo -e "  1. Go to ${GREEN}Status${NC} → ${GREEN}Thread${NC}"
echo -e "  2. Copy the ${GREEN}Dataset (Active/Pending)${NC}"
echo ""
echo -e "${BLUE}Method 2: Via API (curl)${NC}"
echo -e "  Run the following command to get the dataset:"
echo ""
echo -e "  ${GREEN}curl -s http://${SLZB_IP}/api/thread/dataset | jq${NC}"
echo ""

# Try to get dataset via API
echo -e "${YELLOW}  Attempting to get Thread dataset via API...${NC}"
DATASET=$(curl -s -m 5 "http://${SLZB_IP}/api/thread/dataset" 2>/dev/null)

if [ $? -eq 0 ] && [ -n "$DATASET" ]; then
    echo -e "${GREEN}  ✓ Thread dataset retrieved:${NC}"
    echo "$DATASET" | head -20
    echo ""
else
    echo -e "${YELLOW}  Could not retrieve dataset via API.${NC}"
    echo -e "${YELLOW}  Please copy the dataset manually from the Web UI.${NC}"
fi

# ============================================================
# Step 6: Commission ESP32-C6 Fan
# ============================================================
echo -e "${YELLOW}[Step 6] Commission ESP32-C6 Fan${NC}"
echo ""
echo -e "  There are two ways to commission the fan:"
echo ""
echo -e "${BLUE}Method 1: QR Code (Recommended)${NC}"
echo -e "  1. Open Matter controller on SLZB-06M (Home Assistant, HomeKit, etc.)"
echo -e "  2. Select 'Add Device' or 'Commission Device'"
echo -e "  3. Choose 'Scan QR Code'"
echo -e "  4. The ESP32-C6 fan will appear and can be scanned"
echo ""
echo -e "${BLUE}Method 2: Manual Pairing Code${NC}"
echo -e "  1. Connect serial to ESP32-C6 fan (USB-Serial)"
echo -e "  2. Watch the console output"
echo -e "  3. Note the Pairing Code displayed"
echo -e "  4. Enter the code in your Matter controller"
echo ""

# ============================================================
# Step 7: Verify connection
# ============================================================
echo -e "${YELLOW}[Step 7] Verify Thread Connection${NC}"
echo ""
echo -e "  After commissioning, verify the connection:"
echo ""
echo -e "  1. Check SLZB-06M Web UI → ${GREEN}Status${NC} → ${GREEN}Thread Devices${NC}"
echo -e "  2. The ESP32-C6 fan should appear in the device list"
echo -e "  3. Verify Matter device is accessible from your controller"
echo ""

# Try to list Thread devices
echo -e "${YELLOW}  Attempting to list Thread devices...${NC}"
DEVICES=$(curl -s -m 5 "http://${SLZB_IP}/api/thread/devices" 2>/dev/null)

if [ $? -eq 0 ] && [ -n "$DEVICES" ]; then
    echo -e "${GREEN}  ✓ Thread devices:${NC}"
    echo "$DEVICES" | head -30
    echo ""
else
    echo -e "${YELLOW}  Could not retrieve device list via API.${NC}"
    echo -e "${YELLOW}  Please check manually in the Web UI.${NC}"
fi

# ============================================================
# Summary
# ============================================================
echo -e "${BLUE}"
echo "============================================================"
echo "  Setup Complete!"
echo "============================================================"
echo -e "${NC}"
echo -e "${GREEN}Next steps:${NC}"
echo -e "  1. Flash the updated firmware to ESP32-C6 fan"
echo -e "  2. Commission the fan using QR Code or Pairing Code"
echo -e "  3. Verify the fan appears in your Matter controller"
echo -e "  4. Test fan control (on/off, speed)"
echo ""
echo -e "${GREEN}Useful links:${NC}"
echo -e "  - SLZB-06M Web UI: http://${SLZB_IP}"
echo -e "  - ESP32-C6 Serial Monitor: Check your serial terminal"
echo ""
echo -e "${YELLOW}Troubleshooting:${NC}"
echo -e "  - Ensure SLZB-06M firmware is up to date"
echo -e "  - Check that Thread is enabled on SLZB-06M"
echo -e "  - Verify ESP32-C6 is powered and running"
echo -e "  - Check serial output for Matter errors"
echo ""
