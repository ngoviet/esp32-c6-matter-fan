#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <stdio.h>
#include "driver/rmt_tx.h"
#include "esp_timer.h"
#include "led_strip_encoder.h"
#include "config.h"

/**
 * @brief RGB LED status indicator using WS2812 via RMT peripheral.
 *
 * Colors:
 *   FAST_BLINK (blue)   — commissioning / waiting for BLE pairing
 *   ON (green)          — Thread connected, operational
 *   SLOW_BLINK (red)    — Thread disconnected
 *   RAPID_BLINK (red)   — factory reset in progress
 */
class LedIndicator {
public:
    enum class Pattern { OFF, ON, FAST_BLINK, SLOW_BLINK, RAPID_BLINK };

    LedIndicator() : m_channel(nullptr), m_encoder(nullptr), m_blink_timer(nullptr),
                     m_current_pattern(Pattern::OFF), m_led_on(false) {}

    esp_err_t init() {
        // RMT TX channel for WS2812
        rmt_tx_channel_config_t tx_cfg = {};
        tx_cfg.gpio_num = PIN_STATUS_LED;
        tx_cfg.clk_src = RMT_CLK_SRC_DEFAULT;
        tx_cfg.resolution_hz = 10 * 1000 * 1000; // 10 MHz = 100ns per tick
        tx_cfg.mem_block_symbols = 64;
        tx_cfg.trans_queue_depth = 4;

        esp_err_t ret = rmt_new_tx_channel(&tx_cfg, &m_channel);
        if (ret != ESP_OK) {
            printf("E LED: RMT TX channel failed: %d\n", ret);
            return ret;
        }

        // WS2812 encoder
        led_strip_encoder_config_t enc_cfg = { .resolution = 10000000 }; // 10 MHz
        ret = rmt_new_led_strip_encoder(&enc_cfg, &m_encoder);
        if (ret != ESP_OK) {
            printf("E LED: RMT encoder failed: %d\n", ret);
            return ret;
        }

        ret = rmt_enable(m_channel);
        if (ret != ESP_OK) {
            printf("E LED: RMT enable failed: %d\n", ret);
            return ret;
        }

        // Turn LED off initially
        uint8_t off[3] = {0, 0, 0};
        _send_pixels(off, 3);

        const esp_timer_create_args_t args = {
            .callback = &LedIndicator::timer_cb, .arg = this, .name = "led_blink"
        };
        esp_timer_create(&args, &m_blink_timer);

        printf("I LED: WS2812 ready on GPIO %d\n", (int)PIN_STATUS_LED);
        return ESP_OK;
    }

    void set_pattern(Pattern p) {
        m_current_pattern = p;
        esp_timer_stop(m_blink_timer);
        m_led_on = false;

        switch (p) {
            case Pattern::OFF:
                _show(0, 0, 0);
                break;
            case Pattern::ON:
                _show(0, 8, 0); // Green (dim)
                break;
            case Pattern::FAST_BLINK:
                _start_blink(0, 0, 8, LED_FAST_BLINK_MS); // Blue (dim)
                break;
            case Pattern::SLOW_BLINK:
                _start_blink(8, 0, 0, LED_SLOW_BLINK_MS); // Red (dim)
                break;
            case Pattern::RAPID_BLINK:
                _start_blink(8, 0, 0, LED_RAPID_BLINK_MS); // Red rapid (dim)
                break;
        }
    }

    Pattern get_pattern() const { return m_current_pattern; }

    void deinit() {
        if (m_blink_timer) { esp_timer_stop(m_blink_timer); esp_timer_delete(m_blink_timer); }
        if (m_channel) { rmt_disable(m_channel); rmt_del_channel(m_channel); }
        if (m_encoder) rmt_del_encoder(m_encoder);
    }

private:
    rmt_channel_handle_t m_channel;
    rmt_encoder_handle_t m_encoder;
    esp_timer_handle_t m_blink_timer;
    Pattern m_current_pattern;
    bool m_led_on;
    uint8_t m_r, m_g, m_b;

    void _show(uint8_t r, uint8_t g, uint8_t b) {
        m_r = r; m_g = g; m_b = b;
        uint8_t pixels[3] = {g, r, b}; // WS2812 GRB order
        _send_pixels(pixels, 3);
    }

    void _send_pixels(uint8_t *data, size_t len) {
        if (!m_channel || !m_encoder) return;
        rmt_transmit_config_t tx_cfg = { .loop_count = 0 };
        rmt_transmit(m_channel, m_encoder, data, len, &tx_cfg);
        rmt_tx_wait_all_done(m_channel, pdMS_TO_TICKS(50));
    }

    void _start_blink(uint8_t r, uint8_t g, uint8_t b, uint32_t interval_ms) {
        m_r = r; m_g = g; m_b = b;
        m_led_on = false;
        _show(0, 0, 0);
        esp_timer_start_periodic(m_blink_timer, interval_ms * 1000);
    }

    static void timer_cb(void* arg) {
        auto* self = static_cast<LedIndicator*>(arg);
        self->m_led_on = !self->m_led_on;
        if (self->m_led_on) self->_show(self->m_r, self->m_g, self->m_b);
        else self->_show(0, 0, 0);
    }
};

#endif // LED_INDICATOR_H
