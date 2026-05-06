#ifndef BUTTON_H
#define BUTTON_H

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"

/**
 * @brief Button with debounce, short-press, and long-press (10s) detection.
 *
 * Short press (< 10s) → toggle fan on/off.
 * Long press (>= 10s) → factory reset Matter fabric (re-commissioning needed).
 */
class Button {
public:
    typedef void (*on_press_callback)();
    typedef void (*on_long_press_callback)();

    Button(on_press_callback press_cb, on_long_press_callback long_cb = nullptr)
        : m_press_cb(press_cb), m_long_cb(long_cb), m_debounce_timer(nullptr), m_long_press_timer(nullptr), m_is_pressed(false) {}

    esp_err_t init() {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << PIN_ENCODER_SW);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) return ret;

        const esp_timer_create_args_t debounce_args = {
            .callback = &Button::debounce_timer_cb,
            .arg = this,
            .name = "btn_debounce"
        };
        ret = esp_timer_create(&debounce_args, &m_debounce_timer);
        if (ret != ESP_OK) return ret;

        const esp_timer_create_args_t long_press_args = {
            .callback = &Button::long_press_timer_cb,
            .arg = this,
            .name = "btn_long"
        };
        ret = esp_timer_create(&long_press_args, &m_long_press_timer);
        if (ret != ESP_OK) return ret;

        ret = gpio_isr_handler_add(PIN_ENCODER_SW, Button::isr_handler, reinterpret_cast<void*>(this));
        if (ret != ESP_OK) {
            printf("E (xxx) BUTTON: Failed to add ISR handler!\n");
            return ret;
        }

        printf("I (xxx) BUTTON: Initialized (short + long press).\n");
        return ESP_OK;
    }

private:
    on_press_callback m_press_cb;
    on_long_press_callback m_long_cb;
    esp_timer_handle_t m_debounce_timer;
    esp_timer_handle_t m_long_press_timer;
    bool m_is_pressed;

    static void IRAM_ATTR isr_handler(void* arg) {
        auto* instance = static_cast<Button*>(arg);
        esp_timer_start_once(instance->m_debounce_timer, BUTTON_DEBOUNCE_MS * 1000);
    }

    static void debounce_timer_cb(void* arg) {
        auto* instance = static_cast<Button*>(arg);
        bool level = (gpio_get_level(PIN_ENCODER_SW) == 0); // LOW = pressed

        if (level && !instance->m_is_pressed) {
            // Press detected
            instance->m_is_pressed = true;
            esp_timer_start_once(instance->m_long_press_timer, 10 * 1000000); // 10s
        } else if (!level && instance->m_is_pressed) {
            // Release detected
            instance->m_is_pressed = false;
            esp_err_t stopped = esp_timer_stop(instance->m_long_press_timer);
            if (stopped == ESP_OK) {
                // Timer was still running → short press
                if (instance->m_press_cb) instance->m_press_cb();
            }
            // If timer already fired, long-press callback already called
        }
    }

    static void long_press_timer_cb(void* arg) {
        auto* instance = static_cast<Button*>(arg);
        instance->m_is_pressed = false;
        printf("I (xxx) BUTTON: Long press (10s) detected — factory reset!\n");
        if (instance->m_long_cb) instance->m_long_cb();
    }
};

#endif // BUTTON_H
