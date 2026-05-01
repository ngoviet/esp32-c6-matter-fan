# ESP32-C6 Matter Fan — Project Context

## Project Overview

Smart fan controller built on **ESP32-C6** using **Matter over Thread** (802.15.4). The device controls a PWM-driven fan via a rotary encoder and button locally, while exposing On/Off, Fan Control, and Level Control clusters over Matter for remote control. Network connectivity runs exclusively over Thread — **no WiFi, no Bluetooth**. The device connects to an **SMLIGHT SLZB-06M** Thread Border Router.

**Working build:** ESP-IDF v5.3.1 | **Target:** esp32c6 | **Flash size:** 2 MB (8 MB actual)

## Working Build & Flash Commands

```powershell
# PowerShell — build light example (proven working)
$env:IDF_PATH = "D:/Espressif/frameworks/esp-idf-v5.3.1"
$env:IDF_TOOLS_PATH = "D:/Espressif"
$env:PATH = "D:/Espressif/tools/idf-exe;D:/Espressif;D:/esp32_smart_fan/esp32_c6_matter_fan/gn/out;" + $env:PATH
. "$env:IDF_PATH\export.ps1" 2>&1 | Out-Null
$env:ESP_MATTER_PATH = "D:/Espressif/frameworks/esp-matter"
cd "D:/Espressif/frameworks/esp-matter/examples/light"
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

## Critical Patches (must re-apply after ESP-Matter update)

### 1. Nullable.h — Fix GCC `std::optional::operator==` error
File: `D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/src/app/data-model/Nullable.h`
Line 114-116: Replace `std::optional::operator==` with manual comparison:
```cpp
inline bool operator==(const Nullable<T> & other) const
{
    if (this->has_value() != other.has_value()) return false;
    if (!this->has_value()) return true;
    return this->value() == other.value();
}
```

### 2. create_args_gn.py — Filter conflicting flags from GN args
File: `D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip/create_args_gn.py`
Line 61: Add filter before wrapping flags:
```python
        compile_flags = [f for f in compile_flags
                        if not f.startswith('@"')
                        and "CHIP_SYSTEM_CONFIG_" not in f
                        and "INET_UDP_END_POINT_IMPL_CONFIG_FILE" not in f]
        compile_flags = [f'"{f}"'.replace(replace, replace_with) for f in compile_flags]
```

### 3. chip CMakeLists.txt — No-op external project (manual chip build)
File: `D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip/CMakeLists.txt`
Lines 424-429: Change to:
```cmake
    CONFIGURE_COMMAND       ""
    BUILD_COMMAND           ""
    INSTALL_COMMAND         ""
    BUILD_BYPRODUCTS        ${chip_libraries}
    DEPENDS                 args_gn
    BUILD_ALWAYS            0
```

### 4. Required Kconfig addition for sdkconfig.defaults
Add to any sdkconfig.defaults:
```
CONFIG_MBEDTLS_HKDF_C=y
```

### 5. GN junctions (must exist in config/esp32/)
Create Windows directory junctions:
```powershell
$esp32Cfg = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32"
$chipRoot = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip"
@("build","build_overrides","src","examples","scripts","zzz_generated","config","third_party/connectedhomeip","third_party/pigweed") | % {
    cmd /c "mklink /J `"$esp32Cfg\$_`" `"$chipRoot\$_`"" 2>$null
}
```

### 6. pigweed_environment.gni stub
File: `D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/build_overrides/pigweed_environment.gni`
Content: `# Minimal pigweed_environment.gni stub for ESP32 build`

### 7. Manual chip library build (after GN gen)
```powershell
$chipBuild = "<build_dir>/esp-idf/chip"
$gnPath = "D:/esp32_smart_fan/esp32_c6_matter_fan/gn/out/gn.exe"
& $gnPath "--root=$gnRoot" "gen" $chipBuild
# Then patch build.ninja to remove GN regeneration rule
# Then: ninja esp32
# Then create CMake stamps
```

## HA + SLZB-06M Commissioning (Working Config)

### SLZB-06M
- IP: `192.168.10.18`
- Mode: `Thread to remote OTBR`
- Port: 6638 (Thread)

