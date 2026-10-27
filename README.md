# ESP32-C6 Matter Fan

<!-- readme-axi type=esp32 -->

**English** · [Tiếng Việt](README.vi.md)

Smart fan controller on **ESP32-C6** using **Matter over Thread** (ESP-IDF + ESP-Matter, not ESPHome).
The fan shows up in Home Assistant as a **Dimmable Light**: the 0–100 % slider maps to a 28–328 Hz CLK signal
that sets the speed of a frequency-controlled BLDC fan. It can also be controlled locally with a rotary encoder.

> 🇻🇳 Built and maintained by one person in Việt Nam — if it saves you time, please
> [support the project](#support-the-project).

> **Status: experimental.** Works on my own hardware; build setup still needs manual patches to ESP-Matter.

## Features

- **Matter over Thread** — joins an existing Thread network (e.g. SMLIGHT SLZB-06M + OpenThread Border Router add-on),
  or runs as its own Thread Leader ([README_THREAD_LEADER.md](README_THREAD_LEADER.md))
- **Speed control** — 28–328 Hz CLK, 50 % duty on GPIO1, 500 ms soft-start ramp
- **Local control** — EC11 rotary encoder, 100 steps (1:1 with the HA slider); push button toggles on/off and restores the last speed
- **Status LED** — WS2812 on GPIO8
- **OTA** — two 3.9 MB slots on 8 MB flash

## Hardware

| ESP32-C6 | Connects to |
|---|---|
| GPIO1 | Fan CLK / PWM signal (via IRLZ44N MOSFET for 12/24 V fans) |
| GPIO3 / GPIO2 | Encoder CLK / DT |
| GPIO4 | Encoder push button |
| GPIO8 | Status LED (WS2812) |

Full wiring and parts list: [WIRING.md](WIRING.md).

## Quick start

### Build & flash

Tested with **ESP-IDF v5.3.1** and **ESP-Matter**, target `esp32c6`.

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

The current ESP-Matter release needs several patches before it builds on Windows — see [BUILD_GUIDE.md](BUILD_GUIDE.md).
Alternatives: [Docker](DOCKER_README.md) or [native Linux setup](NATIVE_SETUP_GUIDE.md).

## Home Assistant

### Pair with Home Assistant

1. Make sure Home Assistant has a Thread border router and the Matter Server add-on.
2. In the HA Companion app: **Settings → Devices & services → Add Matter device**.
3. Use the manual pairing code **34970112332** (Matter test credentials — change them before using in a real product).

Details and troubleshooting: [COMMISSIONING.md](COMMISSIONING.md), [TESTING_GUIDE_THREAD.md](TESTING_GUIDE_THREAD.md).

## Project layout

```
main/            firmware source (Matter app, fan controller, encoder, LED, Thread leader)
partitions.csv   8 MB partition table with two OTA slots
sdkconfig.defaults*  build configuration (Thread end device / Thread leader)
```

<!-- support:start -->
## Support the project

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

<p align="center"><sub>🇻🇳 VietQR: scan with any Vietnamese banking app, transfer note <code>Ung ho esp32-c6-matter-fan</code>. Thank you for keeping this project alive!</sub></p>
<p align="center"><sub>Crypto (BEP20 network): <code>0xcbf0402b232cb24b4d998f09a4884f87b5f74750</code> · USDT · USDC · BNB · ETH · BTC</sub></p>
<!-- support:end -->

## License

[MIT](LICENSE) © Ngô Đức Việt.
