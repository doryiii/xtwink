#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "input.h"
#include "network.h"
#include "adc_shared.h"

static const char *TAG = "INPUT";

#define BUTTON_ADC_PIN_1 GPIO_NUM_1
#define BUTTON_ADC_PIN_2 GPIO_NUM_2
#define POWER_BUTTON_PIN GPIO_NUM_3

static const char* BUTTON_NAMES[] = {
    "back", "confirm", "left", "right", "up", "down", "power"
};

#define ADC_NO_BUTTON 3900

static const int ADC_RANGES_1[] = {ADC_NO_BUTTON, 3100, 2090, 750, -1};
static const int NUM_BUTTONS_1 = 4;

static const int ADC_RANGES_2[] = {ADC_NO_BUTTON, 1120, -1};
static const int NUM_BUTTONS_2 = 2;

static adc_oneshot_unit_handle_t adc1_handle;
static input_btn_cb_t button_callback = NULL;

void input_set_button_callback(input_btn_cb_t cb) {
    button_callback = cb;
}

static int get_button_from_adc(int adc_value, const int ranges[], int num_buttons) {
    for (int i = 0; i < num_buttons; i++) {
        if (ranges[i + 1] < adc_value && adc_value <= ranges[i]) {
            return i;
        }
    }
    return -1;
}

static uint8_t get_button_state() {
    uint8_t state = 0;
    int adc_raw1 = 0, adc_raw2 = 0;

    // Read ADC channels
    // GPIO1 is ADC1 Channel 1
    // GPIO2 is ADC1 Channel 2
    adc_oneshot_read(adc1_handle, ADC_CHANNEL_1, &adc_raw1);
    int btn1 = get_button_from_adc(adc_raw1, ADC_RANGES_1, NUM_BUTTONS_1);
    if (btn1 >= 0) {
        state |= (1 << btn1);
    }

    adc_oneshot_read(adc1_handle, ADC_CHANNEL_2, &adc_raw2);
    int btn2 = get_button_from_adc(adc_raw2, ADC_RANGES_2, NUM_BUTTONS_2);
    if (btn2 >= 0) {
        state |= (1 << (btn2 + 4));
    }

    if (gpio_get_level(POWER_BUTTON_PIN) == 0) {
        state |= (1 << BTN_POWER);
    }

    return state;
}

static void input_task(void *pvParameters) {
    uint8_t current_state = 0;
    uint8_t last_state = 0;
    uint32_t last_debounce_time = 0;
    const uint32_t debounce_delay = 5;

    while (1) {
        uint32_t current_time = pdTICKS_TO_MS(xTaskGetTickCount());
        uint8_t state = get_button_state();

        if (state != last_state) {
            last_debounce_time = current_time;
            last_state = state;
        }

        if ((current_time - last_debounce_time) > debounce_delay) {
            if (state != current_state) {
                uint8_t pressed = state & ~current_state;
                uint8_t released = current_state & ~state;

                for (int i = 0; i < 7; i++) {
                    if (pressed & (1 << i)) {
                        ESP_LOGI(TAG, "Button pressed: %s", BUTTON_NAMES[i]);
                        char endpoint[64];
                        snprintf(endpoint, sizeof(endpoint), "/button/%s/pressed", BUTTON_NAMES[i]);
                        network_send_notification(endpoint);

                        if (button_callback) {
                            button_callback(i, 1);
                        }
                    }
                    if (released & (1 << i)) {
                        ESP_LOGI(TAG, "Button released: %s", BUTTON_NAMES[i]);
                        char endpoint[64];
                        snprintf(endpoint, sizeof(endpoint), "/button/%s/released", BUTTON_NAMES[i]);
                        network_send_notification(endpoint);

                        if (button_callback) {
                            button_callback(i, 0);
                        }
                    }
                }

                current_state = state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void input_init(void) {
    // Get the shared ADC1 handle initialized by the adc_shared component
    if (adc_shared_get_unit1_handle(&adc1_handle) != ESP_OK || adc1_handle == NULL) {
        ESP_LOGE(TAG, "Failed to get shared ADC unit");
        return;
    }

    // Configure ADC channels
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_1, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_2, &config));

    // Note: POWER_BUTTON_PIN is already configured in power_init().

    xTaskCreate(&input_task, "input_task", 4096, NULL, 10, NULL);
}
