/**
 * @file app_matter.h
 * @brief Matter over Thread interface for ESP32-C6 Smart Fan (IDF v5.3.1 API)
 */
#ifndef APP_MATTER_H
#define APP_MATTER_H

#include <stdint.h>
#include <esp_err.h>
#include "fan_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize Matter over Thread stack (IDF v5.3.1 compatible)
 */
esp_err_t app_matter_init(FanController* fan_controller);

/**
 * @brief Report On/Off state to Matter controller
 */
void app_matter_report_onoff(bool is_on);

/**
 * @brief Report fan speed to Matter controller
 */
void app_matter_report_speed(uint8_t percentage);

/**
 * @brief Get current fan endpoint ID
 */
uint16_t app_matter_get_fan_endpoint_id();

/**
 * @brief Callback type for Matter-initiated speed changes
 */
typedef void (*app_matter_speed_callback_t)(uint8_t percent);

/**
 * @brief Register a callback for Matter-initiated speed changes
 */
void app_matter_register_speed_callback(app_matter_speed_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // APP_MATTER_H
