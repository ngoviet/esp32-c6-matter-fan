idf_component_register(
    SRCS "main.cpp" "app_matter.cpp" "led_strip_encoder.c"
    INCLUDE_DIRS "."
    REQUIRES
        nvs_flash
        esp_event
        esp_timer
        esp_matter
        esp_driver_rmt
        driver
        ieee802154
        esp_system
        bt
        openthread
        lwip
        freertos
        log
        mbedtls
)
