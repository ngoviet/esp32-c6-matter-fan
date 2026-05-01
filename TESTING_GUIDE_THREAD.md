# Hướng dẫn Testing cho Matter over Thread

## 📋 Tổng quan

Hướng dẫn này cung cấp các bước test cho ESP32-C6 Smart Fan sau khi chuyển đổi sang Matter over Thread.

## 🧪 Các bước Test

### Bước 1: Build Firmware

```bash
cd /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan

# Clean previous build
idf.py fullclean

# Configure for ESP32-C6
idf.py set-target esp32c6

# Build firmware
idf.py build
```

**Expected result:**
- Build thành công, không có lỗi
- File firmware tại `build/esp32_c6_matter_fan.bin`

### Bước 2: Flash Firmware

```bash
# Flash firmware vào ESP32-C6
idf.py -p /dev/ttyUSB0 flash

# Hoặc sử dụng esptool trực tiếp
esptool.py -p /dev/ttyUSB0 write_flash 0x0 build/esp32_c6_matter_fan.bin
```

**Expected result:**
- Flash thành công
- ESP32-C6 tự động restart

### Bước 3: Xem Serial Output

```bash
# Xem serial monitor
idf.py -p /dev/ttyUSB0 monitor
```

**Expected output:**
```
I (xxx) MAIN: Initializing NVS...
I (xxx) MAIN: NVS initialized successfully
I (xxx) MAIN: Initializing network...
I (xxx) MAIN: Initializing IEEE 802.15.4 radio...
I (xxx) MAIN: IEEE 802.15.4 radio initialized
I (xxx) MAIN: GPIO ISR service installed
I (xxx) MAIN: Initializing System Manager...
I (xxx) SYSTEM: Initializing system...
I (xxx) SYSTEM: System initialized successfully
I (xxx) MAIN: Initializing Matter over Thread...
I (xxx) APP_MATTER: Initializing Matter over Thread...
I (xxx) APP_MATTER: Matter node created successfully
I (xxx) APP_MATTER: Thread Network Interface endpoint created
I (xxx) APP_MATTER: Thread Network Diagnostics endpoint created
I (xxx) APP_MATTER: Fan endpoint created: <endpoint_id>
I (xxx) APP_MATTER: Fan clusters added: On/Off, Fan Control, Level Control
I (xxx) APP_MATTER: Matter over Thread initialized successfully!
I (xxx) MAIN: ESP32-C6 Smart Fan with Matter over Thread is ready!
```

### Bước 4: Kiểm tra Thread Radio

Trong serial monitor, kiểm tra:
- ✅ IEEE 802.15.4 radio đã được initialize
- ✅ MAC address được hiển thị
- ✅ Không có lỗi Thread initialization

### Bước 5: Commission Device

#### Method 1: QR Code
1. Mở Matter controller (Home Assistant, Apple HomeKit, Google Home)
2. Chọn "Add Device" hoặc "Commission Device"
3. Quét QR Code từ serial console

#### Method 2: Pairing Code
1. Lấy 11-digit pairing code từ serial console
2. Nhập vào Matter controller

**Expected result:**
- Device được commission thành công
- SLZB-06M hiển thị device trong Thread network

### Bước 6: Test Matter Control

#### Test On/Off
1. Từ Matter controller, bật/tắt fan
2. Kiểm tra serial output:
```
I (xxx) APP_MATTER: Matter Cmd: OnOff = 1
I (xxx) FAN_CONTROLLER: Fan turned ON
```

#### Test Speed Control
1. Từ Matter controller, điều chỉnh tốc độ
2. Kiểm tra serial output:
```
I (xxx) APP_MATTER: Matter Cmd: Fan Speed Setting = 50%
I (xxx) FAN_CONTROLLER: Speed set to 50% (250 Hz)
```

### Bước 7: Test Local Control

#### Rotary Encoder
1. Xoay rotary encoder để thay đổi tốc độ
2. Kiểm tra serial output:
```
I (xxx) SYSTEM: Encoder moved to step 16 (48.5%)
I (xxx) FAN_CONTROLLER: Speed set to 48% (248 Hz)
I (xxx) APP_MATTER: Reported speed: 48%
```

#### Button
1. Nhấn button để bật/tắt fan
2. Kiểm tra serial output:
```
I (xxx) SYSTEM: Button pressed!
I (xxx) FAN_CONTROLLER: Fan turned OFF
I (xxx) APP_MATTER: Reported OnOff: 0
```

### Bước 8: Kiểm tra Thread Network Diagnostics

Sử dụng Matter console để kiểm tra Thread diagnostics:

```bash
# Kết nối Matter console
idf.py -p /dev/ttyUSB0 monitor

# Trong Matter console:
diagnostics thread
```

**Expected output:**
- Thread version
- Network uptime
- Node count
- Message statistics

## 🔍 Troubleshooting

### Vấn đề 1: Build lỗi

**Triệu chứng:**
```
CMake Error: ...
```

**Giải pháp:**
```bash
idf.py fullclean
rm -rf build
idf.py set-target esp32c6
idf.py build
```

### Vấn đề 2: Flash lỗi

**Triệu chứng:**
```
Serial port /dev/ttyUSB0 not found
```

**Giải pháp:**
```bash
# Kiểm tra device
ls -la /dev/ttyUSB*

# Thêm user vào dialout group
sudo usermod -a -G dialout $USER

# Logout và login lại
```

### Vấn đề 3: Thread không kết nối

**Triệu chứng:**
```
I (xxx) APP_MATTER: Matter start failed: -1
```

**Giải pháp:**
1. Kiểm tra SLZB-06M đã enable Thread Border Router
2. Kiểm tra firmware SLZB-06M là mới nhất
3. Kiểm tra IEEE 802.15.4 radio đã được initialize

### Vấn đề 4: Matter controller không thấy device

**Triệu chứng:**
- Device không xuất hiện khi scan QR code

**Giải pháp:**
1. Đảm bảo SLZB-06M và ESP32-C6 cùng mạng
2. Kiểm tra Thread network key đúng
3. Restart Matter controller
4. Xóa device và commission lại

## 📊 Test Checklist

| Test Case | Expected Result | Status |
|-----------|-----------------|--------|
| Build firmware | Build thành công | ☐ |
| Flash firmware | Flash thành công | ☐ |
| NVS init | NVS initialized | ☐ |
| IEEE 802.15.4 | Radio initialized | ☐ |
| GPIO ISR | ISR service installed | ☐ |
| System Manager | System initialized | ☐ |
| Matter node | Node created | ☐ |
| Fan endpoint | Endpoint created | ☐ |
| Thread commissioning | Commission thành công | ☐ |
| Matter On/Off | Bật/tắt hoạt động | ☐ |
| Matter Speed | Điều khiển tốc độ | ☐ |
| Rotary Encoder | Thay đổi tốc độ | ☐ |
| Button | Bật/tắt fan | ☐ |
| Thread Diagnostics | Thông tin hiển thị | ☐ |

## 📈 Performance Metrics

Theo dõi các metrics sau:

1. **Thread Connection Time**: Thời gian để join Thread network
2. **Matter Response Time**: Thời gian phản hồi từ Matter controller
3. **Local Control Latency**: Độ trễ khi điều khiển local
4. **Power Consumption**: Tiêu thụ điện năng (nếu có thể đo)

## 🔄 Recovery

Nếu device bị lỗi, có thể recovery:

1. **Factory Reset**: Nhấn button 10 lần
2. **Re-commission**: Commission lại từ Matter controller
3. **Re-flash**: Nạp lại firmware mới
