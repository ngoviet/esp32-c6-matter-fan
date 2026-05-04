/**
 * @brief Rotary Encoder EC11 — GPIO ISR with state-machine debounce
 *
 * Uses a 4-state quadrature decoder to reject noise and ensure only valid
 * transitions are counted. Each detent = 4 state changes = ±1 step.
 * GPIO pull-ups must be enabled in hardware.
 *
 * Hardware fix for noisy encoders: add 100nF capacitor between CLK→GND
 * and DT→GND (RC low-pass filter with internal 45kΩ pull-up).
 */
#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

class RotaryEncoder {
public:
    typedef void (*on_change_callback)(int new_value);

    RotaryEncoder(on_change_callback callback)
        : m_callback(callback), m_current_step(0), m_last_state(0), m_accumulator(0) {}

    esp_err_t init() {
        // Configure CLK pin with interrupt
        gpio_config_t clk_cfg = {};
        clk_cfg.intr_type    = GPIO_INTR_ANYEDGE;
        clk_cfg.mode         = GPIO_MODE_INPUT;
        clk_cfg.pin_bit_mask = (1ULL << PIN_ENCODER_CLK);
        clk_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
        esp_err_t ret = gpio_config(&clk_cfg);
        if (ret != ESP_OK) return ret;

        // Configure DT pin (no interrupt, just read)
        gpio_config_t dt_cfg = {};
        dt_cfg.intr_type    = GPIO_INTR_DISABLE;
        dt_cfg.mode         = GPIO_MODE_INPUT;
        dt_cfg.pin_bit_mask = (1ULL << PIN_ENCODER_DT);
        dt_cfg.pull_up_en   = GPIO_PULLUP_ENABLE;
        ret = gpio_config(&dt_cfg);
        if (ret != ESP_OK) return ret;

        // Read initial state
        m_last_state = (gpio_get_level(PIN_ENCODER_CLK) << 1) | gpio_get_level(PIN_ENCODER_DT);

        // Register ISR
        ret = gpio_isr_handler_add(PIN_ENCODER_CLK, isr_handler, this);
        if (ret != ESP_OK) { printf("E ROTARY: ISR add failed\n"); return ret; }

        printf("I ROTARY: GPIO ISR encoder ready (CLK=%d DT=%d)\n",
               PIN_ENCODER_CLK, PIN_ENCODER_DT);
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
    volatile int m_last_state;  // {CLK, DT} as 2-bit state
    volatile int m_accumulator; // Raw edge delta accumulator (÷4 = detent)

    static void IRAM_ATTR isr_handler(void *arg) {
        auto *self = static_cast<RotaryEncoder*>(arg);
        // Read current state: bit1=CLK, bit0=DT
        int clk = gpio_get_level(PIN_ENCODER_CLK);
        int dt  = gpio_get_level(PIN_ENCODER_DT);
        int state = (clk << 1) | dt;

        // Quadrature state machine — only count valid transitions
        // CW:  00→01→11→10→00  CCW: 00→10→11→01→00
        static const int8_t TRANSITIONS[16] = {
             0,  1, -1,  0,  // 00→00(0), 00→01(+1), 00→10(-1), 00→11(invalid)
            -1,  0,  0,  1,  // 01→00(-1), 01→01(0),  01→10(inv), 01→11(+1)
             1,  0,  0, -1,  // 10→00(+1), 10→01(inv), 10→10(0),  10→11(-1)
             0, -1,  1,  0   // 11→00(inv), 11→01(-1), 11→10(+1), 11→11(0)
        };

        int idx = (self->m_last_state << 2) | state;
        if (idx >= 0 && idx < 16) {
            int delta = TRANSITIONS[idx];
            if (delta != 0) {
                // 4X quadrature: 1 detent = 4 edge transitions
                // Accumulate raw deltas, emit step change every ±4
                self->m_accumulator += delta;
                int step_change = self->m_accumulator / 4;
                if (step_change != 0) {
                    self->m_accumulator -= step_change * 4;
                    int step = self->m_current_step + step_change;
                    if (step < 0) step = 0;
                    if (step > ENCODER_MAX_STEPS) step = ENCODER_MAX_STEPS;
                    if (step != self->m_current_step) {
                        self->m_current_step = step;
                        if (self->m_callback) self->m_callback(step);
                    }
                }
            }
        }
        self->m_last_state = state;
    }
};

#endif // ROTARY_ENCODER_H
