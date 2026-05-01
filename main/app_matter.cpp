/**
 * @file app_matter.cpp
 * @brief Matter over Thread for ESP32-C6 Smart Fan (IDF v5.3.1 legacy API)
 *
 * Fan endpoint (0x002B) with Fan Control + On/Off + Level Control clusters.
 * Uses raw cluster/attribute IDs because generated headers are private in v5.3.1.
 */
#include "app_matter.h"
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_openthread_types.h>
#include <platform/ESP32/OpenthreadLauncher.h>

using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "APP_MATTER";
static uint16_t s_fan_endpoint_id = 0;
static FanController* s_fan_controller = nullptr;
static app_matter_speed_callback_t s_speed_callback = nullptr;

// Matter cluster/attribute IDs (standard, immutable)
static constexpr uint32_t CLUSTER_ON_OFF        = 0x0006;
static constexpr uint32_t ATTR_ON_OFF           = 0x0000;
static constexpr uint32_t CLUSTER_FAN_CONTROL   = 0x0202;
static constexpr uint32_t ATTR_FAN_MODE         = 0x0000;
static constexpr uint32_t ATTR_PERCENT_SETTING  = 0x0002;
static constexpr uint32_t ATTR_PERCENT_CURRENT  = 0x0003;
static constexpr uint32_t CLUSTER_LEVEL_CONTROL = 0x0008;
static constexpr uint32_t ATTR_CURRENT_LEVEL    = 0x0000;

// ---------- Attribute Update Callback ----------

static esp_err_t app_attribute_update_cb(attribute::callback_type_t type,
                                         uint16_t endpoint_id, uint32_t cluster_id,
                                         uint32_t attribute_id, esp_matter_attr_val_t *val,
                                         void *priv_data)
{
    if (type != attribute::PRE_UPDATE || endpoint_id != s_fan_endpoint_id) {
        return ESP_OK;
    }

    if (cluster_id == CLUSTER_ON_OFF && attribute_id == ATTR_ON_OFF) {
        ESP_LOGI(TAG, "Matter Cmd: OnOff = %d", val->val.b);
        if (s_fan_controller) {
            if (val->val.b) s_fan_controller->turn_on();
            else s_fan_controller->turn_off();
        }
    }
    else if (cluster_id == CLUSTER_FAN_CONTROL && attribute_id == ATTR_PERCENT_SETTING) {
        uint8_t pct = val->val.u8;
        ESP_LOGI(TAG, "Matter Cmd: Fan Speed = %d%%", pct);
        if (s_fan_controller) {
            if (pct > 0) { s_fan_controller->turn_on(); s_fan_controller->set_speed(pct); }
            else { s_fan_controller->turn_off(); }
        }
        if (s_speed_callback) s_speed_callback(pct);
    }
    else if (cluster_id == CLUSTER_FAN_CONTROL && attribute_id == ATTR_FAN_MODE) {
        ESP_LOGI(TAG, "Matter Cmd: Fan Mode = %d", val->val.u8);
    }
    else if (cluster_id == CLUSTER_LEVEL_CONTROL && attribute_id == ATTR_CURRENT_LEVEL) {
        uint8_t level = val->val.u8;
        uint8_t pct = (level * 100 + 127) / 254;
        ESP_LOGI(TAG, "Matter Cmd: Level = %d -> %d%%", level, pct);
        if (s_fan_controller) { s_fan_controller->set_speed(pct); s_fan_controller->turn_on(); }
        if (s_speed_callback) s_speed_callback(pct);
    }
    return ESP_OK;
}

// ---------- Identification Callback ----------

static esp_err_t app_identification_cb(identification::callback_type_t type,
                                       uint16_t endpoint_id, uint8_t effect_id,
                                       uint8_t effect_variant, void *priv_data)
{
    ESP_LOGI(TAG, "Identification: type=%d, effect=%d", (int)type, effect_id);
    return ESP_OK;
}

// ---------- Matter Event Callback ----------

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
    ESP_LOGI(TAG, "Initializing Matter over Thread...");

    // 1. Create Matter node with attribute + identification callbacks
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) { ESP_LOGE(TAG, "Failed to create Matter node"); return ESP_FAIL; }

    // 2. Create Fan endpoint (legacy API includes descriptor + identify + groups + fan_control)
    endpoint::fan::config_t fan_config;
    fan_config.fan_control.fan_mode = 0;
    fan_config.fan_control.fan_mode_sequence = 2;
    fan_config.fan_control.percent_setting = static_cast<uint8_t>(0);
    fan_config.fan_control.percent_current = 0;
    endpoint_t *fan_ep = endpoint::fan::create(node, &fan_config, ENDPOINT_FLAG_NONE, nullptr);
    if (!fan_ep) { ESP_LOGE(TAG, "Failed to create Fan endpoint"); return ESP_FAIL; }
    s_fan_endpoint_id = endpoint::get_id(fan_ep);
    ESP_LOGI(TAG, "Fan endpoint created: %u", s_fan_endpoint_id);

    // 3. Add On/Off cluster
    cluster::on_off::config_t oo_cfg;
    oo_cfg.on_off = false;
    cluster::on_off::create(fan_ep, &oo_cfg, CLUSTER_FLAG_NONE);

    // 4. Add Level Control cluster
    cluster::level_control::config_t lc_cfg;
    lc_cfg.current_level = static_cast<uint8_t>(0);
    lc_cfg.on_level = static_cast<uint8_t>(0);
    lc_cfg.options = 1;
    cluster::level_control::create(fan_ep, &lc_cfg, CLUSTER_FLAG_NONE);

    ESP_LOGI(TAG, "Clusters: On/Off + Fan Control + Level Control");

    // 5. Configure OpenThread
#if CHIP_DEVICE_CONFIG_ENABLE_THREAD
    esp_openthread_platform_config_t ot_config = {
        .radio_config = { .radio_mode = RADIO_MODE_NATIVE },
        .host_config = { .host_connection_mode = HOST_CONNECTION_MODE_NONE },
        .port_config = { .storage_partition_name = "nvs", .netif_queue_size = 10, .task_queue_size = 10 },
    };
    set_openthread_platform_config(&ot_config);
#endif

    // 6. Start Matter
    esp_err_t err = esp_matter::start(app_event_cb);
    if (err != ESP_OK) { ESP_LOGE(TAG, "Matter start failed: %d", err); return err; }

    // 7. Console
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::init();

    ESP_LOGI(TAG, "Matter over Thread ready — Fan Endpoint %u", s_fan_endpoint_id);
    return ESP_OK;
}

// ---------- Report to Matter ----------

void app_matter_report_onoff(bool is_on)
{
    if (s_fan_endpoint_id == 0) return;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.val.b = is_on;
    attribute::update(s_fan_endpoint_id, CLUSTER_ON_OFF, ATTR_ON_OFF, &val);
}

void app_matter_report_speed(uint8_t percentage)
{
    if (s_fan_endpoint_id == 0) return;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    val.val.u8 = percentage;
    attribute::update(s_fan_endpoint_id, CLUSTER_FAN_CONTROL, ATTR_PERCENT_SETTING, &val);
    attribute::update(s_fan_endpoint_id, CLUSTER_FAN_CONTROL, ATTR_PERCENT_CURRENT, &val);
}

uint16_t app_matter_get_fan_endpoint_id() { return s_fan_endpoint_id; }

void app_matter_register_speed_callback(app_matter_speed_callback_t callback)
{
    s_speed_callback = callback;
    ESP_LOGI(TAG, "Speed callback registered");
}
