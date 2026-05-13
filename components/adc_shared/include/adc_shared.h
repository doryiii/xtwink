#pragma once

#include "esp_adc/adc_oneshot.h"
#include "esp_err.h"

/**
 * @brief Initialize and get the shared ADC unit 1 handle.
 *        If the handle has already been initialized, it returns the existing one.
 * 
 * @param[out] out_handle Pointer to store the ADC handle.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t adc_shared_get_unit1_handle(adc_oneshot_unit_handle_t *out_handle);
