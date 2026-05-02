/**
 * @file app_matter.cpp
 * @brief Matter over Thread — Dimmable Light endpoint for fan CLK speed control
 *
 * Device appears as a dimmable light with 0-100% brightness.
 * Brightness maps to CLK frequency 100-400Hz (50% fixed duty).
 * Matches ESPHome logic from main.yaml: freq = 100 + (pct * 300) / 100
 */
#include "app_matter.h"
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_openthread_types.h>
#include <platform/ESP32/OpenthreadLauncher.h>
#include <esp_matter_ota.h>

using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "APP_MATTER";
static uint16_t s_endpoint_id = 0;
static FanController* s_fan_controller = nullptr;
static app_matter_speed_callback_t s_speed_callback = nullptr;

// Standard Matter cluster/attribute IDs
static constexpr uint32_t CLUSTER_ON_OFF        = 0x0006;
static constexpr uint32_t ATTR_ON_OFF           = 0x0000;
static constexpr uint32_t CLUSTER_LEVEL_CONTROL = 0x0008;
static constexpr uint32_t ATTR_CURRENT_LEVEL    = 0x0000;

// ---------- Attribute Update Callback ----------

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type,
                                         uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val,
                                         void *priv_data)
{
    if (type != attribute::PRE_UPDATE || endpoint_id != s_endpoint_id) {
        return ESP_OK;
    }

    // On/Off cluster
    if (cluster_id == CLUSTER_ON_OFF && attribute_id == ATTR_ON_OFF) {
        ESP_LOGI(TAG, "Matter: OnOff = %d", val->val.b);
        if (s_fan_controller) {
            if (val->val.b) {
                s_fan_controller->turn_on();
                if (s_speed_callback) s_speed_callback(s_fan_controller->get_speed());
            } else {
                s_fan_controller->turn_off();
            }
        }
    }
    // Level Control → brightness 0-254 maps to fan speed 0-100%
    else if (cluster_id == CLUSTER_LEVEL_CONTROL && attribute_id == ATTR_CURRENT_LEVEL) {
        uint8_t level = val->val.u8;
        uint8_t pct = (level * 100 + 127) / 254;
        ESP_LOGI(TAG, "Matter: Level=%d -> Speed=%d%%", level, pct);
        if (s_fan_controller) {
            if (level == 0) {
                s_fan_controller->turn_off();
            } else {
                s_fan_controller->set_speed(pct);
                s_fan_controller->turn_on();
            }
        }
        if (s_speed_callback) s_speed_callback(pct);
    }
    return ESP_OK;
}

// ---------- Identification Callback ----------

static esp_err_t app_identification_cb(identification::callback_type_t type,
                                       uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification: type=%d effect=%d", (int)type, effect_id);
    return ESP_OK;
}

// ---------- Event Callback ----------

static void app_event_cb(const ChipDeviceEvent *event, intptr_t arg)
{
    if (event->Type == chip::DeviceLayer::DeviceEventType::kCommissioningComplete) {
        ESP_LOGI(TAG, "*** Commissioning complete! ***");
    }
}

// ---------- Init ----------

esp_err_t app_matter_init(FanController* fan_controller)
{
    s_fan_controller = fan_controller;
    ESP_LOGI(TAG, "Initializing Matter Dimmable Light (Fan CLK driver)...");

    // 1. Create Matter node
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) { ESP_LOGE(TAG, "Node create failed"); return ESP_FAIL; }

    // Set device name (appears as NodeLabel in HA)
    uint16_t root_ep_id = 0;
    esp_matter_attr_val_t name_val = esp_matter_invalid(NULL);
    name_val.type = ESP_MATTER_VAL_TYPE_CHAR_STRING;
    name_val.val.a.b = (uint8_t*)"Fan speed control";
    name_val.val.a.s = 17;
    attribute::update(0, 0x0028, 0x0005, &name_val);

    // 2. Create Dimmable Light endpoint (on_off + level_control built-in)
    endpoint::dimmable_light::config_t light_config;
    light_config.on_off.on_off = false;
    light_config.level_control.current_level = static_cast<uint8_t>(0);
    light_config.level_control.on_level = static_cast<uint8_t>(0);
    light_config.level_control.options = 1;

    endpoint_t *ep = endpoint::dimmable_light::create(node, &light_config, ENDPOINT_FLAG_NONE, nullptr);
    if (!ep) { ESP_LOGE(TAG, "Endpoint create failed"); return ESP_FAIL; }
    s_endpoint_id = endpoint::get_id(ep);
    ESP_LOGI(TAG, "Dimmable Light endpoint: %u", s_endpoint_id);

    // 3. OpenThread config
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t ot_config = {
        .radio_config = { .radio_mode = RADIO_MODE_NATIVE },
        .host_config = { .host_connection_mode = HOST_CONNECTION_MODE_NONE },
        .port_config = { .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10 },
    };
    set_openthread_platform_config(&ot_config);
#endif

    // 4. Start Matter
    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) { ESP_LOGE(TAG, "Matter start failed: %d", err); return err; }

    // 5. Init OTA requestor
    esp_matter_ota_requestor_init();
    ESP_LOGI(TAG, "OTA requestor initialized");

    // 6. Console
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::init();

    ESP_LOGI(TAG, "Matter Dimmable Light ready — Endpoint %u", s_endpoint_id);
    return ESP_OK;
}

// ---------- Report to Matter ----------

void app_matter_report_onoff(bool is_on)
{
    if (s_endpoint_id == 0) return;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.val.b = is_on;
    attribute::update(s_endpoint_id, CLUSTER_ON_OFF, ATTR_ON_OFF, &val);
}

void app_matter_report_speed(uint8_t percentage)
{
    if (s_endpoint_id == 0) return;
    // Report speed as brightness level (0-100% -> 0-254)
    uint8_t level = (percentage * 254 + 50) / 100;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.val.u8 = level;
    attribute::update(s_endpoint_id, CLUSTER_LEVEL_CONTROL, ATTR_CURRENT_LEVEL, &val);
}

uint16_t app_matter_get_fan_endpoint_id() { return s_endpoint_id; }

void app_matter_register_speed_callback(app_matter_speed_callback_t callback)
{
    s_speed_callback = callback;
    ESP_LOGI(TAG, "Speed callback registered");
}
