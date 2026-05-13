#include "adc_shared.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "ADC_SHARED";

static adc_oneshot_unit_handle_t s_adc1_handle = NULL;
static SemaphoreHandle_t s_mutex = NULL;

esp_err_t adc_shared_get_unit1_handle(adc_oneshot_unit_handle_t *out_handle) {
    if (!out_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (s_mutex == NULL) {
        // Technically this has a small race condition if called concurrently from two threads before mutex is created,
        // but typically init happens sequentially or we can just use a critical section. 
        // For our use case, app_main initializes components in order, so this is fine.
        static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&mux);
        if (s_mutex == NULL) {
            s_mutex = xSemaphoreCreateMutex();
        }
        portEXIT_CRITICAL(&mux);
    }

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (s_adc1_handle != NULL) {
        *out_handle = s_adc1_handle;
        xSemaphoreGive(s_mutex);
        return ESP_OK;
    }

    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t err = adc_oneshot_new_unit(&init_config1, &s_adc1_handle);
    if (err == ESP_OK) {
        *out_handle = s_adc1_handle;
    } else {
        ESP_LOGE(TAG, "Failed to initialize ADC1: %s", esp_err_to_name(err));
        *out_handle = NULL;
    }

    xSemaphoreGive(s_mutex);
    return err;
}
