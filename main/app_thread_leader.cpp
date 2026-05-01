/**
 * @file app_thread_leader.cpp
 * @brief Thread Leader (Border Router) implementation for ESP32-C6 Smart Fan
 * 
 * This file implements Thread Leader functionality where ESP32-C6
 * CREATES and MANAGES its own Thread network as a Border Router.
 * 
 * Other Thread devices can join this network and communicate via Matter.
 */

#include "app_thread_leader.h"
#include <esp_log.h>
#include <esp_matter.h>
#include <esp_matter_console.h>
#include <esp_openthread.h>
#include <esp_openthread_border_router.h>
#include <esp_openthread_utils.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "THREAD_LEADER";

// Thread network configuration (stored in NVS)
static thread_leader_info_t s_thread_info = {
    .network_name = "ESP32-C6-FAN",
    .pan_id = 0xABCD,
    .channel = 15,
    .master_key = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                   0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF},
    .is_leader = false,
    .is_running = false
};

// NVS namespace for Thread configuration
static const char *NVS_NAMESPACE = "thread";

/**
 * @brief Generate a random 16-byte Master Key
 */
static void generate_master_key(uint8_t *key) {
    // Use ESP-RNG to generate random master key
    // esp_random() returns 32-bit random value, call it 4 times for 16 bytes
    for (int i = 0; i < 4; i++) {
        uint32_t random_val = esp_random();
        key[i * 4]     = (random_val >> 0) & 0xFF;
        key[i * 4 + 1] = (random_val >> 8) & 0xFF;
        key[i * 4 + 2] = (random_val >> 16) & 0xFF;
        key[i * 4 + 3] = (random_val >> 24) & 0xFF;
    }
    
    // Ensure first byte is even (required by Thread spec)
    key[0] &= 0xFE;
    
    ESP_LOGI(TAG, "Generated random Master Key: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
             key[0], key[1], key[2], key[3], key[4], key[5], key[6], key[7],
             key[8], key[9], key[10], key[11], key[12], key[13], key[14], key[15]);
}

/**
 * @brief Save Thread configuration to NVS
 */
static esp_err_t save_thread_config_to_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return err;
    }
    
    nvs_set_str(handle, "network_name", s_thread_info.network_name);
    nvs_set_u16(handle, "pan_id", s_thread_info.pan_id);
    nvs_set_u8(handle, "channel", s_thread_info.channel);
    nvs_set_blob(handle, "master_key", s_thread_info.master_key, 16);
    
    nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Thread config saved to NVS");
    return ESP_OK;
}

/**
 * @brief Load Thread configuration from NVS
 */
static esp_err_t load_thread_config_from_nvs(void) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No Thread config in NVS, using defaults");
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    size_t name_len = sizeof(s_thread_info.network_name);
    nvs_get_str(handle, "network_name", s_thread_info.network_name, &name_len);
    s_thread_info.pan_id = nvs_get_u16(handle, "pan_id");
    s_thread_info.channel = nvs_get_u8(handle, "channel");
    nvs_get_blob(handle, "master_key", s_thread_info.master_key, &name_len);
    
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Thread config loaded from NVS:");
    ESP_LOGI(TAG, "  Network Name: %s", s_thread_info.network_name);
    ESP_LOGI(TAG, "  PAN ID: 0x%04X", s_thread_info.pan_id);
    ESP_LOGI(TAG, "  Channel: %d", s_thread_info.channel);
    
    return ESP_OK;
}

/**
 * @brief Convert Master Key bytes to Thread string format
 */
static void master_key_to_string(const uint8_t *key, char *output, size_t max_len) {
    // Ensure output buffer is large enough for 32 hex chars + null terminator
    if (max_len < 33) {
        return;
    }
    
    for (size_t i = 0; i < 16; i++) {
        snprintf(output + (i * 2), max_len - (i * 2), "%02x", key[i]);
    }
    output[32] = '\0';
}

