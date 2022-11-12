/*
 * user_timer.h
 *
*  Created on: Apr 04, 2022
 *      Author: Juan Sebastian Giraldo Duque
 */

#ifndef USER_TIMER_H_
#define USER_TIMER_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_sleep.h"

#define DEEP_SLEEP_TIMEOUT 60*1000

void deep_sleep_timer_start(void);
void deep_sleep_timer_stop(void);
void deep_sleep_setup_timer(void);

#endif /* USER_TIMER_H_ */