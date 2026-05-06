/**
 * @file main.cpp
 * @brief ESP32-C6 Smart Fan — Matter over Thread Fan device type.
 *
 * Init order: NVS → Network → 802.15.4 → GPIO ISR → SystemManager → Matter
 * Watchdog: 10s task watchdog fed from idle loop.
 */
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_ieee802154.h"
#include "esp_mac.h"

#include "system_manager.h"
#include "app_matter.h"

static const char *TAG = "MAIN";

static void init_network(void) {
    esp_event_handler_instance_t instance_id;
    esp_event_handler_instance_register(
        ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID,
        [](void*, esp_event_base_t, int32_t, void*) {}, NULL, &instance_id);
}

static void init_ieee802154(void) {
    ESP_LOGI(TAG, "Enabling IEEE 802.15.4 radio...");
    esp_ieee802154_enable();
    uint8_t mac[8];
    esp_read_mac(mac, ESP_MAC_IEEE802154);
    ESP_LOGI(TAG, "15.4 MAC: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], mac[6], mac[7]);
}

static void init_watchdog(void) {
    esp_task_wdt_config_t cfg = {
        .timeout_ms = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic = true,
    };
    esp_task_wdt_init(&cfg);
    esp_task_wdt_add(NULL); // Add current (main) task
    ESP_LOGI(TAG, "Task watchdog: %lu s", (unsigned long)WATCHDOG_TIMEOUT_SEC);
}

extern "C" void app_main(void) {
    // 1. NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // 2. Network events
    init_network();

    // 3. IEEE 802.15.4
    init_ieee802154();

    // 4. GPIO ISR service
    gpio_install_isr_service(0);

    // 5. System Manager (fan + encoder + button + LED)
    static SystemManager sys;
    ESP_ERROR_CHECK(sys.init());

    // 6. Matter over Thread
    ESP_ERROR_CHECK(app_matter_init(sys.get_fan_controller(), sys.get_led()));

    // 7. Watchdog
    init_watchdog();

    ESP_LOGI(TAG, "=== ESP32-C6 Matter Fan ready ===");
    ESP_LOGI(TAG, "Control: encoder (speed) + button (on/off)");
    ESP_LOGI(TAG, "Hold button 10s: factory reset");
    ESP_LOGI(TAG, "Pairing: 34970112332");
    ESP_LOGI(TAG, "LED: fast blink=commissioning, solid=connected, slow=disconnected");

    // 8. Idle loop — feed watchdog
    while (true) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
