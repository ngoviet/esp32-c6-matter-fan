/**
 * @file main.cpp
 * @brief Main entry point for ESP32-C6 Smart Fan with Matter over Thread
 *
 * This is the main entry point for the ESP32-C6 Smart Fan application.
 * It initializes:
 * 1. NVS flash storage
 * 2. GPIO ISR service
 * 3. System Manager (Fan Controller, Rotary Encoder, Button)
 * 4. Thread Network (Leader or Join mode)
 * 5. Matter over Thread stack
 *
 * MODE SELECTION:
 * - Thread Leader Mode: ESP32-C6 CREATES its own Thread network
 *   Enable with: CONFIG_THREAD_LEADER_ENABLED=y in sdkconfig
 * - Thread Join Mode: ESP32-C6 joins existing Thread network via SLZB-06M
 *   Enable with: CONFIG_MATTER_THREAD_ENABLED=y (default)
 *
 * The device connects to SMLIGHT SLZB-06M (Thread Border Router) via
 * Thread network (802.15.4) - NOT WiFi.
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"

// Thread/802.15.4 specific includes
#include "esp_ieee802154.h"
#include "esp_netif.h"
#include "esp_mac.h"

// Project includes
#include "system_manager.h"
#include "app_matter.h"

// Thread Leader mode - temporarily disabled due to ESP-IDF compatibility
// Enable CONFIG_THREAD_LEADER_ENABLED in sdkconfig to activate
// #include "app_thread_leader.h"

static const char *TAG = "MAIN";

/**
 * @brief Network event handler callback (C-style for ESP-IDF compatibility)
 */
static void network_event_handler(void *handler_args, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data) {
    // Handle network commissioning events
    ESP_LOGD("MAIN", "Net event: base=%d, id=%ld",
             (int)event_base, (long)event_id);
}

/**
 * @brief Initialize ESP-NETIF (network interfaces)
 *
 * This must be called before using any network functionality.
 */
static esp_err_t init_network(void) {
    ESP_LOGI(TAG, "Initializing network...");
    
    // Initialize event loop for network events
    esp_event_handler_instance_t instance_id_ANY;
    esp_err_t err = esp_event_handler_instance_register(
        ESP_EVENT_ANY_BASE,
        ESP_EVENT_ANY_ID,
        network_event_handler,
        NULL,
        &instance_id_ANY
    );
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register event handler: %s", esp_err_to_name(err));
        return err;
    }
    
    return ESP_OK;
}

/**
 * @brief Initialize IEEE 802.15.4 radio for Thread
 * 
 * This enables the 802.15.4 radio which is required for Thread.
 */
static void init_ieee802154(void) {
    ESP_LOGI(TAG, "Initializing IEEE 802.15.4 radio...");

    // Enable IEEE 802.15.4 subsystem (IDF v5.3.1 API)
    esp_err_t err = esp_ieee802154_enable();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable IEEE 802.15.4: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "IEEE 802.15.4 radio initialized");

    // Print MAC address
    uint8_t mac[8];
    esp_read_mac(mac, ESP_MAC_IEEE802154);
    ESP_LOGI(TAG, "15.4 MAC: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
}

/**
 * @brief Print commissioning information
 * 
 * Displays information about how to commission the device.
 */
static void print_commissioning_info(void) {
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "  Matter over Thread - Commissioning");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "To commission this device:");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Method 1: QR Code");
    ESP_LOGI(TAG, "  - Use a Matter controller (e.g., SLZB-06M web UI)");
    ESP_LOGI(TAG, "  - Scan the QR code displayed on the console");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Method 2: Pairing Code");
    ESP_LOGI(TAG, "  - Use the 11-digit pairing code");
    ESP_LOGI(TAG, "  - Enter it in your Matter controller");
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "Thread Network:");
    ESP_LOGI(TAG, "  - Border Router: SMLIGHT SLZB-06M");
    ESP_LOGI(TAG, "  - Protocol: Matter over Thread (802.15.4)");
    ESP_LOGI(TAG, "==========================================");
    ESP_LOGI(TAG, "");
}

/**
 * @brief Main entry point for the ESP32-C6 Smart Fan Application
 *
 * This function initializes all system components and starts
 * the Matter over Thread stack.
 */
extern "C" void app_main(void) {
    esp_err_t err;
    
    // ============================================================
    // Step 1: Initialize NVS (Non-Volatile Storage)
    // ============================================================
    ESP_LOGI(TAG, "Initializing NVS...");
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs migration, erasing...");
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS initialization failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "NVS initialized successfully");
    
    // ============================================================
    // Step 2: Initialize Network Stack
    // ============================================================
    err = init_network();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Network initialization warning: %s", esp_err_to_name(err));
        // Continue anyway, Thread may not need network events
    }
    
    // ============================================================
    // Step 3: Initialize IEEE 802.15.4 Radio (for Thread)
    // ============================================================
    init_ieee802154();
    
    // ============================================================
    // Step 4: Install GPIO ISR Service
    // ============================================================
    ESP_LOGI(TAG, "Installing GPIO ISR service...");
    err = gpio_install_isr_service(0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "GPIO ISR service installed");
    
    // ============================================================
    // Step 5: Initialize System Manager
    // ============================================================
    ESP_LOGI(TAG, "Initializing System Manager...");
    static SystemManager system_manager;
    err = system_manager.init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "System Manager initialization failed: %s", esp_err_to_name(err));
        return;
    }
    ESP_LOGI(TAG, "System Manager initialized");
    
    // ============================================================
    // Step 6: Initialize Matter over Thread
    // ============================================================
    ESP_LOGI(TAG, "Initializing Matter over Thread...");
    err = app_matter_init(system_manager.get_fan_controller());
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Matter initialization failed: %s", esp_err_to_name(err));
        return;
    }
    
    // ============================================================
    // Step 7: Print commissioning information
    // ============================================================
    print_commissioning_info();
    
    ESP_LOGI(TAG, "");
    ESP_LOGI(TAG, "ESP32-C6 Smart Fan with Matter over Thread is ready!");
    ESP_LOGI(TAG, "Control: Rotary Encoder (speed), Button (on/off)");
    ESP_LOGI(TAG, "Network: Thread (via SMLIGHT SLZB-06M)");
    ESP_LOGI(TAG, "");
    
    // ============================================================
    // Step 8: Main loop (keeps task alive)
    // ============================================================
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
