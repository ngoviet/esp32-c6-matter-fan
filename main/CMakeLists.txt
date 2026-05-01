idf_component_register(SRCS "main.cpp" "app_matter.cpp"
                     INCLUDE_DIRS "."
                     REQUIRES driver esp_matter freertos esp_netif esp_event nvs_flash ieee802154)

# Thread-specific compile definitions
target_compile_definitions(${COMPONENT_LIB}
    PRIVATE "CONFIG_MATTER_THREAD_ENABLED=1"
)

# Thread Leader mode - only include when explicitly enabled
# Note: Thread Leader requires ESP-IDF v5.5+ with full OpenThread Border Router support
# For now, this feature is available but requires manual sdkconfig configuration
# target_sources(${COMPONENT_LIB} PRIVATE "app_thread_leader.cpp")
# target_include_directories(${COMPONENT_LIB} PUBLIC ".")
# target_compile_definitions(${COMPONENT_LIB}
#     PRIVATE "CONFIG_THREAD_LEADER_ENABLED=1"
# )
