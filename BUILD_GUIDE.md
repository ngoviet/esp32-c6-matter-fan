# Build Guide — ESP32-C6 Matter Fan

## Environment

- **ESP-IDF:** v5.3.1 at `D:/Espressif/frameworks/esp-idf-v5.3.1`
- **ESP-Matter:** at `D:/Espressif/frameworks/esp-matter`
- **GN:** `gn/out/gn.exe` (v2381, downloaded from Google CIPD)
- **Python:** 3.14.4
- **GCC:** 13.2 (riscv32-esp-elf)

## Quick Build (PowerShell)

```powershell
$env:IDF_PATH = "D:/Espressif/frameworks/esp-idf-v5.3.1"
$env:IDF_TOOLS_PATH = "D:/Espressif"
$env:PATH = "D:/Espressif/tools/idf-exe;D:/Espressif;D:/esp32_smart_fan/esp32_c6_matter_fan/gn/out;" + $env:PATH
. "$env:IDF_PATH\export.ps1" 2>&1 | Out-Null
$env:ESP_MATTER_PATH = "D:/Espressif/frameworks/esp-matter"
cd "D:/esp32_smart_fan/esp32_c6_matter_fan"
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

## Full Build Steps (after fullclean)

### 1. Configure & Build

```powershell
idf.py set-target esp32c6
idf.py build
idf.py -p COM4 flash
```

The chip library is now built automatically via CMake ExternalProject (patch #3).  
The `fix_and_build_chip.py` script handles GN gen + build.ninja fix automatically.

### 2. Create directory junction (short path, avoids Windows cmdline limit)

```powershell
cmd /c "mklink /J C:\em D:\Espressif\frameworks\esp-matter"
```

This is needed because GN generates compile commands with long include paths
that can exceed the Windows 32K command-line limit.

## 9 Critical Patches

These must be re-applied after any ESP-Matter or CHIP SDK update:

### 1. Nullable.h — Fix std::optional::operator==
**File:** `esp-matter/connectedhomeip/connectedhomeip/src/app/data-model/Nullable.h` (line 114-116)

```cpp
inline bool operator==(const Nullable<T> & other) const {
    if (this->has_value() != other.has_value()) return false;
    if (!this->has_value()) return true;
    return this->value() == other.value();
}
```

### 2. create_args_gn.py — Filter conflicting flags
**File:** `esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip/create_args_gn.py` (line 61)

```python
compile_flags = [f for f in compile_flags
                if not f.startswith('@"')
                and "CHIP_SYSTEM_CONFIG_" not in f
                and "INET_UDP_END_POINT_IMPL_CONFIG_FILE" not in f]
compile_flags = [f'"{f}"'.replace(replace, replace_with) for f in compile_flags]
```

### 3. chip CMakeLists.txt — Auto GN gen + ninja build via ExternalProject
**File:** `esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip/CMakeLists.txt` (lines 420-430)

```cmake
externalproject_add(
    chip_gn
    SOURCE_DIR              ${CHIP_ROOT}
    BINARY_DIR              ${CMAKE_CURRENT_BINARY_DIR}
    CONFIGURE_COMMAND       ${Python3_EXECUTABLE} D:/esp32_smart_fan/esp32_c6_matter_fan/fix_and_build_chip.py D:/esp32_smart_fan/esp32_c6_matter_fan ${CHIP_ROOT} ${CMAKE_CURRENT_BINARY_DIR}
    BUILD_COMMAND           ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_BINARY_DIR} ninja esp32
    INSTALL_COMMAND         ""
    BUILD_BYPRODUCTS        ${chip_libraries}
    DEPENDS                 args_gn
    BUILD_ALWAYS            0
)
```

**Note:** Update the hardcoded project path if your project location changes.

### 4. sdkconfig.defaults — Add HKDF
```
CONFIG_MBEDTLS_HKDF_C=y
```

### 5. GN junctions in config/esp32/
```powershell
$esp32Cfg = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32"
$chipRoot = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip"
@("build","build_overrides","src","examples","scripts","zzz_generated","config","third_party/connectedhomeip","third_party/pigweed") | % {
    cmd /c "mklink /J `"$esp32Cfg\$_`" `"$chipRoot\$_`"" 2>$null
}
```

### 6. pigweed_environment.gni stub
**File:** `esp-matter/connectedhomeip/connectedhomeip/build_overrides/pigweed_environment.gni`
Content: `# Minimal pigweed_environment.gni stub for ESP32 build`

### 7. pigweed_environment.gni stub at CHIP_ROOT
Create empty file at `connectedhomeip/connectedhomeip/build_overrides/pigweed_environment.gni`

### 8. ExchangeContext.cpp — Prevent subscription loss on network down
**File:** `esp-matter/connectedhomeip/connectedhomeip/src/messaging/ExchangeContext.cpp`

**Add include** (after line 39):
```cpp
#include <messaging/ErrorCategory.h>
```

**Patch error handling** (around line 188):
```cpp
// BEFORE:
if (session->IsSecureSession() && session->AsSecureSession()->IsCASESession())
{
    session->AsSecureSession()->MarkAsDefunct();
}

// AFTER:
if (session->IsSecureSession() &&
    session->AsSecureSession()->IsCASESession() &&
    !IsSendErrorNonCritical(err))
{
    session->AsSecureSession()->MarkAsDefunct();
}
```

**Why:** When Thread disconnects (SLZB-06M power cycle), UDP sends fail with ERR_RTE. Without this patch, ANY send error marks the CASE session as defunct, which terminates all subscriptions. HA then can't receive updates even after Thread reconnects.

### 9. commodity-tariff-server.cpp — Fix format specifiers for RISC-V
**File:** `esp-matter/connectedhomeip/connectedhomeip/src/app/clusters/commodity-tariff-server/commodity-tariff-server.cpp`

```cpp
// Lines 660, 668, 700, 715: Cast uint32_t to unsigned int for %u format
ChipLogDetail(AppServer, "... %u", (unsigned int)value.date);
ChipLogDetail(AppServer, "... %u", (unsigned int)value.dayEntryID);
```

**Why:** On RISC-V 32-bit, `uint32_t` is `long unsigned int` but `%u` expects `unsigned int`. GCC 13.2 with `-Werror=format` fails.
