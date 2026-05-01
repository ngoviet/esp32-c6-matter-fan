# ESP32-C6 Smart Fan - Thread Leader Mode

## 📋 Tổng quan

Chế độ **Thread Leader** cho phép ESP32-C6 **TẠO VÀ QUẢN LÝ** mạng Thread của riêng nó, hoạt động như một **Thread Border Router** và **Leader**.

### So sánh hai chế độ

| Tính năng | Thread Join Mode | Thread Leader Mode |
|-----------|------------------|-------------------|
| **Vai trò ESP32-C6** | Thread End Device | Thread Leader + Border Router |
| **Border Router** | SMLIGHT SLZB-06M | ESP32-C6 |
| **Tạo mạng** | Join network có sẵn | CREATE network mới |
| **Thiết bị khác** | Join vào SLZB-06M | Join vào ESP32-C6 |
| **Cần SLZB-06M** | Có | Không |

### Kiến trúc Thread Leader Mode

```
┌─────────────────────────────────────────────────────┐
│           ESP32-C6 Smart Fan                        │
│    ┌──────────────────────────────────┐             │
│    │  Thread LEADER (Border Router)   │             │
│    │  - Tạo Thread Network            │             │
│    │  - Accept device joins           │             │
│    │  - DHCP Server + NAT64           │             │
│    └────────────┬─────────────────────┘             │
│                 │ Thread Network (802.15.4)         │
│                 ▼                                   │
│    ┌──────────────────────────────────┐             │
│    │  Matter Stack                    │             │
│    │  - Fan Control                   │             │
│    │  - On/Off                        │             │
│    └──────────────────────────────────┘             │
│                                                     │
│    ┌──────────────────────────────────┐             │
│    │  System Manager                  │             │
│    │  (Rotary Encoder + Button)       │             │
│    └──────────────────────────────────┘             │
└─────────────────────────────────────────────────────┘
                │
                ▼
        ┌───────────────┐
        │ Thread Devices│
        │ (Matter End   │
        │  Devices)     │
        └───────────────┘
```

## 🎯 Tính năng

- ✅ **Thread Leader**: ESP32-C6 tạo và quản lý Thread network
- ✅ **Border Router**: DHCP Server, DNS Forwarder, NAT64
- ✅ **Matter over Thread**: Điều khiển quạt qua Matter
- ✅ **Device Join**: Các Thread device khác có thể join vào
- ✅ **Local Control**: Rotary encoder và button
- ✅ **NVS Storage**: Lưu cấu hình Thread network
- ✅ **Auto Recovery**: Tự động khôi phục network sau reboot

## 📂 Cấu trúc file

```
esp32_c6_matter_fan/
├── sdkconfig.defaults              # Thread Join mode (default)
├── sdkconfig.defaults.thread_leader # Thread Leader mode
├── main/
│   ├── main.cpp                    # Entry point (hỗ trợ cả 2 mode)
│   ├── app_matter.cpp              # Matter implementation
│   ├── app_matter.h
│   ├── app_thread_leader.cpp       # Thread Leader implementation (MỚI)
│   ├── app_thread_leader.h         # Thread Leader interface (MỚI)
│   └── ...
├── README_THREAD.md                # Thread Join mode guide
└── README_THREAD_LEADER.md         # File này - Thread Leader guide
```

## 🚀 Cài đặt

### Bước 1: Chọn chế độ Thread

#### Thread Leader Mode (ESP32-C6 tạo network)

```bash
cd esp32_c6_matter_fan

# Copy file cấu hình Thread Leader
cp sdkconfig.defaults.thread_leader sdkconfig.defaults

# Hoặc dùng idf.py menuconfig
idf.py menuconfig
# Chọn:
# Component config → ESP-Thread-BR → Enable Thread Border Router
# Component config → OpenThread → Role → Leader
```

#### Thread Join Mode (Join SLZB-06M network)

```bash
# Sử dụng file cấu hình mặc định
# (không cần copy gì cả)
```

### Bước 2: Build firmware

```bash
cd esp32_c6_matter_fan

# Xóa build cũ
idf.py fullclean

# Set target
idf.py set-target esp32c6

# Build
idf.py build
```

### Bước 3: Flash firmware

```bash
# Nạp firmware vào ESP32-C6
idf.py -p /dev/ttyUSB0 flash

# Mở serial monitor
idf.py -p /dev/ttyUSB0 monitor
```

## 📖 Sử dụng

### Khởi động Thread Leader Mode

Khi ESP32-C6 khởi động ở chế độ Thread Leader, bạn sẽ thấy:

```
I (xxx) MAIN ========================================
I (xxx) MAIN THREAD LEADER MODE ENABLED
I (xxx) MAIN ========================================
I (xxx) THREAD_LEADER Initializing Thread Leader Mode
I (xxx) THREAD_LEADER Thread Network Configuration:
I (xxx) THREAD_LEADER   Network Name: ESP32-C6-FAN
I (xxx) THREAD_LEADER   PAN ID: 0xABCD
I (xxx) THREAD_LEADER   Channel: 15
I (xxx) THREAD_LEADER   Master Key: 00112233445566778899aabbccddeeff
I (xxx) THREAD_LEADER ✓ Thread LEADER is ACTIVE!
I (xxx) THREAD_LEADER Other Thread devices can now join this network
```

### Kết nối Thread device mới

Các Thread device mới có thể join vào mạng do ESP32-C6 tạo:

