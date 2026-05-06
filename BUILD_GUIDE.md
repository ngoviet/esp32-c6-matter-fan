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

### 1. Configure sdkconfig

```powershell
Remove-Item sdkconfig -Force -ErrorAction SilentlyContinue
Copy-Item "D:\Espressif\frameworks\esp-matter\examples\light\sdkconfig.defaults.c6_thread" sdkconfig.defaults -Force
Add-Content sdkconfig.defaults "`nCONFIG_MBEDTLS_HKDF_C=y`n"
```

### 2. CMake Configure

```powershell
idf.py set-target esp32c6
```

### 3. Manual Chip Library Build

```powershell
$chipBuild = "build/esp-idf/chip"
$gnPath = "gn/out/gn.exe"
$gnRoot = "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip/config/esp32"

# GN gen
& $gnPath "--root=$gnRoot" "gen" $chipBuild

# Patch build.ninja to disable GN regeneration loop
$bn = "$chipBuild/build.ninja"
$c = Get-Content $bn -Raw
# (remove rule gn, build.ninja.stamp, and generator=1 lines)
Set-Content $bn $c.Replace($old, $new) -NoNewline

# Build chip library
$env:PATH = "D:/Espressif/tools/ninja/1.11.1;D:/Espressif/tools/idf-exe;D:/Espressif;" + $env:PATH
. "D:/Espressif/frameworks/esp-idf-v5.3.1/export.ps1" 2>&1 | Out-Null
Set-Location $chipBuild
ninja esp32

# Create CMake stamps
$sd = "$chipBuild/chip_gn-prefix/src/chip_gn-stamp"; md -Force $sd | Out-Null
$f = (Get-Date).AddDays(1)
@("chip_gn-configure","chip_gn-build","chip_gn-install","chip_gn-download","chip_gn-update","chip_gn-patch","chip_gn-mkdir") | % { $p="$sd\$_"; if(!(Test-Path $p)){$null>$p};(Get-Item $p).LastWriteTime=$f }
```

### 4. Build Firmware

```powershell
Set-Location "D:\esp32_smart_fan\esp32_c6_matter_fan"
idf.py build
idf.py -p COM4 flash
```

## 7 Critical Patches

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

### 3. chip CMakeLists.txt — No-op external project
**File:** `esp-matter/connectedhomeip/connectedhomeip/config/esp32/components/chip/CMakeLists.txt` (lines 424-429)

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
@("build","build_overrides","src","examples","scripts","zzz_generated","config","third_party/connectedhomeip","third_party/pigweed") | % {
    cmd /c "mklink /J `"$esp32Cfg\$_`" `"$chipRoot\$_`"" 2>$null
}
```

### 6. pigweed_environment.gni stub
**File:** `esp-matter/connectedhomeip/connectedhomeip/build_overrides/pigweed_environment.gni`
Content: `# Minimal pigweed_environment.gni stub for ESP32 build`

### 7. pigweed_environment.gni stub at CHIP_ROOT
Create empty file at `connectedhomeip/connectedhomeip/build_overrides/pigweed_environment.gni`
