#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
#include "driver/ledc.h"

// ========== GPIO CONFIGURATION ==========
// PWM Output (LEDC)
static constexpr gpio_num_t PIN_PWM_OUT = GPIO_NUM_1;

// Rotary Encoder (EC11)
static constexpr gpio_num_t PIN_ENCODER_CLK = GPIO_NUM_3;
static constexpr gpio_num_t PIN_ENCODER_DT   = GPIO_NUM_2;
static constexpr gpio_num_t PIN_ENCODER_SW   = GPIO_NUM_4;

// Status LED (RGB WS2812 on GPIO8 — RMT or simple GPIO toggle)
static constexpr gpio_num_t PIN_STATUS_LED = GPIO_NUM_8;

// ========== FAN PARAMETERS ==========
// Frequency range (Hz)
static constexpr uint32_t FAN_MIN_FREQ = 28;
static constexpr uint32_t FAN_MAX_FREQ = 328;

// Soft-start: ramp speed changes over this many ms
static constexpr uint32_t FAN_RAMP_TIME_MS = 500;
static constexpr uint32_t FAN_RAMP_STEP_MS = 20;

// LEDC Configuration
static constexpr ledc_timer_t FAN_LEDC_TIMER = LEDC_TIMER_0;
static constexpr ledc_mode_t FAN_LEDC_MODE   = LEDC_LOW_SPEED_MODE;
static constexpr ledc_channel_t FAN_LEDC_CHAN = LEDC_CHANNEL_0;
static constexpr ledc_timer_bit_t FAN_DUTY_RES = LEDC_TIMER_13_BIT;

// ========== ENCODER SETTINGS ==========
static constexpr int ENCODER_MAX_STEPS = 100;

// Min interval between encoder events to prevent ISR flood (ms)
static constexpr uint32_t ENCODER_MIN_EVENT_MS = 10;

static inline uint8_t step_to_percent(int step) {
    if (step < 0) return 0;
    if (step > 100) return 100;
    return static_cast<uint8_t>(step);
}

static inline int percent_to_step(uint8_t percent) {
    return (percent > 100) ? 100 : percent;
}

// ========== BUTTON SETTINGS ==========
static constexpr uint32_t BUTTON_DEBOUNCE_MS = 50;

// ========== WATCHDOG ==========
static constexpr uint32_t WATCHDOG_TIMEOUT_SEC = 10;

// ========== LED PATTERNS (ms) ==========
static constexpr uint32_t LED_FAST_BLINK_MS   = 200;
static constexpr uint32_t LED_SLOW_BLINK_MS   = 800;
static constexpr uint32_t LED_RAPID_BLINK_MS  = 80;

#endif // CONFIG_H