### HA OTBR Add-on Config
- Network Device: `192.168.10.18:6638`
- Baudrate: 460800
- Hardware flow control: OFF
- Backbone Network Interface: `enp88s0`

### HA Matter Server Config
- Bluetooth Adapter ID: `0`

### ESP32-C6 Commissioning
- Firmware: `esp-matter/examples/light` (Thread-only C6 config)
- Pairing Code: `34970112332`
- VID: 65521 (0xFFF1), PID: 32768, Discriminator: 3840

### Commissioning Steps
1. Build & flash light example
2. SLZB-06M: Mode → Thread to remote OTBR
3. HA: Install Matter Server + OTBR add-ons
4. OTBR config: Network Device = `192.168.10.18:6638`, Backbone = `enp88s0`
5. Matter Server config: Bluetooth Adapter ID = `0`
6. HA: Add Bluetooth integration
7. Matter Server Web UI → Commission → `34970112332`
8. ESP32-C6 must be within BLE range of HA machine (< 5m)

### Thread Dataset
```
0e080000000000010000000300000f4a0300001435060004001fffe002089971e7e7b46f8daa0708fd858aab07fb1693051082bd2483f5db9526238b2a7ec3b7fc4c030e68612d7468726561642d3961383701029a8704104155fe8a5113e281b9fd3d6e91b6ddb60c0402a0f7f8
```

## Project Structure
```
├── CMakeLists.txt              # Root: IDF v5.3.1 path, ESP-Matter path, Thread GN args
├── main/
│   ├── CMakeLists.txt          # Component: SRCS main.cpp app_matter.cpp, REQUIRES +ieee802154
│   ├── main.cpp                # Entry point (app_main): 8-step init (fixed for IDF v5.3.1 API)
│   ├── config.h                # GPIO pins, LEDC config, encoder/button + step_to_percent()
│   ├── fan_controller.h        # LEDC PWM fan driver (100-400 Hz, 13-bit duty, 50% fixed)
│   ├── rotary_encoder.h        # EC11 rotary encoder with GPIO ISR + set_value()
│   ├── button.h                # Button with esp_timer debounce (GPIO_LEVEL_LOW → 0)
│   ├── system_manager.h        # FreeRTOS Queue event dispatcher + Matter encoder sync
│   ├── app_matter.h            # Matter interface + speed callback registration
│   ├── app_matter.cpp          # Matter stack: node, fan endpoint, clusters, Thread commissioning
│   ├── app_thread_leader.h     # Thread Leader API — disabled
│   └── app_thread_leader.cpp   # Thread Leader implementation — disabled
├── partitions.csv              # NVS (24K) + phy_init (4K) + factory (1920K)
├── sdkconfig.defaults          # Thread Join mode
├── gn/out/gn.exe               # GN binary (v2381)
├── esptool_bin/                # esptool Windows binaries
└── CLAUDE.md                   # This file
```

## Key Files Modified for IDF v5.3.1 Compatibility

| File | Fix |
|------|-----|
| `main/main.cpp` | `event_base_t` → `esp_event_base_t`, `ESP_EVENT_ANY_ID` → `ESP_EVENT_ANY_BASE`, `esp_ieee802154_init()` → `esp_ieee802154_enable()`, `esp_ieee802154_get_mac()` → `esp_read_mac()` |
| `main/app_matter.cpp` | Removed `app_reset.h`, `esp_thread_network.h`, `network_commissioning.h` (not in v5.3.1) |
| `main/button.h` | `GPIO_LEVEL_LOW` → `0` |
| `main/CMakeLists.txt` | Added `ieee802154` to REQUIRES |
| `main/config.h` | Added `step_to_percent()`, `percent_to_step()`, `level_to_percent()` |
| `main/rotary_encoder.h` | Added `set_value()` |
| `main/system_manager.h` | Added `sync_encoder_step()`, Matter speed callback |

## Dependencies
- ESP-IDF v5.3.1 at `D:/Espressif/frameworks/esp-idf-v5.3.1`
- ESP-Matter at `D:/Espressif/frameworks/esp-matter`
- GN v2381 at `gn/out/gn.exe`
- Python 3.14.4
- GCC 13.2 (riscv32-esp-elf)
