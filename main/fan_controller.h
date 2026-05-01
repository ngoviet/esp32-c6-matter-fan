#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include <stdio.h>
#include "esp_err.h"
#include "driver/ledc.h"
#include "config.h"

/**
 * @brief Lớp điều khiển quạt sử dụng LEDC (PWM) để thay đổi tần số.
 */
class FanController {
public:
    FanController() : m_current_speed_percent(0), m_last_speed_percent(50), m_is_on(false) {}

    /**
     * @brief Khởi tạo phần cứng LEDC
     * @return esp_err_t ESP_OK nếu thành công
     */
    esp_err_t init() {
        // 1. Cấu hình Timer
        ledc_timer_config_t timer_cfg = {};
        timer_cfg.speed_mode       = FAN_LEDC_MODE;
        timer_cfg.timer_num        = FAN_LEDC_TIMER;
        timer_cfg.duty_resolution  = FAN_DUTY_RES;
        timer_cfg.freq_hz          = FAN_MIN_FREQ;
        timer_cfg.clk_cfg          = LEDC_AUTO_CLK;

        esp_err_t ret = ledc_timer_config(&timer_cfg);
        if (ret != ESP_OK) {
            printf("E (xxx) FAN_CONTROLLER: Timer config failed!\n");
            return ret;
        }

        // 2. Cấu hình Channel
        ledc_channel_config_t chan_cfg = {};
        chan_cfg.speed_mode     = FAN_LEDC_MODE;
        chan_cfg.channel        = FAN_LEDC_CHAN;
        chan_cfg.timer_sel      = FAN_LEDC_TIMER;
        chan_cfg.duty           = 0;
        chan_cfg.hpoint         = 0;
        chan_cfg.gpio_num       = PIN_PWM_OUT;

        ret = ledc_channel_config(&chan_cfg);
        if (ret != ESP_OK) {
            printf("E (xxx) FAN_CONTROLLER: Channel config failed!\n");
            return ret;
        }

        // Đảm bảo quạt tắt khi khởi tạo
        turn_off();
        return ESP_OK;
    }

    /**
     * @brief Thiết lập tốc độ quạt theo phần trăm (0-100)
     * @param percentage Tốc độ từ 0 đến 100
     */
    void set_speed(uint8_t percentage) {
        if (percentage > 100) percentage = 100;
        m_current_speed_percent = percentage;

        if (percentage == 0) {
            turn_off();
            return;
        }

        // Tính toán tần số dựa trên dải [MIN_FREQ, MAX_FREQ]
        // Công thức: Freq = MIN + (percent * (MAX - MIN) / 100)
        uint32_t target_freq = FAN_MIN_FREQ + 
            (static_cast<uint32_t>(percentage) * (FAN_MAX_FREQ - FAN_MIN_FREQ) / 100);

        // Cập nhật tần số LEDC
        esp_err_t ret = ledc_set_freq(FAN_LEDC_MODE, FAN_LEDC_TIMER, target_freq);
        if (ret != ESP_OK) {
            printf("E (xxx) FAN_CONTROLLER: Failed to set frequency %lu Hz\n", target_freq);
            return;
        }

        // Đảm bảo Duty Cycle luôn là 50% để tạo xung vuông chuẩn cho quạt
        // Với độ phân giải 13-bit, 50% = (2^13 / 2) = 4096
        uint32_t duty = (1 << 13) / 2;
        ledc_set_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN, duty);
        ledc_update_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN);

        printf("I (xxx) FAN_CONTROLLER: Speed set to %u%% (%lu Hz)\n", percentage, target_freq);
    }

    void turn_on() {
        if (!m_is_on) {
            m_is_on = true;
            // Restore last speed (matching YAML restore_mode: RESTORE_DEFAULT_OFF)
            if (m_current_speed_percent == 0 && m_last_speed_percent > 0) {
                m_current_speed_percent = m_last_speed_percent;
            }
            if (m_current_speed_percent == 0) m_current_speed_percent = 50;
            set_speed(m_current_speed_percent);
            printf("I (xxx) FAN_CONTROLLER: Fan turned ON at %u%%\n", m_current_speed_percent);
        }
    }

    void turn_off() {
        if (m_is_on) {
            m_is_on = false;
            // Save speed for restore on next turn_on
            m_last_speed_percent = m_current_speed_percent;
            m_current_speed_percent = 0;
            ledc_set_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN, 0);
            ledc_update_duty(FAN_LEDC_MODE, FAN_LEDC_CHAN);
            printf("I (xxx) FAN_CONTROLLER: Fan turned OFF\n");
        }
    }

    bool is_on() const { return m_is_on; }
    uint8_t get_speed() const { return m_current_speed_percent; }

private:
    uint8_t m_current_speed_percent;
    uint8_t m_last_speed_percent;
    bool m_is_on;
};

#endif // FAN_CONTROLLER_H
