# BÁO CÁO KIỂM TRA CODEBASE - ESP32-C6 SMART FAN

**Ngày kiểm tra:** 2026-04-22
**Thiết bị mục tiêu:** ESP32-C6
**Công nghệ:** Matter over Thread (802.15.4)

---

## 1. TỔNG QUAN DỰ ÁN

### 1.1. Mục đích
Project triển khai ESP32-C6 Smart Fan với Matter over Thread, kết nối đến SMLIGHT SLZB-06M (Thread Border Router).

### 1.2. Kiến trúc hệ thống
```
┌─────────────────────────────────────────────────────┐
│           SMLIGHT SLZB-06M                           │
│         Thread Border Router                         │
└──────────────────────┬──────────────────────────────┘
                       │ Thread Network (802.15.4)
                       │
┌──────────────────────┼──────────────────────────────┐
│                      ▼                              │
│          ESP32-C6 Smart Fan                          │
│  ┌──────────────────────────────┐                   │
│  │ Matter Stack (Thread)        │                   │
│  │  - Fan Control Cluster       │                   │
│  │  - On/Off Cluster            │                   │
│  │  - Level Control Cluster     │                   │
│  │  - Thread Diagnostics        │                   │
│  └─────────────┬────────────────┘                   │
│                │                                     │
│  ┌─────────────┴────────────────┐                   │
│  │ System Manager               │                   │
│  │  - Fan Controller (LEDC/PWM) │                   │
│  │  - Rotary Encoder (GPIO)     │                   │
│  │  - Button (GPIO + Debounce)  │                   │
│  └──────────────────────────────┘                   │
└─────────────────────────────────────────────────────┘
```

### 1.3. Cấu trúc thư mục
```
esp32_c6_matter_fan/
├── CMakeLists.txt                    # Main build config
├── main/
│   ├── CMakeLists.txt                # Component build config
│   ├── main.cpp                      # Entry point
│   ├── app_matter.cpp/h              # Matter implementation
│   ├── app_thread_leader.cpp/h       # Thread Leader (DISABLED)
│   ├── system_manager.h              # System coordinator
│   ├── fan_controller.h              # PWM fan control
│   ├── rotary_encoder.h              # Encoder driver
│   ├── button.h                      # Button driver
│   └── config.h                      # GPIO/config constants
├── sdkconfig.defaults                # Default config
├── partitions.csv                    # Flash partition table
├── managed_components/              # ESP-IDF components
└── esptool_bin/                     # Flashing tools
```

---

## 2. PHÂN TÍCH CHI TIẾT TỪNG MODULE

### 2.1. [`main.cpp`](main/main.cpp:1) - Entry Point
**Dòng:** 212 dòng
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- Khởi tạo NVS flash storage
- Khởi tạo network stack (ESP-NETIF)
- Khởi tạo IEEE 802.15.4 radio
- Khởi tạo GPIO ISR service
- Khởi tạo System Manager
- Khởi tạo Matter over Thread

**Đánh giá:**
- Code được tổ chức rõ ràng với 7 bước khởi tạo
- Có xử lý lỗi cho từng bước
- Thread Leader mode đang bị comment (line 43)

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Lambda callback trong esp_event_handler_instance_register | ⚠️ Trung bình | Sử dụng C++ lambda (line 60-68) nhưng hàm ESP-IDF expect C function pointer. Có thể gây vấn đề portability. |

---

### 2.2. [`app_matter.cpp/h`](main/app_matter.cpp:1) - Matter over Thread
**Dòng:** 353 dòng (cpp) + 54 dòng (h)
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- Tạo Matter node với Fan endpoint
- Thêm các cluster: On/Off, Fan Control, Level Control
- Xử lý attribute update callback
- Network commissioning endpoint cho Thread
- Thread Network Diagnostics endpoint
- Report state về Matter controller

**Đánh giá:**
- Implementation đầy đủ các cluster Matter cần thiết
- Có callback cho attribute update và identification
- Tích hợp tốt với Fan Controller

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Hardcoded path trong CMakeLists.txt | 🔴 Cao | Line 4-10 CMakeLists.txt gốc hardcode path `/home/vokupt/esp/...` |
| 2 | Callback type mismatch | ⚠️ Trung bình | `app_attribute_update_cb` và `app_identification_cb` là C++ non-static function nhưng được truyền cho API expect function pointer |
| 3 | Missing return value | ⚠️ Trung bình | `app_matter_init()` không return error code, khó debug khi fail |

---

### 2.3. [`app_thread_leader.cpp/h`](main/app_thread_leader.cpp:1) - Thread Leader (DISABLED)
**Dòng:** 386 dòng (cpp) + 105 dòng (h)
**Trạng thái:** 🔴 Bị disable trong build

**Chức năng chính (khi được enable):**
- Tạo Thread network Leader
- Border Router functionality
- NVS configuration persistence
- Master Key generation
- Network state management

