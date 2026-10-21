# ESP32-C6 Matter Fan

> **Status: experimental.** Works on my own hardware; build setup still needs manual patches to ESP-Matter.

Smart fan controller on **ESP32-C6** using **Matter over Thread** (ESP-IDF + ESP-Matter, not ESPHome).
The fan shows up in Home Assistant as a **Dimmable Light**: the 0–100 % slider maps to a 28–328 Hz CLK signal
that sets the speed of a frequency-controlled BLDC fan. It can also be controlled locally with a rotary encoder.

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

## Build & flash

Tested with **ESP-IDF v5.3.1** and **ESP-Matter**, target `esp32c6`.

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

The current ESP-Matter release needs several patches before it builds on Windows — see [BUILD_GUIDE.md](BUILD_GUIDE.md).
Alternatives: [Docker](DOCKER_README.md) or [native Linux setup](NATIVE_SETUP_GUIDE.md).

## Pair with Home Assistant

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

## License

[MIT](LICENSE)
