# 🐳 Hướng dẫn sử dụng Docker cho ESP32-C6 Matter Fan

## 📋 Yêu cầu hệ thống

- **Windows 11** (hoặc Windows 10 Pro)
- **Docker Desktop** (version 4.0+)
- **WSL 2** đã được kích hoạt
- **ESP32-C6** được kết nối qua USB (nhận là COM3)

## 🚀 Cài đặt nhanh

### Bước 1: Cài đặt Docker Desktop

1. Tải Docker Desktop từ: https://www.docker.com/products/docker-desktop/
2. Chạy installer và làm theo hướng dẫn
3. Khởi động lại máy nếu cần
4. Mở Docker Desktop và đợi đến khi trạng thái là **Running**

### Bước 2: Kiểm tra Docker hoạt động

Mở PowerShell và chạy:
```powershell
docker --version
docker run hello-world
```

Nếu thấy "Hello from Docker!" → Thành công! ✅

## 🛠️ Sử dụng script esp_build.bat

Script này giúp bạn build, flash và monitor dễ dàng.

### Các lệnh có thể dùng:

| Lệnh | Mô tả |
|------|-------|
| `esp_build.bat` | Build + Flash + Monitor (đầy đủ) |
| `esp_build.bat build` | Chỉ build firmware |
| `esp_build.bat flash` | Chỉ flash vào ESP32 |
| `esp_build.bat fullflash` | Build + Flash tự động |
| `esp_build.bat monitor` | Chỉ xem log serial |
| `esp_build.bat clean` | Xóa build cache |

### Cách sử dụng:

1. Mở **Command Prompt** hoặc **PowerShell**
2. Di chuyển vào thư mục dự án:
   ```cmd
   cd D:\HA\esp32_smart_fan\esp32_c6_matter_fan
   ```
3. Chạy script:
   ```cmd
   esp_build.bat
   ```

## 🐳 Sử dụng docker-compose trực tiếp

### Build firmware
```cmd
docker-compose run --rm build
```

### Flash vào ESP32 (COM3)
```cmd
docker run --rm -v %cd%:/project --device COM3:/dev/ttyS0 espressif/idf:release-v5.5 idf.py -p /dev/ttyS0 flash
```

### Xem serial monitor
```cmd
docker run --rm -it -v %cd%:/project --device COM3:/dev/ttyS0 espressif/idf:release-v5.5 idf.py -p /dev/ttyS0 monitor
```

### Clean build
```cmd
docker-compose run --rm clean
```

## ⚠️ Xử lý lỗi thường gặp

### Lỗi 1: Docker không chạy
```
error while validating the Info context: Can't connect to Docker daemon
```
**Giải pháp:** Khởi động Docker Desktop

### Lỗi 2: Không truy cập được COM3
```
error while opening serial port connection: trying to open tty device
```
**Giải pháp:** 
- Kiểm tra ESP32 đã được nhận trong Device Manager
- Đảm bảo cổng là COM3 (hoặc sửa trong script nếu khác)
- Cài driver CH340/CP2102 cho ESP32

### Lỗi 3: Build chậm hoặc hết dung lượng
```cmd
# Xóa cache Docker
docker system prune -a

# Kiểm tra dung lượng
docker system df
```

### Lỗi 4: WSL2 không hoạt động
```cmd
# Trong PowerShell (Admin)
wsl --update
wsl --set-default-version 2
wsl --shutdown
```

## 📂 Cấu trúc project với Docker

```
esp32_c6_matter_fan/
├── Dockerfile              # Cấu hình Docker image
├── docker-compose.yml      # Dịch vụ Docker (build, flash, monitor)
├── esp_build.bat           # Script tiện ích Windows
├── main/                   # Source code
│   ├── main.cpp
│   ├── config.h
│   ├── fan_controller.h
│   ├── rotary_encoder.h
│   ├── button.h
│   └── system_manager.h
└── build/                  # Output build (được mount từ container)
    └── esp32_c6_matter_fan.bin  # Firmware đã build
```

## 🔧 Tùy chỉnh

### Thay đổi cổng COM
Nếu ESP32 được nhận là cổng khác COM3 (ví dụ COM4):
- Sửa trong `esp_build.bat`: thay `COM3` thành `COM4`
- Hoặc trong `docker-compose.yml`: thay `COM3:/dev/ttyS0` thành `COM4:/dev/ttyS0`

### Sử dụng Docker CLI trực tiếp
Nếu bạn muốn tương tác trực tiếp với container:
```cmd
# Vào container
docker run -it -v %cd%:/project --workdir /project espressif/idf:release-v5.5 bash

# Chạy idf.py trong container
idf.py build
idf.py -p /dev/ttyS0 flash
```

## ✅ Kiểm tra sau khi flash

Sau khi flash thành công, mở serial monitor để xem log:
```cmd
esp_build.bat monitor
```

Bạn sẽ thấy output tương tự:
```
I (xxx) SYSTEM: Starting ESP32-C6 Smart Fan Application...
I (xxx) SYSTEM: System is ready. Use the Rotary Encoder and Button to control the fan.
I (xxx) SYSTEM: Heartbeat - System is running...
```

## 📞 Hỗ trợ

Nếu gặp vấn đề, hãy kiểm tra:
1. Docker Desktop đang chạy
2. ESP32 được kết nối và nhận là cổng COM
3. WSL 2 đã được cài đặt
4. Dung lượng disk đủ (>30GB)

---
**Chúc bạn thành công!** 🎉
