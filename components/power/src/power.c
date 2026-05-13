#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "power.h"
#include "network.h"

static const char *TAG = "POWER";

static power_shutdown_cb_t s_shutdown_cb = NULL;

void power_register_shutdown_cb(power_shutdown_cb_t cb)
{
    s_shutdown_cb = cb;
}

void power_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << POWER_BUTTON_PIN) | (1ULL << USB_DETECT_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

bool power_is_usb_connected(void)
{
    return gpio_get_level(USB_DETECT_PIN) == 1;
}

void power_shutdown(void)
{
    ESP_LOGI(TAG, "Shutting down...");
    
    if (s_shutdown_cb) {
        s_shutdown_cb();
    }

    network_send_notification("/shutting_down");

    gpio_set_direction(POWER_LATCH_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(POWER_LATCH_PIN, 0);

    esp_sleep_config_gpio_isolate();
    gpio_deep_sleep_hold_en();
    gpio_hold_en(POWER_LATCH_PIN);

    esp_sleep_enable_gpio_wakeup();
    esp_deep_sleep_start();
}

void power_button_task(void *pvParameters)
{
    while (gpio_get_level(POWER_BUTTON_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    int press_count = 0;
    while (1) {
        if (gpio_get_level(POWER_BUTTON_PIN) == 0) {
            press_count++;
            if (press_count >= 20) {
                if (power_is_usb_connected()) {
                    ESP_LOGI(TAG, "USB power detected, ignoring shutdown request");
                    network_send_notification("/power/button/shutdown_ignored_usb");
                    // Wait for release to avoid multiple notifications
                    while (gpio_get_level(POWER_BUTTON_PIN) == 0) {
                        vTaskDelay(pdMS_TO_TICKS(50));
                    }
                    press_count = 0;
                } else {
                    power_shutdown();
                }
            }
        } else {
            press_count = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
