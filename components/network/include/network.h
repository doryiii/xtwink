#pragma once

#include "esp_err.h"

/**
 * @brief Initialize WiFi and connect to configured AP
 */
void network_init(void);

/**
 * @brief Send a GET notification to the configured server
 * 
 * @param endpoint The endpoint to append to the base URL (e.g., "/started")
 */
void network_send_notification(const char* endpoint);

/**
 * @brief Task for continuous HTTP testing
 */
void network_http_test_task(void *pvParameters);
