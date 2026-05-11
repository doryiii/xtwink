#pragma once

#include "driver/gpio.h"

#define POWER_BUTTON_PIN GPIO_NUM_3
#define POWER_LATCH_PIN GPIO_NUM_13

/**
 * @brief Initialize power management hardware (GPIOs)
 */
void power_init(void);

/**
 * @brief Perform shutdown sequence (cut power on battery, deep sleep on USB)
 */
void power_shutdown(void);

/**
 * @brief Task for monitoring power button
 */
void power_button_task(void *pvParameters);
