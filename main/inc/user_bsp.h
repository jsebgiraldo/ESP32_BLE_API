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

#define USER_BUTTON_PIN        GPIO_NUM_35
#define USER_BUTTON_HIGH_CHK() (gpio_get_level(USER_BUTTON_PIN) == 1)
#define USER_BUTTON_LOW_CHK()  (gpio_get_level(USER_BUTTON_PIN) == 0)

#define STEP_UP_CONVERTER_PIN  GPIO_NUM_23
#define ENABLE_STEP_UP_CONVERTER() gpio_set_level(GPIO_NUM_23,1)
#define DISABLE_STEP_UP_CONVERTER() gpio_set_level(GPIO_NUM_23,0)

#define STAND_BY_CHARGER        GPIO_NUM_33
#define CHARGING_SIGNAL         GPIO_NUM_32

#define LED_STAND_BY            GPIO_NUM_13
#define LED_STAND_BY_ON()         gpio_set_level(GPIO_NUM_13,1)
#define LED_STAND_BY_OFF()        gpio_set_level(GPIO_NUM_13,0)

#define LED_CHARGING            GPIO_NUM_14
#define LED_CHARGING_ON()         gpio_set_level(GPIO_NUM_14,1) 
#define LED_CHARGING_OFF()        gpio_set_level(GPIO_NUM_14,0)

void user_bsp_setup(void);

#endif /* USER_BSP_H_ */