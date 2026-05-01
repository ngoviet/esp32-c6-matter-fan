# ESP32-C6 Smart Fan - Matter over Thread

## 📋 Tổng quan

Project này chuyển đổi ESP32-C6 Smart Fan từ **Matter over WiFi** sang **Matter over Thread** để kết nối với **SMLIGHT SLZB-06M** (Thread Border Router).

### Kiến trúc
```
┌─────────────────────────────────────────────────────┐
│           SMLIGHT SLZB-06M                          │
│    Thread Border Router + Zigbee Coordinator        │
└──────────────┬──────────────────────────────────────┘
               │ Thread Network (802.15.4)
               │
┌──────────────┼──────────────────────────────────────┐
│              ▼                                      │
│      ESP32-C6 Smart Fan                             │
│   ┌─────────────────────────────┐                   │
│   │ Matter Stack (Over Thread)  │                   │
│   │   - Fan Control             │                   │
│   │   - On/Off                  │                   │
│   │   - Thread Diagnostics      │                   │
│   └───────────┬─────────────────┘                   │
│               │                                     │
│   ┌───────────┴─────────────────┐                   │
│   │ System Manager              │                   │
│   │ (Rotary Encoder + Button)   │                   │
│   └─────────────────────────────┘                   │
└─────────────────────────────────────────────────────┘
```

## 🎯 Tính năng

- ✅ **Matter over Thread**: Kết nối qua Thread network (802.15.4)
- ✅ **SLZB-06M Integration**: Kết nối với SMLIGHT SLZB-06M làm Thread Border Router
- ✅ **Fan Control**: Điều khiển tốc độ quạt (0-100%)
- ✅ **On/Off Control**: Bật/tắt quạt
- ✅ **Local Control**: Rotary encoder và button
- ✅ **Thread Diagnostics**: Thông tin mạng Thread

## 📂 Cấu trúc project

```
esp32_c6_matter_fan/
├── CMakeLists.txt              # Main CMake config (Thread support)
├── sdkconfig.defaults          # Thread configuration defaults
├── main/
│   ├── main.cpp                # Entry point (Thread init)
│   ├── app_matter.cpp          # Matter over Thread implementation
│   ├── app_matter.h            # Matter interface header
│   ├── fan_controller.h        # Fan PWM control
│   ├── rotary_encoder.h        # Rotary encoder driver
│   ├── button.h                # Button driver
│   ├── system_manager.h        # System coordinator
│   ├── config.h                # GPIO configuration
│   └── CMakeLists.txt          # Main component config
├── setup_slzb06m.sh            # SLZB-06M setup script
├── TESTING_GUIDE_THREAD.md     # Testing guide
├── README_THREAD.md            # This file
└── plans/
    └── matter_over_thread_migration.md  # Migration plan
```

## 🔧 Yêu cầu

### Phần cứng
- ESP32-C6 Dev Board
- SMLIGHT SLZB-06M (Thread Border Router)
- Rotary Encoder (ECC11 hoặc tương đương)
- Button
- Quạt DC 5V-12V

### Phần mềm
- ESP-IDF v5.5.4+
- ESP-Matter SDK
- Python 3.12+

## 🚀 Cài đặt

### Bước 1: Cài đặt môi trường

```bash
cd /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan
sudo bash install_native.sh
```

Xem [`NATIVE_SETUP_GUIDE.md`](NATIVE_SETUP_GUIDE.md) để biết chi tiết.

### Bước 2: Cấu hình SLZB-06M

```bash
# Chạy script cấu hình SLZB-06M
bash setup_slzb06m.sh
```

Hoặc cấu hình thủ công qua Web UI:
1. Truy cập `http://<SLZB-06M-IP>`
2. Settings → Network → Enable Thread
3. Set Thread mode: Border Router
4. Save và restart

### Bước 3: Build và Flash

```bash
# Build firmware
idf.py build

# Flash firmware
idf.py -p /dev/ttyUSB0 flash

# Mở serial monitor
idf.py -p /dev/ttyUSB0 monitor
```

## 📖 Sử dụng

### Local Control
- **Rotary Encoder**: Xoay để thay đổi tốc độ quạt
- **Button**: Nhấn để bật/tắt quạt

### Matter Control
1. Commission device qua QR Code hoặc Pairing Code
2. Sử dụng Matter controller (Home Assistant, Apple HomeKit, Google Home)
3. Điều khiển quạt từ xa

## 🔍 Troubleshooting

### Vấn đề 1: Thread không kết nối

**Triệu chứng:**
```
I (xxx) APP_MATTER: Matter start failed
```

**Giải pháp:**
- Kiểm tra SLZB-06M đã enable Thread Border Router
- Kiểm tra firmware SLZB-06M là mới nhất
- Restart SLZB-06M

### Vấn đề 2: Build lỗi

**Triệu chứng:**
```
CMake Error
```

**Giải pháp:**
```bash
idf.py fullclean
rm -rf build
idf.py set-target esp32c6
idf.py build
```

### Vấn đề 3: Flash lỗi

**Triệu chứng:**
```
Serial port not found
```

**Giải pháp:**
```bash
ls -la /dev/ttyUSB*
sudo usermod -a -G dialout $USER
```

## 📚 Tài liệu tham khảo

- [ESP-IDF Thread Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-guides/ieee802154/index.html)
- [Matter over Thread](https://project-chip.github.io/connectedhomeip-doc/thr.html)
- [SMLIGHT SLZB-06M Guide](https://smlight.tech/)
- [ESP-Matter SDK](https://github.com/espressif/esp-matter)

## 📝 Changelog

### v1.0 (2024-04-20)
- Chuyển đổi từ Matter over WiFi sang Matter over Thread
- Thêm Thread network interface
- Thêm Thread diagnostics cluster
- Thêm script cấu hình SLZB-06M
- Thêm testing guide

## 👥 Đóng góp

Mọi đóng góp đều được chào đón. Vui lòng tạo pull request hoặc issue.

## 📄 License

MIT License
