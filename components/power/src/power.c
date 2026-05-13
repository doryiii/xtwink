#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include <math.h>
#include "power.h"
#include "network.h"
#include "adc_shared.h"

static const char *TAG = "POWER";

static power_shutdown_cb_t s_shutdown_cb = NULL;
static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static adc_cali_handle_t s_adc1_cali_handle = NULL;
static bool s_do_calibration = false;

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

    if (adc_shared_get_unit1_handle(&s_adc1_handle) == ESP_OK) {
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN_DB_12,
        };
        adc_oneshot_config_channel(s_adc1_handle, ADC_CHANNEL_0, &config);

        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .chan = ADC_CHANNEL_0,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        if (adc_cali_create_scheme_curve_fitting(&cali_config, &s_adc1_cali_handle) == ESP_OK) {
            s_do_calibration = true;
        } else {
            ESP_LOGW(TAG, "Calibration scheme creation failed, using raw values");
        }
    } else {
        ESP_LOGE(TAG, "Failed to get shared ADC unit");
    }
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

int power_get_battery_percentage(void)
{
    int raw;
    if (adc_oneshot_read(s_adc1_handle, ADC_CHANNEL_0, &raw) != ESP_OK) {
        return 0;
    }
    int voltage = 0;
    if (s_do_calibration) {
        adc_cali_raw_to_voltage(s_adc1_cali_handle, raw, &voltage);
    } else {
        voltage = raw; // fallback
    }

    double volts = (voltage * 2.0) / 1000.0;
    double y = -144.9390 * volts * volts * volts +
               1655.8629 * volts * volts -
               6158.8520 * volts +
               7501.3202;
    if (y < 0.0) y = 0.0;
    if (y > 100.0) y = 100.0;
    return (int)(y + 0.5);
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