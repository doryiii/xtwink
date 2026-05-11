#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "network.h"
#include "power.h"
#include "input.h"

static const char *TAG = "APP";

void app_main(void)
{
    // Initialize Power & Buttons
    power_init();
    input_init();
    xTaskCreate(&power_button_task, "power_btn", 2048, NULL, 10, NULL);

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Network
    network_init();
    network_send_notification("/started");

    // Start background HTTP loop
    xTaskCreate(&network_http_test_task, "http_loop", 4096, NULL, 5, NULL);
}
