/**
 * @brief Rotary Encoder EC11 — GPIO ISR with state-machine debounce
 *        and event flood protection.
 *
 * Uses a 4-state quadrature decoder to reject noise. Each detent = 4 state
 * changes = ±1 step. Callback throttled to ENCODER_MIN_EVENT_MS to prevent
 * queue flooding during rapid turns while maintaining accurate tracking.
 */
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

class RotaryEncoder {
public:
    typedef void (*on_change_callback)(int new_value);

    RotaryEncoder(on_change_callback callback)
        : m_callback(callback), m_current_step(0), m_last_state(0),
          m_accumulator(0), m_last_event_us(0) {}

    esp_err_t init() {
        gpio_config_t clk_cfg = {};
        clk_cfg.intr_type    = GPIO_INTR_ANYEDGE;
        clk_cfg.mode         = GPIO_MODE_INPUT;
        clk_cfg.pin_bit_mask = (1ULL << PIN_ENCODER_CLK);
        clk_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
        esp_err_t ret = gpio_config(&clk_cfg);
        if (ret != ESP_OK) return ret;

        gpio_config_t dt_cfg = {};
        dt_cfg.intr_type    = GPIO_INTR_DISABLE;
        dt_cfg.mode         = GPIO_MODE_INPUT;
        dt_cfg.pin_bit_mask = (1ULL << PIN_ENCODER_DT);
        dt_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
        ret = gpio_config(&dt_cfg);
        if (ret != ESP_OK) return ret;

        m_last_state = (gpio_get_level(PIN_ENCODER_CLK) << 1) | gpio_get_level(PIN_ENCODER_DT);

        ret = gpio_isr_handler_add(PIN_ENCODER_CLK, isr_handler, this);
        if (ret != ESP_OK) { printf("E ROTARY: ISR add failed\n"); return ret; }

        printf("I ROTARY: GPIO ISR ready (CLK=%d DT=%d, min_event=%lu ms)\n",
               PIN_ENCODER_CLK, PIN_ENCODER_DT, (unsigned long)ENCODER_MIN_EVENT_MS);
        return ESP_OK;
    }

    int get_value() const { return m_current_step; }

    void set_value(int step) {
        if (step < 0) step = 0;
        if (step > ENCODER_MAX_STEPS) step = ENCODER_MAX_STEPS;
        m_current_step = step;
        m_accumulator = 0;
    }

private:
    on_change_callback m_callback;
    volatile int m_current_step;
    volatile int m_last_state;
    volatile int m_accumulator;
    volatile int64_t m_last_event_us;

    static void IRAM_ATTR isr_handler(void *arg) {
        auto *self = static_cast<RotaryEncoder*>(arg);
        int clk = gpio_get_level(PIN_ENCODER_CLK);
        int dt  = gpio_get_level(PIN_ENCODER_DT);
        int state = (clk << 1) | dt;

        static const int8_t TRANSITIONS[16] = {
             0,  1, -1,  0,
            -1,  0,  0,  1,
             1,  0,  0, -1,
             0, -1,  1,  0
        };

        int idx = (self->m_last_state << 2) | state;
        if (idx >= 0 && idx < 16) {
            int delta = TRANSITIONS[idx];
            if (delta != 0) {
                self->m_accumulator += delta;
                int step_change = self->m_accumulator / 4;
                if (step_change != 0) {
                    self->m_accumulator -= step_change * 4;
                    int step = self->m_current_step + step_change;
                    if (step < 0) step = 0;
                    if (step > ENCODER_MAX_STEPS) step = ENCODER_MAX_STEPS;
                    if (step != self->m_current_step) {
                        self->m_current_step = step;

                        // Flood protection: throttle callback rate while
                        // keeping position tracking accurate in ISR.
                        int64_t now = esp_timer_get_time();
                        if ((now - self->m_last_event_us) >= (ENCODER_MIN_EVENT_MS * 1000)) {
                            self->m_last_event_us = now;
                            if (self->m_callback) self->m_callback(step);
                        }
                    }
                }
            }
        }
        self->m_last_state = state;
    }
};

#endif // ROTARY_ENCODER_H
