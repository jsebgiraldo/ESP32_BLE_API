/*
 * user_dac.h
 *
 *  Created on: Oct 17, 2022
 *      Author: jsebgiraldo
 */

#ifndef INC_USER_DAC_H_
#define INC_USER_DAC_H_


/* Includes -------------------------------------------------------------------*/

#include <driver/dac.h>

#include "freertos/FreeRTOS.h"

typedef struct modulation_wave_config
{
    uint16_t T1;
    uint16_t T2;
    uint16_t T3;
    uint8_t max_intensity;
}modulation_wave_config_t;

/**
 * Message IDs for the WiFi application task
 * @note Expand this based on your application requirements.
 */
typedef enum dac_app_message
{
    DAC_APP_MSG_START_OPERATION = 0,
    DAC_APP_MSG_STOP_OPERATION,
    DAC_APP_MSG_RISING_STATE,
    DAC_APP_MSG_FALLING_STATE,
    DAC_APP_MSG_MAX_INTENSITY_STATE,
    DAC_APP_MSG_STAY_STATE
} dac_app_message_e;

/**
 * Structure for the message queue
 * @note Expand this based on application requirements e.g. add another type and parameter as required
 */
typedef struct dac_app_queue_message
{
	dac_app_message_e msgID;
} dac_app_queue_message_t;


void dac_modulation_wave_setup(void);
void dac_modulation_wave_start(void);
void dac_modulation_wave_stop(void);
void dac_modulation_wave_configure(modulation_wave_config_t *configuration);

void timer_treatmnet_start(void);
void timer_treatmnet_stop(void);
void timer_treatmnet_change_period(uint8_t time);

BaseType_t dac_app_send_message(dac_app_message_e msgID);

#endif /* INC_USER_DAC_H_ */
