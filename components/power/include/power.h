#pragma once

#include "driver/gpio.h"

#define POWER_BUTTON_PIN GPIO_NUM_3
#define POWER_LATCH_PIN GPIO_NUM_13
#define USB_DETECT_PIN GPIO_NUM_20

/**
 * @brief Initialize power management hardware (GPIOs)
 */
void power_init(void);

/**
 * @brief Check if USB power is connected
 * 
 * @return true if USB is connected, false otherwise
 */
bool power_is_usb_connected(void);

/**
 * @brief Perform shutdown sequence (cut power on battery, deep sleep on USB)
 */
void power_shutdown(void);

typedef void (*power_shutdown_cb_t)(void);
void power_register_shutdown_cb(power_shutdown_cb_t cb);

/**
 * @brief Get the battery percentage
 * 
 * @return int Battery percentage (0-100)
 */
int power_get_battery_percentage(void);

/**
 * @brief Task for monitoring power button
 */
void power_button_task(void *pvParameters);
