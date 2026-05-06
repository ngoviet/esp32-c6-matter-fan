cmake_minimum_required(VERSION 3.16)

# Đảm bảo IDF_PATH được ưu tiên
if(NOT DEFINED ENV{IDF_PATH})
    set(ENV{IDF_PATH} "D:/Espressif/frameworks/esp-idf-v5.3.1")
endif()

# ESP-Matter path
if(NOT DEFINED ENV{ESP_MATTER_PATH})
    set(ENV{ESP_MATTER_PATH} "D:/Espressif/frameworks/esp-matter")
endif()

set(ESP_MATTER_PATH "D:/Espressif/frameworks/esp-matter")
set(MATTER_SDK_PATH "D:/Espressif/frameworks/esp-matter/connectedhomeip/connectedhomeip")

# Extra components: esp_matter + CHIP platform
set(EXTRA_COMPONENT_DIRS
    "${ESP_MATTER_PATH}/components"
    "${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip/config/esp32/components"
)

# Matter over Thread (no WiFi)
set(MATTER_EXTRA_ARGS_GN "enable_thread=true\n")
set(MATTER_EXTRA_ARGS_GN "${MATTER_EXTRA_ARGS_GN}enable_wifi=false\n")
set(MATTER_EXTRA_ARGS_GN "${MATTER_EXTRA_ARGS_GN}esp_target=\"esp32c6\"\n")
set(ENV{MATTER_EXTRA_ARGS_GN} "${MATTER_EXTRA_ARGS_GN}")

add_compile_definitions(CHIP_DEVICE_LAYER_TARGET=ESP32)
add_compile_definitions(CHIP_HAVE_CONFIG_H=1)

# Suppress GCC 13 warnings in CHIP SDK (format + deprecation)
add_compile_options(-Wno-deprecated-declarations -Wno-error=format)

# Faster child supervision: check every 30s, re-attach after 60s of silence
# (OpenThread defaults are 129s interval, 190s timeout — too slow for recovery)
add_compile_definitions(OPENTHREAD_CONFIG_CHILD_SUPERVISION_INTERVAL=30)
add_compile_definitions(OPENTHREAD_CONFIG_CHILD_SUPERVISION_CHECK_TIMEOUT=60)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_c6_matter_fan)