1. **Sử dụng Thread Commissioner** (ví dụ: SMLIGHT SLZB-06M)
   ```
   - Truy cập Web UI của SLZB-06M
   - Settings → Thread → Commissioner
   - Enable Commissioner
   - Commission new device
   ```

2. **Sử dụng Matter Controller**
   ```
   - Mở Matter controller (Home Assistant, SmartThings)
   - Add device → Scan QR code hoặc nhập pairing code
   - Device sẽ join vào Thread network của ESP32-C6
   ```

### Cấu hình Thread Network

Cấu hình mặc định:
- **Network Name**: `ESP32-C6-FAN`
- **PAN ID**: `0xABCD`
- **Channel**: `15`
- **Master Key**: `00112233445566778899aabbccddeeff`

Thay đổi cấu hình trong [`sdkconfig.defaults.thread_leader`](sdkconfig.defaults.thread_leader):

```ini
# Thread PAN ID
CONFIG_OPENTHREAD_PAN_ID=0xABCD

# Thread Channel
CONFIG_OPENTHREAD_CHANNEL=15

# Thread Network Name
CONFIG_OPENTHREAD_NETWORK_NAME="ESP32-C6-FAN"

# Master Key (16 bytes, hex)
CONFIG_OPENTHREAD_MASTER_KEY="00112233445566778899aabbccddeeff"
```

### Factory Reset

Để reset Thread network về mặc định:

```cpp
// Trong code
app_thread_leader_factory_reset();
```

Hoặc xóa NVS:

```bash
idf.py -p /dev/ttyUSB0 erase-flash
```

## 🔧 Troubleshooting

### Vấn đề 1: Thread Leader không trở thành Leader

**Triệu chứng:**
```
W (xxx) THREAD_LEADER Thread started but Leader role not yet assigned
```

**Giải pháp:**
- Đợi 10-30 giây để Leader election hoàn tất
- Kiểm tra `CONFIG_OPENTHREAD_ROLE_LEADER=y` trong menuconfig
- Restart thiết bị

### Vấn đề 2: Device không join được vào Thread network

**Triệu chứng:**
```
E (xxx) OPENTHREAD Device join failed
```

**Giải pháp:**
- Kiểm tra Master Key của device khớp với network
- Đảm bảo device và ESP32-C6 cùng Thread channel
- Kiểm tra signal strength (đảm bảo gần nhau)

### Vấn đề 3: Build lỗi OpenThread

**Triệu chứng:**
```
CMake Error: Unknown option: --enable-leader
```

**Giải pháp:**
```bash
idf.py fullclean
rm -rf build
idf.py set-target esp32c6
idf.py menuconfig  # Chọn Thread Leader options
idf.py build
```

### Vấn đề 4: Thread network không khởi động

**Triệu chứng:**
```
E (xxx) THREAD_LEADER Failed to initialize OpenThread Border Router
```

**Giải pháp:**
- Kiểm tra IEEE 802.15.4 đã được enable:
  ```
  CONFIG_IEEE802154_ENABLED=y
  ```
- Xóa NVS và rebuild:
  ```bash
  idf.py erase-flash
  idf.py build flash
  ```

## 📊 Thread Network Information

### Các trạng thái Thread

| State | Mô tả |
|-------|-------|
| `DISCONNECTED` | Chưa kết nối Thread network |
| `SECURITY_ERROR` | Lỗi bảo mật (Master Key sai) |
| `NO_PARENT` | Chưa tìm được Parent |
| `ROLE_LEADER` | ✅ ESP32-C6 là Thread Leader |
| `ROLE_ROUTER` | Đã là Router trong network |
| `ROLE_CHILD` | Là End Device (child) |
| `ROLE_DETACHED` | Đã tách khỏi network |

### Thread Channels

| Channel | Frequency |
|---------|-----------|
| 11 | 2405 MHz |
| 12 | 2410 MHz |
| ... | ... |
| 15 | 2425 MHz (mặc định) |
| ... | ... |
| 26 | 2480 MHz |

## 📚 API Reference

### app_thread_leader.h

```cpp
// Khởi tạo Thread Leader
esp_err_t app_thread_leader_init(void);

// Dừng Thread Leader
esp_err_t app_thread_leader_stop(void);

// Lấy thông tin network
esp_err_t app_thread_leader_get_info(thread_leader_info_t *info);

// Cấu hình network
esp_err_t app_thread_leader_set_config(
    const char *network_name,
    uint16_t pan_id,
    uint8_t channel,
    const uint8_t *master_key
);

// Factory reset
esp_err_t app_thread_leader_factory_reset(void);

// Kiểm tra trạng thái
bool app_thread_leader_is_running(void);
bool app_thread_leader_is_leader(void);
```

## 📚 Tài liệu tham khảo

- [ESP-IDF OpenThread Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-guides/ieee802154/index.html)
- [Thread Specification](https://threadgroup.org/)
- [Matter over Thread](https://project-chip.github.io/connectedhomeip-doc/thr.html)
- [OpenThread Documentation](https://openthread.io/guides)

## 📝 Changelog

### v1.0 (2026-04-21)
- Thêm Thread Leader mode
- Tạo `app_thread_leader.cpp/h`
- Hỗ trợ cả 2 chế độ: Leader và Join
- Thêm NVS storage cho Thread config
- Thêm documentation

## 👥 Đóng góp

Mọi đóng góp đều được chào đón. Vui lòng tạo pull request hoặc issue.

## 📄 License

MIT License
