#ifndef SYSTEM_MANAGER_H
#define SYSTEM_MANAGER_H

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_task_wdt.h"
#include "fan_controller.h"
#include "rotary_encoder.h"
#include "button.h"
#include "led_indicator.h"
#include "config.h"
#include "app_matter.h"

enum class SystemEvent {
    ENCODER_CHANGED,
    BUTTON_PRESSED
};

struct EventMessage {
    SystemEvent type;
    int value;
};

class SystemManager {
public:
    SystemManager() : m_fan(nullptr), m_encoder(nullptr), m_button(nullptr),
                      m_led(nullptr), m_event_queue(nullptr), m_task_handle(nullptr),
                      m_disable_sync_until(0) {}

    esp_err_t init() {
        printf("I SYSTEM: Initializing...\n");

        s_instance = this;

        m_event_queue = xQueueCreate(10, sizeof(EventMessage));
        if (m_event_queue == nullptr) return ESP_FAIL;

        // Fan Controller
        m_fan = new FanController();
        esp_err_t ret = m_fan->init();
        if (ret != ESP_OK) return ret;

        // Status LED
        m_led = new LedIndicator();
        ret = m_led->init();
        if (ret != ESP_OK) return ret;
        m_led->set_pattern(LedIndicator::Pattern::FAST_BLINK);

        // Rotary Encoder
        m_encoder = new RotaryEncoder([](int step) {
            EventMessage msg = { SystemEvent::ENCODER_CHANGED, step };
            SystemManager::send_event_from_isr(msg);
        });
        if (m_encoder->init() != ESP_OK) return ESP_FAIL;

        // Button (short = toggle, long 10s = factory reset)
        m_button = new Button(
            []() {
                EventMessage msg = { SystemEvent::BUTTON_PRESSED, 0 };
                SystemManager::send_event(msg);
            },
            []() {
                printf("I SYSTEM: Factory reset via long press!\n");
                app_matter_factory_reset();
            }
        );
        if (m_button->init() != ESP_OK) return ESP_FAIL;

        // Event task
        xTaskCreate(event_task_wrapper, "sys_event", 4096, this, 10, &m_task_handle);

        // Matter → encoder sync callback
        app_matter_register_speed_callback([](uint8_t percent) {
            if (s_instance) {
                s_instance->sync_encoder_step(percent_to_step(percent));
            }
        });

        printf("I SYSTEM: Ready.\n");
        return ESP_OK;
    }

    static void send_event_from_isr(EventMessage msg) {
        if (s_instance && s_instance->m_event_queue) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            xQueueSendFromISR(s_instance->m_event_queue, &msg, &xHigherPriorityTaskWoken);
            if (xHigherPriorityTaskWoken) {
                portYIELD_FROM_ISR();
            }
        }
    }

    static void send_event(EventMessage msg) {
        if (s_instance && s_instance->m_event_queue) {
            xQueueSend(s_instance->m_event_queue, &msg, 0);
        }
    }

    void shutdown() {
        if (m_task_handle) vTaskDelete(m_task_handle);
        delete m_button; delete m_encoder; delete m_led; delete m_fan;
        if (m_event_queue) vQueueDelete(m_event_queue);
    }

    FanController* get_fan_controller() { return m_fan; }
    LedIndicator* get_led() { return m_led; }

    void sync_encoder_step(int step) {
        if (m_encoder && xTaskGetTickCount() > m_disable_sync_until) {
            m_encoder->set_value(step);
        }
    }

private:
    FanController* m_fan;
    RotaryEncoder* m_encoder;
    Button* m_button;
    LedIndicator* m_led;
    QueueHandle_t m_event_queue;
    TaskHandle_t m_task_handle;
    TickType_t m_disable_sync_until;

    static SystemManager* s_instance;

    static void event_task_wrapper(void* arg) {
        static_cast<SystemManager*>(arg)->event_loop();
    }

    void event_loop() {
        EventMessage msg;
        esp_task_wdt_add(NULL);
        printf("I SYSTEM: Event loop started (watchdog armed).\n");

        while (true) {
            // 1-second tick so watchdog is fed even when idle
            if (xQueueReceive(m_event_queue, &msg, pdMS_TO_TICKS(1000)) == pdPASS) {
                handle_event(msg);
            }
            esp_task_wdt_reset();
        }
    }

    void handle_event(const EventMessage& msg) {
        switch (msg.type) {
            case SystemEvent::ENCODER_CHANGED: {
                m_disable_sync_until = xTaskGetTickCount() + pdMS_TO_TICKS(2000);

                uint8_t percentage = step_to_percent(msg.value);
                printf("I SYSTEM: Encoder step %d (%d%%)\n", msg.value, percentage);
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
                printf("I SYSTEM: Button press!\n");
                if (m_fan->is_on()) {
                    m_fan->turn_off();
                    app_matter_report_onoff(false);
                } else {
                    m_fan->turn_on();
                    app_matter_report_onoff(true);
                    app_matter_report_speed(m_fan->get_speed());
                }
                break;
            }
        }
    }
};

inline SystemManager* SystemManager::s_instance = nullptr;

#endif // SYSTEM_MANAGER_H
