#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
#include "driver/ledc.h"

// ========== GPIO CONFIGURATION ==========
// PWM Output (LEDC)
static constexpr gpio_num_t PIN_PWM_OUT = GPIO_NUM_1;

// Rotary Encoder (ECC11)
static constexpr gpio_num_t PIN_ENCODER_CLK = GPIO_NUM_2;
static constexpr gpio_num_t PIN_ENCODER_DT   = GPIO_NUM_3;
static constexpr gpio_num_t PIN_ENCODER_SW   = GPIO_NUM_4;

// ========== FAN PARAMETERS ==========
// Frequency range (Hz)
static constexpr uint32_t FAN_MIN_FREQ = 100;
static constexpr uint32_t FAN_MAX_FREQ = 400;

// LEDC Configuration
static constexpr ledc_timer_t FAN_LEDC_TIMER = LEDC_TIMER_0;
static constexpr ledc_mode_t FAN_LEDC_MODE   = LEDC_LOW_SPEED_MODE;
static constexpr ledc_channel_t FAN_LEDC_CHAN = LEDC_CHANNEL_0;
static constexpr ledc_timer_bit_t FAN_DUTY_RES = LEDC_TIMER_13_BIT;

// ========== ENCODER SETTINGS ==========
// 1 step = ~3% speed change (based on 33 steps for 100%)
static constexpr int ENCODER_MAX_STEPS = 33;

// Bidirectional mapping: encoder step (0–33) ↔ Matter percent (0–100)
// Integer math with rounding — faster and more precise than float on MCU
static inline uint8_t step_to_percent(int step) {
    int pct = (step * 100 + ENCODER_MAX_STEPS / 2) / ENCODER_MAX_STEPS;
    return static_cast<uint8_t>(pct > 100 ? 100 : (pct < 0 ? 0 : pct));
}

static inline int percent_to_step(uint8_t percent) {
    int step = (static_cast<int>(percent) * ENCODER_MAX_STEPS + 50) / 100;
    return (step > ENCODER_MAX_STEPS ? ENCODER_MAX_STEPS : (step < 0 ? 0 : step));
}

// Matter Level Control cluster uses 0–254 range → map to 0–100%
static inline uint8_t level_to_percent(uint8_t level) {
    int pct = (static_cast<int>(level) * 100 + 127) / 254;
    return static_cast<uint8_t>(pct > 100 ? 100 : pct);
}

// ========== BUTTON SETTINGS ==========
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;

#endif // CONFIG_H
