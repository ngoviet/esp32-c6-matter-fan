/**
 * @file app_matter.cpp
 * @brief Matter over Thread — Dimmable Light endpoint for fan CLK speed control.
 *
 * Device appears as a dimmable light with 0-100% brightness slider.
 * Brightness % maps to CLK frequency 28-328Hz via: freq = 28 + (pct * 300) / 100
 * 50% fixed duty cycle on GPIO1.
 *
 * Note: Fan device type (0x002B) would be ideal but the generated data model
 * adds 150+ include dirs that exceed Windows cmdline limit. Dimmable Light
 * is functionally equivalent — brightness slider controls fan speed linearly
 * after inverse gamma correction.
 */
#include "app_matter.h"
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_matter_core.h>
#include <esp_openthread_types.h>
#include <esp_timer.h>
#include <platform/ESP32/OpenthreadLauncher.h>
#include <math.h>

using namespace esp_matter;
using namespace esp_matter::cluster;

static const char *TAG = "APP_MATTER";
static uint16_t s_endpoint_id = 0;
static FanController* s_fan_controller = nullptr;
static LedIndicator* s_led = nullptr;
static app_matter_speed_callback_t s_speed_callback = nullptr;
static esp_timer_handle_t s_reconnect_timer = nullptr;

// Standard Matter cluster/attribute IDs
static constexpr uint32_t CLUSTER_ON_OFF        = 0x0006;
static constexpr uint32_t ATTR_ON_OFF           = 0x0000;
static constexpr uint32_t CLUSTER_LEVEL_CONTROL = 0x0008;
static constexpr uint32_t ATTR_CURRENT_LEVEL    = 0x0000;

// If Thread is disconnected for 5 minutes, reboot to attempt fresh re-attach
static constexpr uint32_t RECONNECT_TIMEOUT_SEC = 300;

static void reconnect_timer_cb(void*) {
    ESP_LOGW(TAG, "Thread disconnected for %lu s — rebooting...", (unsigned long)RECONNECT_TIMEOUT_SEC);
    esp_restart();
}

// Linear level-to-percent: HA sends raw 0-254 Level → map to 0-100%
// No gamma correction — HA Matter Server sends linear values.
static uint8_t level_to_pct(uint8_t level) {
    if (level <= 1) return 0;
    int pct = (static_cast<int>(level) * 100 + 127) / 254;
    return static_cast<uint8_t>(pct > 100 ? 100 : pct);
}

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
    // Level Control — undo HA gamma correction for linear CLK response
    else if (cluster_id == CLUSTER_LEVEL_CONTROL && attribute_id == ATTR_CURRENT_LEVEL) {
        uint8_t level = val->val.u8;
        uint8_t pct = level_to_pct(level);
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
        if (s_reconnect_timer) esp_timer_stop(s_reconnect_timer);
        if (s_led) s_led->set_pattern(LedIndicator::Pattern::ON);
    }
    else if (event->Type == chip::DeviceLayer::DeviceEventType::kThreadConnectivityChange) {
        if (event->ThreadConnectivityChange.Result == chip::DeviceLayer::ConnectivityChange::kConnectivity_Established) {
            ESP_LOGI(TAG, "Thread connection established");
            if (s_reconnect_timer) esp_timer_stop(s_reconnect_timer);
            if (s_led) s_led->set_pattern(LedIndicator::Pattern::ON);
        } else if (event->ThreadConnectivityChange.Result == chip::DeviceLayer::ConnectivityChange::kConnectivity_Lost) {
            ESP_LOGW(TAG, "Thread connection lost — reboot in %lu s", (unsigned long)RECONNECT_TIMEOUT_SEC);
            if (s_reconnect_timer) {
                esp_timer_start_once(s_reconnect_timer, RECONNECT_TIMEOUT_SEC * 1000000);
            }
            if (s_led) s_led->set_pattern(LedIndicator::Pattern::SLOW_BLINK);
        }
    }
}

// ---------- Init ----------

esp_err_t app_matter_init(FanController* fan_controller, LedIndicator* led)
{
    s_fan_controller = fan_controller;
    s_led = led;
    ESP_LOGI(TAG, "Initializing Matter Dimmable Light (Fan CLK driver)...");

    if (s_led) s_led->set_pattern(LedIndicator::Pattern::FAST_BLINK);

    // 1. Create Matter node
    node::config_t node_config;
    node_t *node = node::create(&node_config, app_attribute_update_cb, app_identification_cb);
    if (!node) { ESP_LOGE(TAG, "Node create failed"); return ESP_FAIL; }

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

    // 5. Console
    esp_matter::console::diagnostics_register_commands();
    esp_matter::console::init();

    // 6. Reconnect timer
    const esp_timer_create_args_t timer_args = {
        .callback = reconnect_timer_cb, .arg = nullptr, .name = "reconnect"
    };
    esp_timer_create(&timer_args, &s_reconnect_timer);

    ESP_LOGI(TAG, "Matter Dimmable Light ready — Endpoint %u", s_endpoint_id);
    return ESP_OK;
}

// ---------- Report to Matter ----------

void app_matter_report_onoff(bool is_on)
{
    if (s_endpoint_id == 0) return;
    attribute_t *attr = attribute::get(s_endpoint_id, CLUSTER_ON_OFF, ATTR_ON_OFF);
    if (!attr) return;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attr, &val);
    val.val.b = is_on;
    attribute::update(s_endpoint_id, CLUSTER_ON_OFF, ATTR_ON_OFF, &val);
}

void app_matter_report_speed(uint8_t percentage)
{
    if (s_endpoint_id == 0) return;
    uint8_t level = (percentage * 254 + 50) / 100;
    attribute_t *attr = attribute::get(s_endpoint_id, CLUSTER_LEVEL_CONTROL, ATTR_CURRENT_LEVEL);
    if (!attr) return;
    esp_matter_attr_val_t val = esp_matter_invalid(NULL);
    attribute::get_val(attr, &val);
    val.val.u8 = level;
    attribute::update(s_endpoint_id, CLUSTER_LEVEL_CONTROL, ATTR_CURRENT_LEVEL, &val);
}

uint16_t app_matter_get_fan_endpoint_id() { return s_endpoint_id; }

void app_matter_register_speed_callback(app_matter_speed_callback_t callback)
{
    s_speed_callback = callback;
    ESP_LOGI(TAG, "Speed callback registered");
}

void app_matter_factory_reset()
{
    ESP_LOGW(TAG, "=== FACTORY RESET ===");
    if (s_led) s_led->set_pattern(LedIndicator::Pattern::RAPID_BLINK);
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_matter::factory_reset();
}
