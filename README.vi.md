# ESP32-C6 Matter Fan

<!-- readme-axi type=esp32 -->

[English](README.md) · **Tiếng Việt**

Bộ điều khiển quạt thông minh chạy trên **ESP32-C6**, dùng **Matter over Thread** (ESP-IDF + ESP-Matter, không phải ESPHome).
Quạt hiện trong Home Assistant dưới dạng **Dimmable Light**: thanh trượt 0–100 % được ánh xạ sang tín hiệu CLK 28–328 Hz
để đặt tốc độ cho quạt BLDC điều khiển bằng tần số. Quạt cũng chỉnh được tại chỗ bằng encoder xoay.

> 🇻🇳 Dự án do một người Việt tự xây dựng và duy trì — nếu nó giúp ích cho bạn, hãy
> [ủng hộ dự án](#ủng-hộ-dự-án).

> **Trạng thái: thử nghiệm.** Chạy được trên phần cứng của chính tác giả; khâu build vẫn phải vá tay ESP-Matter.

## Tính năng

- **Matter over Thread** — tham gia mạng Thread có sẵn (ví dụ SMLIGHT SLZB-06M + add-on OpenThread Border Router),
  hoặc tự làm Thread Leader ([README_THREAD_LEADER.md](README_THREAD_LEADER.md))
- **Chỉnh tốc độ** — CLK 28–328 Hz, duty 50 % trên GPIO1, tăng tốc mềm (soft-start) 500 ms
- **Điều khiển tại chỗ** — encoder xoay EC11, 100 nấc (khớp 1:1 với thanh trượt HA); nút nhấn bật/tắt và khôi phục tốc độ gần nhất
- **LED trạng thái** — WS2812 trên GPIO8
- **OTA** — hai slot 3.9 MB trên flash 8 MB

## Phần cứng

| ESP32-C6 | Nối tới |
|---|---|
| GPIO1 | Tín hiệu CLK / PWM của quạt (qua MOSFET IRLZ44N với quạt 12/24 V) |
| GPIO3 / GPIO2 | CLK / DT của encoder |
| GPIO4 | Nút nhấn của encoder |
| GPIO8 | LED trạng thái (WS2812) |

Sơ đồ nối dây đầy đủ và danh sách linh kiện: [WIRING.md](WIRING.md).

## Bắt đầu nhanh

### Build và flash

Đã thử với **ESP-IDF v5.3.1** và **ESP-Matter**, target `esp32c6`.

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

Bản ESP-Matter hiện tại cần vá vài chỗ mới build được trên Windows — xem [BUILD_GUIDE.md](BUILD_GUIDE.md).
Cách khác: [Docker](DOCKER_README.md) hoặc [cài đặt native trên Linux](NATIVE_SETUP_GUIDE.md).

## Home Assistant

### Ghép đôi với Home Assistant

1. Bảo đảm Home Assistant đã có Thread border router và add-on Matter Server.
2. Trong app HA Companion: **Settings → Devices & services → Add Matter device**.
3. Dùng mã ghép đôi thủ công **34970112332** (thông tin test của Matter — hãy đổi trước khi dùng cho sản phẩm thật).

Chi tiết và cách xử lý sự cố: [COMMISSIONING.md](COMMISSIONING.md), [TESTING_GUIDE_THREAD.md](TESTING_GUIDE_THREAD.md).

## Cấu trúc dự án

```
main/            mã nguồn firmware (ứng dụng Matter, bộ điều khiển quạt, encoder, LED, Thread leader)
partitions.csv   bảng phân vùng 8 MB với hai slot OTA
sdkconfig.defaults*  cấu hình build (Thread end device / Thread leader)
```

<!-- support:start -->
## Ủng hộ dự án

<p align="center">
  <img src="https://raw.githubusercontent.com/ngoviet/.github/main/assets/support/banner.svg" width="720" alt="Built and maintained by one person in Việt Nam">
</p>

<p align="center">
  <a href="https://buymeacoffee.com/ngoviet"><img src="https://raw.githubusercontent.com/ngoviet/.github/main/assets/support/btn-coffee.svg" height="56" alt="Buy me a coffee"></a>
  &nbsp;
  <a href="https://github.com/sponsors/ngoviet"><img src="https://raw.githubusercontent.com/ngoviet/.github/main/assets/support/btn-sponsors.svg" height="56" alt="GitHub Sponsors"></a>
</p>

<p align="center">
  <img src="https://raw.githubusercontent.com/ngoviet/.github/main/assets/support/vietqr-card.svg" width="300" alt="VietQR — MB Bank — NGO DUC VIET — 911051212">
</p>

<p align="center"><sub>🇻🇳 VietQR: quét bằng app ngân hàng bất kỳ, nội dung <code>Ung ho esp32-c6-matter-fan</code>. Cảm ơn bạn đã giúp dự án tiếp tục phát triển!</sub></p>
<p align="center"><sub>Tiền mã hoá (mạng BEP20): <code>0xcbf0402b232cb24b4d998f09a4884f87b5f74750</code> · USDT · USDC · BNB · ETH · BTC</sub></p>
<!-- support:end -->

## Giấy phép

[MIT](LICENSE) © Ngô Đức Việt.
