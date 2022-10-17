/*
 * user_mcpwm.h
 *
 *  Created on: Oct 17, 2022
 *      Author: jsebgiraldo
 */

#ifndef INC_USER_MCPWM_H_
#define INC_USER_MCPWM_H_

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


#define TARGET_MCPWM_UNIT MCPWM_UNIT_0
#define TIMER0_OUTPUT_GPIO GPIO_NUM_16
#define TIMER1_OUTPUT_GPIO GPIO_NUM_17

void pwm_carrier_setup(void);
void pwm_carrier_start(void);
void pwm_carrier_stop(void);

#endif /* INC_USER_MCPWM_H_ */
