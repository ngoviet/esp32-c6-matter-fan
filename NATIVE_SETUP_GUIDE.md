# Hướng dẫn Cài đặt Môi trường ESP32-C6 Matter Fan (Native Linux)

## 📋 Tổng quan

Script `install_native.sh` đã được tạo sẵn. Bạn cần chạy nó với `sudo`.

## 🚀 Các bước thực hiện

### Bước 1: Chạy script cài đặt

Mở terminal và chạy lệnh sau (nhập password `vokupt` khi được yêu cầu):

```bash
cd /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan
sudo bash install_native.sh
```

Hoặc chạy từng bước riêng biệt bên dưới.

---

## 🔧 Cài đặt từng bước (nếu cần)

### Bước 1: Cài đặt dependencies

```bash
sudo apt-get update
sudo apt-get install -y git curl wget cmake ninja-build python3 python3-pip pipx python3-venv python3-full python3-setuptools libssl-dev libffi-dev pkg-config xz-utils libglib2.0-dev libdbus-1-dev libavahi-client-dev libavahi-common-dev libreadline-dev device-tree-compiler openocd esptool dfu-util udev
```

### Bước 2: Thêm user vào group dialout

```bash
sudo usermod -a -G dialout vokupt
```

**⚠️ QUAN TRỌNG**: Logout và login lại để thay đổi có hiệu lực.

### Bước 3: Cấu hình udev rules cho ESP32

```bash
sudo bash -c 'cat > /etc/udev/rules.d/99-esp32.rules << EOF
# ESP32-C6
SUBSYSTEM=="tty", ATTRS{idVendor}=="303a", MODE="0666"
# CH340
SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="5512", MODE="0666"
# CP2102
SUBSYSTEM=="tty", ATTRS{idVendor}=="10c4", ATTRS{idProduct}=="ea60", MODE="0666"
# FTDI
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", MODE="0666"
EOF'

sudo udevadm control --reload-rules
sudo udevadm trigger
```

### Bước 4: Clone và cài đặt ESP-IDF v5.5.4

```bash
mkdir -p ~/esp
git clone --depth 1 --branch v5.5.4 https://github.com/espressif/esp-idf.git ~/esp/esp-idf
cd ~/esp/esp-idf
./install.sh esp32c6
```

### Bước 5: Clone và cài đặt ESP-Matter SDK

```bash
git clone --depth 1 --recursive https://github.com/espressif/esp-matter.git ~/esp/esp-matter
cd ~/esp/esp-matter
./install.sh
```

### Bước 6: Cấu hình environment variables

Thêm vào `~/.bashrc`:

```bash
cat >> ~/.bashrc << EOF

# ESP-IDF Configuration
export IDF_PATH=$HOME/esp/esp-idf
source $HOME/esp/esp-idf/export.sh

# ESP-Matter Configuration
export ESP_MATTER_PATH=$HOME/esp/esp-matter
EOF
```

Hoặc thêm vào `~/.profile`:

```bash
cat >> ~/.profile << EOF

# ESP-IDF Configuration
export IDF_PATH=$HOME/esp/esp-idf
source $HOME/esp/esp-idf/export.sh

# ESP-Matter Configuration
export ESP_MATTER_PATH=$HOME/esp/esp-matter
EOF
```

Sau đó source lại:

```bash
source ~/.bashrc
```

### Bước 7: Build firmware

```bash
cd /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan
source ~/esp/esp-idf/export.sh
export ESP_MATTER_PATH=$HOME/esp/esp-matter
idf.py fullclean
idf.py build
```

### Bước 8: Flash firmware

```bash
idf.py -p /dev/ttyACM0 flash
```

### Bước 9: Serial monitor

```bash
idf.py -p /dev/ttyACM0 monitor
```

Để thoát khỏi monitor, nhấn `Ctrl+]`.

---

## ✅ Kiểm tra kết quả

Sau khi cài đặt, kiểm tra các thành phần:

```bash
# Kiểm tra ESP-IDF
idf.py --version

# Kiểm tra environment
echo $IDF_PATH
echo $ESP_MATTER_PATH

# Kiểm tra firmware
ls -la /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan/build/esp32_c6_matter_fan.bin
```

---

## 🔍 Troubleshooting

### Lỗi 1: Permission denied khi truy cập /dev/ttyACM0

```bash
sudo usermod -a -G dialout $USER
sudo chmod 666 /dev/ttyACM0
```

**Logout và login lại** để áp dụng.

### Lỗi 2: ESP_MATTER_PATH not set

```bash
export ESP_MATTER_PATH=$HOME/esp/esp-matter
```

Thêm vào `~/.bashrc` để persistent.

### Lỗi 3: Cannot find esp-matter components

```bash
cd ~/esp/esp-matter
./install.sh
```

### Lỗi 4: Port not found

```bash
# Kiểm tra port
ls -la /dev/ttyACM* /dev/ttyUSB*

# Nếu là /dev/ttyUSB0, thay thế trong lệnh flash
idf.py -p /dev/ttyUSB0 flash
```

### Lỗi 5: Build failed - cmake not found

```bash
sudo apt-get install -y cmake ninja-build
```

---

## 📁 Cấu trúc sau cài đặt

```
/home/vokupt/
├── esp/
│   ├── esp-idf/          # ESP-IDF v5.5.4
│   └── esp-matter/       # ESP-Matter SDK
└── Downloads/esp32_smart_fan/
    └── esp32_c6_matter_fan/
        ├── build/
        │   └── esp32_c6_matter_fan.bin  # Firmware đã build
        └── install_native.sh
```

---

## 🎯 Lệnh thường dùng

```bash
# Build firmware
cd /home/vokupt/Downloads/esp32_smart_fan/esp32_c6_matter_fan
source ~/esp/esp-idf/export.sh
idf.py build

# Flash firmware
idf.py -p /dev/ttyACM0 flash

# Build + Flash
idf.py build flash

# Serial monitor
idf.py -p /dev/ttyACM0 monitor

# Clean build
idf.py fullclean

# Erase flash
idf.py erase-flash
```
