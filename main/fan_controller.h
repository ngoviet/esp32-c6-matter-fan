#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <stdio.h>
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "config.h"

class FanController {
public:
    FanController() : m_current_speed_percent(0), m_last_speed_percent(50),
                      m_is_on(false), m_current_freq(FAN_MIN_FREQ) {}

    esp_err_t init() {
        ledc_timer_config_t timer_cfg = {};
        timer_cfg.speed_mode       = FAN_LEDC_MODE;
        timer_cfg.timer_num        = FAN_LEDC_TIMER;
        timer_cfg.duty_resolution  = FAN_DUTY_RES;
        timer_cfg.freq_hz          = FAN_MIN_FREQ;
        timer_cfg.clk_cfg          = LEDC_AUTO_CLK;

        esp_err_t ret = ledc_timer_config(&timer_cfg);
        if (ret != ESP_OK) {
            printf("E FAN: Timer config failed!\n");
            return ret;
        }

        ledc_channel_config_t chan_cfg = {};
        chan_cfg.speed_mode     = FAN_LEDC_MODE;
        chan_cfg.channel        = FAN_LEDC_CHAN;
        chan_cfg.timer_sel      = FAN_LEDC_TIMER;
        chan_cfg.duty           = 0;
        chan_cfg.hpoint         = 0;
        chan_cfg.gpio_num       = PIN_PWM_OUT;

        ret = ledc_channel_config(&chan_cfg);
        if (ret != ESP_OK) {
            printf("E FAN: Channel config failed!\n");
            return ret;
        }

        turn_off();
        return ESP_OK;
    }

    void set_speed(uint8_t percentage) {
        if (percentage > 100) percentage = 100;
        if (percentage == m_current_speed_percent && m_is_on) return;
        m_current_speed_percent = percentage;

        if (percentage == 0) {
            turn_off();
            return;
        }

        // Ensure 50% duty is applied (only needed once after off→on)
        if (!m_is_on) {
            uint32_t duty = (1 << 13) / 2;
            ledc_set_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN, duty);
            ledc_update_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN);
            m_is_on = true;
        }

        uint32_t target_freq = _percent_to_freq(percentage);
        _ramp_freq(target_freq);

        printf("I FAN: Speed %u%% (%lu Hz)\n", percentage, target_freq);
    }

    void turn_on() {
        if (m_is_on) return;

        // Determine starting speed
        if (m_current_speed_percent == 0 && m_last_speed_percent > 0) {
            m_current_speed_percent = m_last_speed_percent;
        }
        if (m_current_speed_percent == 0) m_current_speed_percent = 50;

        // Start ramp from minimum frequency
        uint8_t target_pct = m_current_speed_percent;
        m_current_speed_percent = 0;       // force set_speed to proceed
        m_current_freq = FAN_MIN_FREQ;     // ramp starts from min

        set_speed(target_pct);            // sets duty, ramps freq, sets m_is_on=true

        printf("I FAN: ON at %u%%\n", target_pct);
    }

    void turn_off() {
        if (m_is_on) {
            m_is_on = false;
            m_last_speed_percent = m_current_speed_percent;
            m_current_speed_percent = 0;
            ledc_set_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN, 0);
            ledc_update_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN);
            printf("I FAN: OFF\n");
        }
    }

    bool is_on() const { return m_is_on; }
    uint8_t get_speed() const { return m_is_on ? m_current_speed_percent : 0; }
    uint8_t get_last_speed() const { return m_last_speed_percent; }

private:
    uint8_t m_current_speed_percent;
    uint8_t m_last_speed_percent;
    bool m_is_on;
    uint32_t m_current_freq; // Actual current output frequency

    uint32_t _percent_to_freq(uint8_t pct) {
        return FAN_MIN_FREQ + (static_cast<uint32_t>(pct) * (FAN_MAX_FREQ - FAN_MIN_FREQ) / 100);
    }

    void _apply_freq(uint32_t freq) {
        ledc_set_freq(FAN_LEDC_MODE, FAN_LEDC_TIMER, freq);
        m_current_freq = freq;
    }

    void _ramp_freq(uint32_t target_freq) {
        if (target_freq == m_current_freq) return;

        int32_t total_delta = static_cast<int32_t>(target_freq) - static_cast<int32_t>(m_current_freq);
        int steps = FAN_RAMP_TIME_MS / FAN_RAMP_STEP_MS;
        if (steps < 1) steps = 1;

        int32_t delta_per_step = total_delta / steps;
        if (delta_per_step == 0) delta_per_step = (total_delta > 0) ? 1 : -1;

        uint32_t freq = m_current_freq;
        for (int i = 0; i < steps; i++) {
            freq += delta_per_step;
            if (delta_per_step > 0 && freq >= target_freq) { freq = target_freq; break; }
            if (delta_per_step < 0 && freq <= target_freq) { freq = target_freq; break; }
            _apply_freq(freq);
            vTaskDelay(pdMS_TO_TICKS(FAN_RAMP_STEP_MS));
        }
        _apply_freq(target_freq);
    }
};

#endif // FAN_CONTROLLER_H
