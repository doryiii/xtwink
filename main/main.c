#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "network.h"
#include "power.h"
#include "input.h"
#include "app_display.h"
#include "test_img.h"

static void btn_callback(int btn, int state) {
    if (state == 1) { // Pressed
        if (btn == BTN_LEFT) {
            app_display_send_cmd(DISPLAY_CMD_PREV_IMAGE);
        } else if (btn == BTN_RIGHT) {
            app_display_send_cmd(DISPLAY_CMD_NEXT_IMAGE);
        } else if (btn == BTN_UP) {
            app_display_send_cmd(DISPLAY_CMD_FULL_REFRESH);
        } else if (btn == BTN_DOWN) {
            app_display_send_cmd(DISPLAY_CMD_TEST_PATTERN);
        }
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize Power & Buttons
    power_init();
    power_register_shutdown_cb(app_display_shutdown);
    input_init();
    input_set_button_callback(btn_callback);
    xTaskCreate(&power_button_task, "power_btn", 2048, NULL, 10, NULL);

    // Initialize Display
    app_display_init();
    app_display_set_images(image_array, image_count);

    // Initialize Network
    network_init();
    network_send_notification("/started");

    // Start background HTTP loop
    xTaskCreate(&network_http_test_task, "http_loop", 4096, NULL, 5, NULL);
}