/**
 * @brief Get OpenThread instance
 */
static esp_openthread_instance_t get_openthread_instance(void) {
    return esp_openthread_get_default_instance();
}

/**
 * @brief Thread network state change callback
 */
static void thread_state_changed_callback(esp_openthread_state_changed_event_t event, void *arg) {
    ESP_LOGI(TAG, "Thread state changed: event=%d", event);
    
    switch (event) {
        case ESP_OPENTHREAD_STATE_CHANGED_NET_IF_UP:
            ESP_LOGI(TAG, "Thread network interface UP");
            s_thread_info.is_running = true;
            break;
            
        case ESP_OPENTHREAD_STATE_CHANGED_NET_IF_DOWN:
            ESP_LOGI(TAG, "Thread network interface DOWN");
            s_thread_info.is_running = false;
            break;
            
        case ESP_OPENTHREAD_STATE_CHANGED_LEADER_ROLE_ASSIGNED:
            ESP_LOGI(TAG, "✓ ESP32-C6 is now Thread LEADER!");
            s_thread_info.is_leader = true;
            break;
            
        case ESP_OPENTHREAD_STATE_CHANGED_COMMISSIONING_STARTED:
            ESP_LOGI(TAG, "Thread commissioning started");
            break;
            
        case ESP_OPENTHREAD_STATE_CHANGED_COMMISSIONING_FAILED:
            ESP_LOGW(TAG, "Thread commissioning failed");
            break;
            
        case ESP_OPENTHREAD_STATE_CHANGED_COMMISSIONING_COMPLETED:
            ESP_LOGI(TAG, "Thread commissioning completed");
            break;
            
        default:
            break;
    }
}

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
esp_err_t app_thread_leader_init(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Initializing Thread Leader Mode");
    ESP_LOGI(TAG, "========================================");
    
    // Load configuration from NVS (or use defaults)
    if (load_thread_config_from_nvs() != ESP_OK) {
        ESP_LOGI(TAG, "Using default Thread network configuration");
    }
    
    // Print current configuration
    char master_key_str[33] = {0};
    master_key_to_string(s_thread_info.master_key, master_key_str, sizeof(master_key_str));
    
    ESP_LOGI(TAG, "Thread Network Configuration:");
    ESP_LOGI(TAG, "  Network Name: %s", s_thread_info.network_name);
    ESP_LOGI(TAG, "  PAN ID: 0x%04X", s_thread_info.pan_id);
    ESP_LOGI(TAG, "  Channel: %d", s_thread_info.channel);
    ESP_LOGI(TAG, "  Master Key: %s", master_key_str);
    
    // Initialize OpenThread
    esp_openthread_instance_t ot_instance = get_openthread_instance();
    if (ot_instance == NULL) {
        ESP_LOGE(TAG, "Failed to get OpenThread instance");
        return ESP_FAIL;
    }
    
    // Initialize OpenThread with Border Router
    esp_err_t err = esp_openthread_border_router_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize OpenThread Border Router: %s", esp_err_to_name(err));
        return err;
    }
    
    // Register state change callback
    esp_openthread_register_state_changed_callback(thread_state_changed_callback, NULL);
    
    // Start Thread network as Leader
    ESP_LOGI(TAG, "Starting Thread network as LEADER...");
    
    // Set Thread network parameters
    ot_instance->set_pan_id(ot_instance, s_thread_info.pan_id);
    ot_instance->set_channel(ot_instance, s_thread_info.channel);
    
    // Set Master Key
    ot_openthread_key_t master_key;
    memcpy(master_key.key, s_thread_info.master_key, 16);
    master_key.key[0] &= 0xFE;  // Ensure first byte is even
    master_key.seq = 0;
    master_key.timeout = 0;
    ot_instance->set_master_key(ot_instance, &master_key);
    
    // Set network name
    ot_instance->set_network_name(ot_instance, s_thread_info.network_name);
    
    // Start the Thread network
    err = esp_openthread_start(ot_instance);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start OpenThread: %s", esp_err_to_name(err));
        return err;
    }
    
    // Wait for Thread to initialize and become Leader
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Check if we became Leader
    if (s_thread_info.is_leader) {
        ESP_LOGI(TAG, "✓ Thread Leader is ACTIVE!");
        ESP_LOGI(TAG, "  Other Thread devices can now join this network");
    } else {
        ESP_LOGW(TAG, "Thread started but Leader role not yet assigned");
        ESP_LOGW(TAG, "Waiting for Leader election...");
    }
    
    // Save configuration to NVS
    save_thread_config_to_nvs();
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Thread Leader Mode Ready!");
    ESP_LOGI(TAG, "========================================");
    
    return ESP_OK;
}

