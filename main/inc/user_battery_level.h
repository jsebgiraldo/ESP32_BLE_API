/*
 * user_battery_level.h
 *
 *  Created on: Oct 17, 2022
 *      Author: jsebgiraldo
 */

#ifndef INC_BATT_LEVEL_H_
#define INC_BATT_LEVEL_H_

#include "stdio.h"

void battery_level_init(void);
uint8_t get_battery_level(void);

#endif /* INC_BATT_LEVEL_H_ */