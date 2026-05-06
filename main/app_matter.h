/**
 * @file app_matter.h
 * @brief Matter over Thread interface — Fan device type with Fan Control cluster.
 *
 * Device appears as a Fan (0x002B) in Home Assistant with:
 *   - FanMode (Off=0 / On=5) for power control
 *   - PercentSetting 0-100% for speed (linear, no gamma correction)
 *
 * CLK frequency: 28-328Hz, 50% fixed duty on GPIO1.
 */
#ifndef APP_MATTER_H
#define APP_MATTER_H

#include <stdint.h>
#include <esp_err.h>
#include "fan_controller.h"
#include "led_indicator.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t app_matter_init(FanController* fan_controller, LedIndicator* led = nullptr);

void app_matter_report_onoff(bool is_on);
void app_matter_report_speed(uint8_t percentage);
uint16_t app_matter_get_fan_endpoint_id();

typedef void (*app_matter_speed_callback_t)(uint8_t percent);
void app_matter_register_speed_callback(app_matter_speed_callback_t callback);

/**
 * @brief Factory reset Matter fabric + Thread credentials.
 * Hold button for 10 seconds to trigger. Device reboots into commissioning mode.
 */
void app_matter_factory_reset();

#ifdef __cplusplus
}
#endif

#endif // APP_MATTER_H
