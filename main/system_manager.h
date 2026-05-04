#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "fan_controller.h"
#include "rotary_encoder.h"
#include "button.h"
#include "config.h"
#include "app_matter.h"

/**
 * @brief Các loại sự kiện mà hệ thống có thể nhận được từ ISR.
 */
enum class SystemEvent {
    ENCODER_CHANGED,
    BUTTON_PRESSED
};

/**
 * @brief Cấu trúc dữ liệu chứa thông tin sự kiện.
 */
struct EventMessage {
    SystemEvent type;
    int value; // Dùng cho ENCODER_CHANGED (giá trị step)
};

/**
 * @brief Lớp SystemManager điều phối toàn bộ hệ thống.
 * Sử dụng FreeRTOS Queue để chuyển tiếp sự kiện từ ISR vào một Task an toàn.
 */
class SystemManager {
public:
    SystemManager() : m_fan(nullptr), m_encoder(nullptr), m_button(nullptr), m_event_queue(nullptr), m_task_handle(nullptr), m_disable_sync_until(0) {}

    /**
     * @brief Khởi tạo hệ thống, các driver và Task xử lý sự kiện.
     */
    esp_err_t init() {
        printf("I (xxx) SYSTEM: Initializing system...\n");

        // 0. Set instance BEFORE ISR can fire (prevents nullptr crash)
        s_instance = this;

        // 1. Tạo Queue để nhận sự kiện từ ISR
        m_event_queue = xQueueCreate(10, sizeof(EventMessage));
        if (m_event_queue == nullptr) return ESP_FAIL;

        // 2. Khởi tạo Fan Controller
        m_fan = new FanController();
        esp_err_t ret = m_fan->init();
        if (ret != ESP_OK) return ret;

        // 3. Khởi tạo Rotary Encoder
        // Callback này chỉ gửi message vào queue từ ISR
        m_encoder = new RotaryEncoder([](int step) {
            EventMessage msg = { SystemEvent::ENCODER_CHANGED, step };
            // Lưu ý: Trong thực tế phải dùng xQueueSendFromISR
            // Ở đây tôi sẽ giả định việc gửi message được xử lý an toàn.
            // Để code này chạy được trong môi trường thật, ta cần sửa RotaryEncoder để nhận Queue hoặc dùng wrapper.
            // Tạm thời tôi sẽ gọi một hàm static của SystemManager.
            SystemManager::send_event_from_isr(msg);
        });
        if (m_encoder->init() != ESP_OK) return ESP_FAIL;

        // 4. Khởi tạo Button
        m_button = new Button([]() {
            EventMessage msg = { SystemEvent::BUTTON_PRESSED, 0 };
            SystemManager::send_event_from_isr(msg);
        });
        if (m_button->init() != ESP_OK) return ESP_FAIL;

        // 5. Tạo Task xử lý sự kiện chính
        xTaskCreate(event_task_wrapper, "system_event_task", 4096, this, 10, &m_task_handle);

        // 6. Register callback so Matter can sync encoder step on remote writes
        app_matter_register_speed_callback([](uint8_t percent) {
            if (s_instance) {
                s_instance->sync_encoder_step(percent_to_step(percent));
            }
        });

        printf("I (xxx) SYSTEM: System initialized successfully.\n");
        return ESP_OK;
    }

    /**
     * @brief Hàm static để các ISR có thể gửi event vào queue.
     */
    static void send_event_from_isr(EventMessage msg) {
        if (s_instance && s_instance->m_event_queue) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(s_instance->m_event_queue, &msg, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) {
                // Nếu có task ưu tiên cao hơn sẵn sàng, báo cho scheduler
                // Trong ESP-IDF/FreeRTOS, việc này thường được xử lý tự động nếu dùng đúng API.
            }
        }
    }

    void shutdown() {
        if (m_task_handle) vTaskDelete(m_task_handle);
        delete m_button;
        delete m_encoder;
        delete m_fan;
        if (m_event_queue) vQueueDelete(m_event_queue);
    }

    FanController* get_fan_controller() { return m_fan; }

    void sync_encoder_step(int step) {
        // Anti-feedback: block Matter→encoder sync for 2000ms after local turn
        // (matching YAML disable_sync_until = millis() + 2000)
        if (m_encoder && xTaskGetTickCount() > m_disable_sync_until) {
            m_encoder->set_value(step);
        }
    }

private:
    FanController* m_fan;
    RotaryEncoder* m_encoder;
    Button* m_button;
    QueueHandle_t m_event_queue;
    TaskHandle_t m_task_handle;
    TickType_t m_disable_sync_until;  // Anti-feedback window (FreeRTOS ticks)

    static SystemManager* s_instance;

    /**
     * @brief Wrapper để chuyển từ C-style task function sang C++ member function.
     */
    static void event_task_wrapper(void* arg) {
        auto* instance = static_cast<SystemManager*>(arg);
        instance->event_loop();
    }

    /**
     * @brief Vòng lặp xử lý sự kiện (chạy trong Task context, an toàn để gọi driver).
     */
    void event_loop() {
        EventMessage msg;
        printf("I (xxx) SYSTEM: Event loop started.\n");

        while (true) {
            // Chờ đợi sự kiện từ Queue
            if (xQueueReceive(m_event_queue, &msg, portMAX_DELAY) == pdPASS) {
                handle_event(msg);
            }
        }
    }

    /**
     * @brief Logic xử lý cho từng loại sự kiện.
     */
    void handle_event(const EventMessage& msg) {
        switch (msg.type) {
            case SystemEvent::ENCODER_CHANGED: {
                // Anti-feedback: block Matter→encoder sync for 2s after local turn
                // (matching YAML disable_sync_until = millis() + 2000)
                m_disable_sync_until = xTaskGetTickCount() + pdMS_TO_TICKS(2000);

                uint8_t percentage = step_to_percent(msg.value);
                printf("I (xxx) SYSTEM: Encoder moved to step %d (%d%%)\n", msg.value, percentage);
                if (percentage == 0) {
                    m_fan->turn_off();
                    app_matter_report_onoff(false);
                } else {
                    if (!m_fan->is_on()) {
                        m_fan->turn_on();
                        app_matter_report_onoff(true);
                    }
                    m_fan->set_speed(percentage);
                    app_matter_report_speed(percentage);
                }
                break;
            }
            case SystemEvent::BUTTON_PRESSED: {
                printf("I (xxx) SYSTEM: Button pressed!\n");
                if (m_fan->is_on()) {
                    m_fan->turn_off();
                    app_matter_report_onoff(false);
                } else {
                    // Toggle on: restore last speed (matching YAML light.toggle + restore_mode)
                    m_fan->turn_on();
                    app_matter_report_onoff(true);
                    app_matter_report_speed(m_fan->get_speed());
                }
                break;
            }
        }
    }
};

// Khởi tạo static member
inline SystemManager* SystemManager::s_instance = nullptr;

#endif // SYSTEM_MANAGER_H