/**
 * @brief Stop Thread Leader mode
 * 
 * Tears down the Thread network and stops Border Router.
 * 
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_stop(void) {
    ESP_LOGI(TAG, "Stopping Thread Leader mode...");
    
    esp_openthread_instance_t ot_instance = get_openthread_instance();
    if (ot_instance != NULL) {
        esp_openthread_stop(ot_instance);
    }
    
    esp_openthread_border_router_stop();
    
    s_thread_info.is_leader = false;
    s_thread_info.is_running = false;
    
    ESP_LOGI(TAG, "Thread Leader mode stopped");
    return ESP_OK;
}

/**
 * @brief Get current Thread network information
 * 
 * @param info Pointer to thread_leader_info_t to fill
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_get_info(thread_leader_info_t *info) {
    if (info == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(info, &s_thread_info, sizeof(thread_leader_info_t));
    return ESP_OK;
}

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
) {
    if (network_name != NULL) {
        strncpy(s_thread_info.network_name, network_name, sizeof(s_thread_info.network_name) - 1);
        s_thread_info.network_name[sizeof(s_thread_info.network_name) - 1] = '\0';
    }
    
    if (pan_id != 0xFFFF) {
        s_thread_info.pan_id = pan_id;
    }
    
    if (channel >= 11 && channel <= 26) {
        s_thread_info.channel = channel;
    }
    
    if (master_key != NULL) {
        memcpy(s_thread_info.master_key, master_key, 16);
    }
    
    ESP_LOGI(TAG, "Thread config updated");
    return ESP_OK;
}

/**
 * @brief Reset Thread network to factory defaults
 * 
 * @return esp_err_t ESP_OK if successful
 */
esp_err_t app_thread_leader_factory_reset(void) {
    ESP_LOGI(TAG, "Resetting Thread network to factory defaults...");
    
    // Stop Thread if running
    if (s_thread_info.is_running) {
        app_thread_leader_stop();
    }
    
    // Erase Thread NVS namespace
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err == ESP_OK) {
        nvs_erase_all(handle);
        nvs_close(handle);
    }
    
    // Reset to defaults
    strncpy(s_thread_info.network_name, "ESP32-C6-FAN", sizeof(s_thread_info.network_name) - 1);
    s_thread_info.pan_id = 0xABCD;
    s_thread_info.channel = 15;
    generate_master_key(s_thread_info.master_key);
    
    s_thread_info.is_leader = false;
    s_thread_info.is_running = false;
    
    ESP_LOGI(TAG, "Thread network reset to factory defaults");
    return ESP_OK;
}

/**
 * @brief Check if Thread network is active
 * 
 * @return true if Thread network is running
 * @return false if Thread network is not active
 */
bool app_thread_leader_is_running(void) {
    return s_thread_info.is_running;
}

/**
 * @brief Get Thread Leader role status
 * 
 * @return true if ESP32-C6 is Thread Leader
 * @return false if ESP32-C6 is not Leader
 */
bool app_thread_leader_is_leader(void) {
    return s_thread_info.is_leader;
}
