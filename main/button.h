#ifndef BUTTON_H
#define BUTTON_H

#include <stdio.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "config.h"

/**
 * @brief Lớp điều khiển Nút nhấn với cơ chế chống rung (Debounce) sử dụng esp_timer.
 */
class Button {
public:
    // Callback function type for when the button is pressed
    typedef void (*on_press_callback)();

    Button(on_press_callback callback) 
        : m_callback(callback), m_debounce_timer(nullptr) {}

    /**
     * @brief Khởi tạo chân GPIO và Timer cho Button
     * @return esp_err_t ESP_OK nếu thành công
     */
    esp_err_t init() {
        // 1. Cấu hình chân GPIO
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_NEGEDGE; // Ngắt khi nhấn (chuyển từ High -> Low)
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pin_bit_mask = (1ULL << PIN_ENCODER_SW);
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        esp_err_t ret = gpio_config(&io_conf);
        if (ret != ESP_OK) return ret;

        // 2. Tạo Timer để xử lý Debounce
        const esp_timer_create_args_t timer_args = {
            .callback = &Button::timer_callback_static,
            .arg = this,
            .name = "button_debounce"
        };

        ret = esp_timer_create(&timer_args, &m_debounce_timer);
        if (ret != ESP_OK) return ret;

        // 3. Đăng ký ISR cho Button
        // Lưu ý: gpio_install_isr_service(0) chỉ được gọi một lần duy nhất trong toàn bộ hệ thống.
        // Ở đây ta giả định nó đã được gọi hoặc sẽ được gọi ở main.
        ret = gpio_isr_handler_add(PIN_ENCODER_SW, Button::isr_wrapper_static, reinterpret_cast<void*>(this));
        if (ret != ESP_OK) {
            printf("E (xxx) BUTTON: Failed to add ISR handler!\n");
            return ret;
        }

        printf("I (xxx) BUTTON: Initialized.\n");
        return ESP_OK;
    }

private:
    on_press_callback m_callback;
    esp_timer_handle_t m_debounce_timer;

    /**
     * @brief Hàm ISR tĩnh được gọi bởi GPIO ngắt.
     */
    static void IRAM_ATTR isr_wrapper_static(void* arg) {
        auto* instance = static_cast<Button*>(arg);
        // Bắt đầu timer để kiểm tra trạng thái sau khi debounce
        esp_timer_start_once(instance->m_debounce_timer, BUTTON_DEBOUNCE_MS * 1000);
    }

    /**
     * @brief Callback của Timer (chạy trong Task context).
     */
    static void timer_callback_static(void* arg) {
        auto* instance = static_cast<Button*>(arg);
        instance->handle_debounce();
    }

    /**
     * @brief Kiểm tra trạng thái thực tế sau khi hết thời gian debounce.
     */
    void handle_debounce() {
        // Nếu chân GPIO vẫn ở mức LOW, nghĩa là nút vẫn đang được nhấn (không phải nhiễu)
        if (gpio_get_level(PIN_ENCODER_SW) == 0) {
            if (m_callback) {
                m_callback();
            }
        }
    }
};

#endif // BUTTON_H
