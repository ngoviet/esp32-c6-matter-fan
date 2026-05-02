# ESP32-C6 Matter Fan — Project Context

## Project Overview

Smart fan controller on **ESP32-C6** using **Matter over Thread**. Appears as a **Dimmable Light** in Home Assistant — brightness slider 0-100% maps to CLK frequency 28-328Hz. Controlled locally via rotary encoder + button, remotely via Matter.

- **Endpoint:** Dimmable Light (On/Off + Level Control)
- **CLK:** 28-328Hz, 50% fixed duty, GPIO1
- **Encoder:** 100 steps (1:1 with HA slider), GPIO2/3
- **Button:** Toggle on/off, restore last speed, GPIO4
- **OTA:** 2 slots × 3900KB (8MB flash)
- **Pairing Code:** 34970112332

**Working build:** ESP-IDF v5.3.1 | **Target:** esp32c6 | **Flash:** 8MB

## Quick Build & Flash

```powershell
$env:IDF_PATH = "D:/Espressif/frameworks/esp-idf-v5.3.1"
$env:IDF_TOOLS_PATH = "D:/Espressif"
$env:PATH = "D:/Espressif/tools/idf-exe;D:/Espressif;" + $env:PATH
. "$env:IDF_PATH\export.ps1" 2>&1 | Out-Null
$env:ESP_MATTER_PATH = "D:/Espressif/frameworks/esp-matter"
cd "D:/esp32_smart_fan/esp32_c6_matter_fan"
idf.py build
idf.py -p COM4 flash
```

## Critical Build Patches (re-apply after ESP-Matter update)

### 1. Nullable.h — Fix `std::optional::operator==`
File: `esp-matter/.../src/app/data-model/Nullable.h` lines 114-116
```cpp
inline bool operator==(const Nullable<T> & other) const {
    if (this->has_value() != other.has_value()) return false;
    if (!this->has_value()) return true;
    return this->value() == other.value();
}
```

### 2. create_args_gn.py — Filter conflicting flags
File: `esp-matter/.../config/esp32/components/chip/create_args_gn.py` line 61
```python
compile_flags = [f for f in compile_flags
                if not f.startswith('@"')
                and "CHIP_SYSTEM_CONFIG_" not in f
                and "INET_UDP_END_POINT_IMPL_CONFIG_FILE" not in f]
compile_flags = [f'"{f}"'.replace(replace, replace_with) for f in compile_flags]
```

### 3. chip CMakeLists.txt — No-op external project
File: `esp-matter/.../config/esp32/components/chip/CMakeLists.txt` lines 424-429
```cmake
CONFIGURE_COMMAND       ""
BUILD_COMMAND           ""
INSTALL_COMMAND         ""
BUILD_BYPRODUCTS        ${chip_libraries}
DEPENDS                 args_gn
BUILD_ALWAYS            0
```

### 4. sdkconfig.defaults — Add HKDF
```
CONFIG_MBEDTLS_HKDF_C=y
```

### 5. GN junctions in config/esp32/
```powershell
$esp32Cfg = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32"
$chipRoot = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip"
"build","build_overrides","src","examples","scripts","zzz_generated","config","third_party/connectedhomeip","third_party/pigweed" | % {
    cmd /c "mklink /J `"$esp32Cfg\$_`" `"$chipRoot\$_`"" 2>$null
}
```

### 6. pigweed_environment.gni stub
File: `esp-matter/.../build_overrides/pigweed_environment.gni` — empty stub file.

### 7. Manual chip library build (after GN gen, before idf.py build)
```powershell
# After cmake configure, before ninja:
$chipBuild = "build/esp-idf/chip"
& gn.exe --root=<gnRoot> gen $chipBuild
# Edit build.ninja: remove GN regeneration rule
# ninja esp32
# Create CMake stamps in chip_gn-prefix/src/chip_gn-stamp/
```

## HA + SLZB-06M Commissioning

| Param | Value |
|-------|-------|
| SLZB-06M Mode | Thread to remote OTBR |
| SLZB-06M IP | 192.168.10.18:6638 |
| OTBR Backbone | enp88s0 |
| HA Bluetooth | 64:79:F0:45:79:B3 (ID: 0) |
| Matter Server BT Adapter ID | 0 |
| Pairing Code | 34970112332 |
| Thread Dataset | `0e080000000000010000000300000f4a...` |

## Project Files

| File | Purpose |
|------|---------|
| `main/config.h` | GPIO pins, freq range, encoder steps |
| `main/fan_controller.h` | LEDC PWM: freq control, 50% duty |
| `main/rotary_encoder.h` | EC11 encoder ISR, 0-100 steps |
| `main/button.h` | Button debounce via esp_timer |
| `main/system_manager.h` | FreeRTOS Queue dispatcher + matter sync |
| `main/app_matter.cpp` | Matter Dimmable Light endpoint |
| `main/main.cpp` | Entry point: NVS→802.15.4→GPIO→System→Matter |
| `partitions.csv` | OTA: 2×3900KB slots, 8MB flash |
| `sdkconfig.defaults` | Thread-only, HKDF enabled |
| `WIRING.md` | Wiring diagram |
