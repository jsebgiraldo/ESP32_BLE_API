/*
 * user_carrier_wave.h
 *
 *  Created on: Oct 17, 2022
 *      Author: jsebgiraldo
 */

#ifndef EX_USER_MCPWM_H_
#define EX_USER_MCPWM_H_

/* Includes -------------------------------------------------------------------*/

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_rom_gpio.h"
#include "soc/mcpwm_periph.h"
#include "hal/gpio_hal.h"
#include "esp_log.h"
#include "esp_check.h"
#include "soc/rtc.h"
#include "driver/mcpwm.h"


#define TIMER2_OUTPUT_GPIO GPIO_NUM_18

void external_wave_start(void);
void external_wave_stop(void);
void external_wave_setup(void);
void external_wave_config(mcpwm_config_t *config);

#endif /* EX_USER_MCPWM_H_ */
