/**
 * @file app_thread_leader.h
 * @brief Thread Leader (Border Router) interface for ESP32-C6 Smart Fan
 * 
 * This header defines the interface for Thread Leader functionality
 * where ESP32-C6 CREATES and MANAGES its own Thread network.
 */

#ifndef APP_THREAD_LEADER_H
#define APP_THREAD_LEADER_H

#include <stdint.h>
#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Thread network information structure
 */
typedef struct {
    char network_name[32];      // Thread Network Name
    uint16_t pan_id;            // PAN ID
    uint8_t channel;            // Thread channel (11-26)
    uint8_t master_key[16];     // Master Key (16 bytes)
    bool is_leader;             // True if ESP32-C6 is Thread Leader
    bool is_running;            // True if Thread network is active
} thread_leader_info_t;

/**
 * @brief Initialize Thread Leader mode
 * 
 * Creates ESP32-C6 as a Thread Leader (Border Router).
 * This function will:
 * 1. Initialize OpenThread stack in LEADER role
 * 2. Create a new Thread network with configurable parameters
 * 3. Enable Border Router functionality
 * 4. Start accepting Thread device joins
 * 
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_init(void);

/**
 * @brief Stop Thread Leader mode
 * 
 * Tears down the Thread network and stops Border Router.
 * 
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_stop(void);

/**
 * @brief Get current Thread network information
 * 
 * @param info Pointer to thread_leader_info_t to fill
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_get_info(thread_leader_info_t *info);

/**
 * @brief Set Thread network parameters
 * 
 * @param network_name Thread network name (max 31 chars)
 * @param pan_id PAN ID (0xFFFF = auto)
 * @param channel Thread channel (11-26)
 * @param master_key Master key (16 bytes, NULL = auto-generate)
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_set_config(
    const char *network_name,
    uint16_t pan_id,
    uint8_t channel,
    const uint8_t *master_key
);

/**
 * @brief Reset Thread network to factory defaults
 * 
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_factory_reset(void);

/**
 * @brief Check if Thread network is active
 * 
 * @return true if Thread network is running
 * @return false if Thread network is not active
 */
bool app_thread_leader_is_running(void);

/**
 * @brief Get Thread Leader role status
 * 
 * @return true if ESP32-C6 is Thread Leader
 * @return false if ESP32-C6 is not Leader
 */
bool app_thread_leader_is_leader(void);

#ifdef __cplusplus
}
#endif

#endif // APP_THREAD_LEADER_H