**Đánh giá:**
- Code được implement đầy đủ nhưng chưa được enable
- Có NVS storage cho Thread config
- Có callback cho state changed events

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Master key hardcoded default | 🟡 Thấp | Default master key trong line 32-33 có thể thay đổi cho production |
| 2 | esp_random_fill không tồn tại | 🔴 Cao | `esp_random_fill()` (line 46) có thể không có trong ESP-IDF hiện tại |
| 3 | Buffer overflow trong master_key_to_string | 🔴 Cao | Line 113: `output[32]` vượt quá buffer 32 bytes (index 0-31) |
| 4 | Component không được include | 🔴 Cao | Line 13-17 trong main/CMakeLists.txt bị comment, component không được build |

---

### 2.4. [`system_manager.h`](main/system_manager.h:1) - System Coordinator
**Dòng:** 173 dòng
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- Điều phối Fan Controller, Rotary Encoder, Button
- FreeRTOS Queue cho event handling từ ISR
- Event loop task xử lý tất cả sự kiện
- Tích hợp với Matter reporting

**Đánh giá:**
- Sử dụng design pattern tốt (singleton + observer)
- Queue-based event handling an toàn cho ISR context
- Static callback wrapper cho C++ member functions

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Comment tiếng Việt trong code | 🟡 Thấp | Line 57-60 có TODO comment về xQueueSendFromISR nhưng code đang dùng xQueueSendFromISR đúng ở line 86 |
| 2 | Comment không đồng nhất | 🟡 Thấp | Line 57-59 có comment nói "Tạm thời tôi sẽ gọi..." nhưng thực tế code đã đúng |
| 3 | Không có shutdown mechanism | ⚠️ Trung bình | `shutdown()` method tồn tại nhưng không được gọi trong lifecycle |

---

### 2.5. [`fan_controller.h`](main/fan_controller.h:1) - PWM Fan Control
**Dòng:** 118 dòng
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- LEDC (PWM) configuration
- Speed control 0-100%
- On/Off control
- Frequency range 100-400 Hz

**Đánh giá:**
- Sử dụng LEDC driver đúng cách
- Duty cycle 50% cho square wave (phù hợp cho fan)
- Có bounds checking cho speed percentage

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Frequency vs Duty confusion | ⚠️ Trung bình | Comment line 81 nói "50% duty = 4096" nhưng với 13-bit resolution, 50% thực tế là 2^12 = 4096 (đúng) |
| 2 | Không có soft-start | 🟡 Thấp | Fan bật ngay lập tức ở tốc độ mong muốn, có thể gây surge current |

---

### 2.6. [`rotary_encoder.h`](main/rotary_encoder.h:1) - Encoder Driver
**Dòng:** 112 dòng
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- GPIO interrupt handling cho CLK/DT
- Direction detection (forward/reverse)
- Step counting với bounds checking
- Callback mechanism

**Đánh giá:**
- Sử dụng ISR cho responsive input
- Logic direction detection đúng chuẩn Gray code
- Bounds checking cho step value

**Vấn đề phát hiện:**
| STT | Vấn đề | Mức độ | Mô tả |
|-----|--------|--------|-------|
| 1 | Callback trong ISR context | 🔴 Cao | Line 104-106: callback được gọi trực tiếp trong ISR (`handle_interrupt()` có `IRAM_ATTR`). Callback có thể thực hiện thao tác dài gây mất response. |
| 2 | Không có debounce cho encoder | ⚠️ Trung bình | Rotary encoder cơ có thể gây bounce, không có cơ chế debounce |
| 3 | Logic direction detection phức tạp | 🟡 Thấp | Logic line 83-95 có thể đơn giản hơn bằng cách chỉ kiểm tra rising edge |

---

### 2.7. [`button.h`](main/button.h:1) - Button Driver
**Dòng:** 92 dòng
**Trạng thái:** ✅ Hoạt động

**Chức năng chính:**
- GPIO interrupt cho button press
- esp_timer-based debounce
- Callback mechanism

**Đánh giá:**
- Sử dụng esp_timer cho debounce (tốt hơn delay trong ISR)
- Timer callback chạy trong task context (an toàn)
- Negative edge trigger phù hợp với pull-up

**Đánh giá:** Module tốt, không có vấn đề nghiêm trọng.

---

### 2.8. [`config.h`](main/config.h:1) - Configuration
**Dòng:** 36 dòng
**Trạng thái:** ✅ Hoạt động

**Thông tin cấu hình:**
| Parameter | Value | Ghi chú |
|-----------|-------|---------|
| PWM Output | GPIO_NUM_1 | LEDC Channel 0 |
| Encoder CLK | GPIO_NUM_2 | Interrupt pin |
| Encoder DT | GPIO_NUM_3 | Input |
| Encoder SW | GPIO_NUM_4 | Button (interrupt) |
| Min Frequency | 100 Hz | Fan min speed |
| Max Frequency | 400 Hz | Fan max speed |
| LEDC Resolution | 13-bit | 8192 steps |
| Encoder Steps | 33 | ~3% per step |
| Button Debounce | 50 ms | |

---

## 3. BUILD CONFIGURATION

