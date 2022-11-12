/*
 * user_bsp.h
 *
 *  Created on: Apr 04, 2022
 *      Author: Juan Sebastian Giraldo Duque
 *      gitlab: https://github.com/jsebgiraldo
 */

#ifndef USER_BSP_H_
#define USER_BSP_H_

#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

#define BUTTON_WAKEUP_LEVEL   0

#define USER_BUTTON_PIN        GPIO_NUM_0
#define USER_BUTTON_HIGH_CHK() (gpio_get_level(USER_BUTTON_PIN) == 1)
#define USER_BUTTON_LOW_CHK()  (gpio_get_level(USER_BUTTON_PIN) == 0)

#define STEP_UP_CONVERTER_PIN  GPIO_NUM_23
#define ENABLE_STEP_UP_CONVERTER() gpio_set_level(GPIO_NUM_23,1)
#define DISABLE_STEP_UP_CONVERTER() gpio_set_level(GPIO_NUM_23,0)

void user_bsp_setup(void);

#endif /* USER_BSP_H_ */