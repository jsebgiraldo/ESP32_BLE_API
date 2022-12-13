/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SVC_DEVICE_BATTERY_INST_ID                 0
#define SVC_HEART_RATE_INST_ID                     1
#define SVC_DEVICE_CONFIG_INST_ID                  2
#define SVC_CARRIER_WAVE_INST_ID                   3
#define SVC_EXTERNAL_WAVE_INST_ID                  4
#define SVC_INTENSITY_MODULATION_ID                5

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,
    IDX_CHAR_CFG_A_2,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,
    IDX_CHAR_CFG_B,
    IDX_CHAR_CFG_B_2,

    IDX_CHAR_C,
    IDX_CHAR_VAL_C,
    IDX_CHAR_CFG_C,
    IDX_CHAR_CFG_C_2,

    HRS_IDX_NB,
};

void user_ble_start(void);

void user_ble_notify_battery_level(uint8_t *value);
void user_ble_notify_heart_rate(uint8_t *value);