#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdio.h>
#include "driver/gpio.h"
#include "config.h"

/**
 * @brief Lớp điều khiển Rotary Encoder sử dụng ngắt (Interrupt) để đọc tín hiệu CLK và DT.
 * 
 * Design: Callback được gọi từ ISR nhưng chỉ thực hiện thao tác nhanh.
 * Logic xử lý nặng nên được thực hiện trong task context.
 */
class RotaryEncoder {
public:
    // Callback function type - chỉ dùng cho thao tác nhanh trong ISR
    typedef void (*on_change_callback)(int new_value);

    RotaryEncoder(on_change_callback callback) 
        : m_callback(callback), m_current_step(0), m_last_clk_state(false) {}

    /**
     * @brief Khởi tạo các chân GPIO cho Encoder
     * @return esp_err_t ESP_OK nếu thành công
     */
    esp_err_t init() {
        // 1. Cấu hình chân CLK (Interrupt pin)
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_ANYEDGE; // Ngắt khi có bất kỳ sự thay đổi trạng thái nào
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << PIN_ENCODER_CLK);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) return ret;

        // 2. Cấu hình chân DT
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.pin_bit_mask = (1ULL << PIN_ENCODER_DT);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        ret = gpio_config(&io_conf);
        if (ret != ESP_OK) return ret;

        // Đọc trạng thái ban đầu của CLK
        m_last_clk_state = gpio_get_level(PIN_ENCODER_CLK);

        // 3. Đăng ký ISR (Interrupt Service Routine) cho chân CLK
        // Lưu ý: ISR service đã được cài đặt trong main.cpp, chỉ cần đăng ký handler tại đây.
        ret = gpio_isr_handler_add(PIN_ENCODER_CLK, isr_wrapper, reinterpret_cast<void*>(this));
        if (ret != ESP_OK) {
            printf("E (xxx) ROTARY_ENCODER: Failed to install ISR!\n");
            return ret;
        }

        printf("I (xxx) ROTARY_ENCODER: Initialized.\n");
        return ESP_OK;
    }

    int get_value() const { return m_current_step; }

    void set_value(int step) {
        if (step < 0) step = 0;
        if (step > ENCODER_MAX_STEPS) step = ENCODER_MAX_STEPS;
        m_current_step = step;
    }

private:
    on_change_callback m_callback;
    int m_current_step;
    bool m_last_clk_state;

    /**
     * @brief Hàm ISR tĩnh để làm cầu nối giữa C-style interrupt và C++ object.
     */
    static void IRAM_ATTR isr_wrapper(void* arg) {
        auto* instance = static_cast<RotaryEncoder*>(arg);
        instance->handle_interrupt_isr();
    }

    /**
     * @brief Logic xử lý ngắt cơ bản - CHỈ trong ISR context.
     * 
     * Lưu ý: Callback được gọi trong ISR context, chỉ nên thực hiện thao tác nhanh.
     * Để tránh system hang, callback nên chỉ gửi vào queue (xQueueSendFromISR).
     */
    void IRAM_ATTR handle_interrupt_isr() {
        bool current_clk_state = gpio_get_level(PIN_ENCODER_CLK);
        
        // Nếu trạng thái CLK thay đổi từ 0 -> 1 hoặc 1 -> 0
        if (current_clk_state != m_last_clk_state) {
            // Đọc chân DT để xác định hướng xoay
            bool dt_state = gpio_get_level(PIN_ENCODER_DT);

            // Logic chuẩn cho Encoder: Nếu CLK thay đổi và DT khác trạng thái CLK, đó là một bước.
            if (current_clk_state == true) { // Rising edge
                if (dt_state != current_clk_state) {
                    m_current_step++;
                } else {
                    m_current_step--;
                }
            } else { // Falling edge
                if (dt_state == current_clk_state) {
                    m_current_step++;
                } else {
                    m_current_step--;
                }
            }

            // Giới hạn giá trị trong dải [0, ENCODER_MAX_STEPS]
            if (m_current_step < 0) m_current_step = 0;
            if (m_current_step > ENCODER_MAX_STEPS) m_current_step = ENCODER_MAX_STEPS;

            m_last_clk_state = current_clk_state;

            // Gọi callback để thông báo giá trị mới
            // Lưu ý: Callback chạy trong ISR context, chỉ nên thực hiện thao tác nhanh
            if (m_callback) {
                m_callback(m_current_step);
            }
        }
    }
};

#endif // ROTARY_ENCODER_H