### 3.1. [`CMakeLists.txt`](CMakeLists.txt:1) - Root Build Config
**Vấn đề nghiêm trọng:**
- 🔴 **Hardcoded paths** (line 4, 7, 10):
  - `ESP_MATTER_PATH = "/home/vokupt/esp/esp-matter"`
  - `ESP_IDF_PATH = "/home/vokupt/esp/esp-idf"`
  - `MATTER_SDK_PATH = "${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip"`
- Các path này chỉ hoạt động trên máy của tác giả

### 3.2. [`main/CMakeLists.txt`](main/CMakeLists.txt:1) - Component Build Config
**Vấn đề:**
- Thread Leader component bị comment (line 13-17)
- Chỉ include `main.cpp` và `app_matter.cpp`
- Missing component: `esp_openthread` cho Thread functionality

### 3.3. [`sdkconfig.defaults`](sdkconfig.defaults:1)
**Cấu hình hiện tại:**
- ✅ IEEE 802.15.4 enabled
- ✅ WiFi disabled
- ✅ Bluetooth disabled
- ✅ Matter Thread enabled
- ✅ NVS partition enabled

### 3.4. [`partitions.csv`](partitions.csv:1)
**Partition table:**
| Partition | Type | Size | Ghi chú |
|-----------|------|------|---------|
| nvs | data | 24K | NVS storage |
| phy_init | data | 4K | PHY init data |
| factory | app | 1920K | Factory firmware |

**Tổng:** 2MB flash (phù hợp cho ESP32-C6)

---

## 4. MANAGED COMPONENTS

Các component chính trong `managed_components/`:
- `espressif__esp_delta_ota` - Delta OTA updates
- `espressif__esp_encrypted_img` - Encrypted image support
- `espressif__esp_rcp_update` - RCP update support
- `espressif__jsmn` - JSON parser
- `espressif__json_generator` - JSON generation
- `espressif__mdns` - mDNS discovery

---

## 5. TỔNG HỢP VẤN ĐỀ

### Vấn đề Nghiêm trọng (🔴 Cần sửa ngay)
| STT | Vấn đề | File | Ảnh hưởng |
|-----|--------|------|-----------|
| 1 | Hardcoded paths trong CMakeLists.txt | CMakeLists.txt:4-10 | Code không build được trên máy khác |
| 2 | Callback trong ISR context | rotary_encoder.h:104 | Có thể gây system hang nếu callback dài |
| 3 | Buffer overflow | app_thread_leader.cpp:113 | Memory corruption |
| 4 | esp_random_fill() không tồn tại | app_thread_leader.cpp:46 | Build error |

### Vấn đề Trung bình (⚠️ Nên sửa)
| STT | Vấn đề | File | Ảnh hưởng |
|-----|--------|------|-----------|
| 1 | Lambda callback với ESP-IDF event handler | main.cpp:60 | Portability issue |
| 2 | Missing return value trong app_matter_init() | app_matter.cpp:188 | Khó debug |
| 3 | Không có shutdown mechanism | system_manager.h:94 | Resource leak |
| 4 | Không có debounce cho encoder | rotary_encoder.h | Input noise |

### Vấn đề Thấp (🟡 Nên cải thiện)
| STT | Vấn đề | File | Ảnh hưởng |
|-----|--------|------|-----------|
| 1 | Comment tiếng Việt lẫn Anh | Nhiều file | Code consistency |
| 2 | Hardcoded default master key | app_thread_leader.cpp:32 | Security concern |
| 3 | Không có soft-start cho fan | fan_controller.h | Surge current |

---

## 6. ĐỀ XUẤT CẢI TIẾN

### 6.1. Immediate Actions (Ưu tiên cao)
1. **Sửa hardcoded paths**: Sử dụng environment variable hoặc file config
2. **Sửa callback trong ISR**: Chỉ dùng xQueueSendFromISR, không gọi callback trực tiếp
3. **Sửa buffer overflow**: Tăng buffer size hoặc dùng snprintf
4. **Sửa esp_random_fill**: Dùng esp_random() thay thế

### 6.2. Short-term Improvements
1. Thêm error handling cho network initialization
2. Thêm watchdog timer cho hệ thống
3. Implement soft-start cho fan
4. Thêm debounce cho rotary encoder

### 6.3. Long-term Enhancements
1. Enable Thread Leader mode khi ESP-IDF v5.5+ ổn định
2. Thêm OTA update support
3. Thêm telemetry và diagnostics
4. Implement secure boot và flash encryption

---

## 7. KẾT LUAN

Project có kiến trúc tốt với sự phân tách module rõ ràng. Tuy nhiên có một số vấn đề nghiêm trọng cần sửa trước khi deploy:
- Hardcoded paths ngăn build trên môi trường khác
- Callback trong ISR context có thể gây system instability
- Một số buffer overflow trong Thread Leader code

**Điểm mạnh:**
- Kiến trúc module rõ ràng
- Sử dụng FreeRTOS Queue cho ISR-safe event handling
- đầy đủ Matter clusters cho fan control
- Good separation of concerns

**Điểm yếu:**
- Hardcoded paths
- ISR callback issues
- Missing error handling trong một số trường hợp
