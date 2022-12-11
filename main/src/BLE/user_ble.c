/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/********************************************************************************
*
* This file is for gatt server. It can send adv data, and get connected by client.
*
*********************************************************************************/


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "user_ble.h"
#include "esp_gatt_common_api.h"

#define DEBUG_ON  0

#if DEBUG_ON
#define EXAMPLE_DEBUG ESP_LOGI
#else
#define EXAMPLE_DEBUG( tag, format, ... )
#endif

#define EXAMPLE_TAG "BLE_COMP"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "API_TECHNOLOGY"


/* The max length of characteristic value. When the gatt client write or prepare write,
*  the data length must be less than GATTS_EXAMPLE_CHAR_VAL_LEN_MAX.
*/
#define GATTS_EXAMPLE_CHAR_VAL_LEN_MAX 500
#define LONG_CHAR_VAL_LEN           500
#define SHORT_CHAR_VAL_LEN          10
#define GATTS_NOTIFY_FIRST_PACKET_LEN_MAX 20

#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done       = 0;

uint16_t gatt_db_handle_table[HRS_IDX_NB];

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;


static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = true,
    .min_interval        = 0x20,
    .max_interval        = 0x40,
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x40,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST      = 0x180F;
static const uint16_t CHAR_1_SHORT_WR              = 0x2A19;
static const uint8_t  char1_name[]  = "Device battery";


static const uint16_t GATTS_SERVICE_HEART_RATE     = 0x180D;
static const uint16_t CHAR_HEART_RATE              = 0x2A37;
static const uint8_t char_heart_rate_name[]  = "Heart rate";


static const uint8_t GATTS_SERVICE_UUID_CONFIGURATION[] = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x7D,0x79,0x9E,0x00,0xB0,0x5C};
static const uint8_t CHAR_TREATMENT_TIME[]              = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x7D,0x79,0x4A,0xFF,0xAF,0x5C};
static const uint8_t char_treatment_name[]  = "Treatment time";
static const uint8_t CHAR_MAX_INTENSITY[]               = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x7D,0x79,0x98,0xFC,0xAF,0x5C};
static const uint8_t char_max_intensity_name[]  = "Max intensity";

static const uint8_t GATTS_SERVICE_UUID_CARRIER_WAVE[]  = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8B,0x79,0xD6,0x0C,0x70,0xC7};
static const uint8_t CHAR_CARRIER_FREQUENCY[]           = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8B,0x79,0x68,0x11,0x70,0xC7};
static const uint8_t char_carrier_frequency_name[]  = "Carrier frequency";
static const uint8_t CHAR_PPW[]                         = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8B,0x79,0xFC,0x13,0x70,0xC7};
static const uint8_t char_ppw_name[]  = "ppw";

static const uint8_t GATTS_SERVICE_UUID_EXTERNAL_WAVE[] = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8C,0x79,0xC0,0xF9,0xD2,0x8C};
static const uint8_t CHAR_EXTERNAL_FREQUENCY[]          = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8C,0x79,0xEA,0xFC,0xD2,0x8C};
static const uint8_t char_external_frequency_name[]  = "External Frequency";
static const uint8_t CHAR_PPWs[]                        = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8C,0x79,0xE6,0x00,0xD3,0x8C};
static const uint8_t char_ppws_name[]  = "ppws";

static const uint8_t GATTS_SERVICE_UUID_INTENSITY[]     = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8D,0x79,0x92,0x4A,0xCA,0xEE};
static const uint8_t CHAR_T1[]                          = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8D,0x79,0x58,0x52,0xCA,0xEE};
static const uint8_t char_t1_name[]  = "t1";
static const uint8_t CHAR_T2[]                          = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8D,0x79,0xDE,0x53,0xCA,0xEE};
static const uint8_t char_t2_name[]  = "t2";
static const uint8_t CHAR_T3[]                          = {0x02,0x00,0x12,0xAC,0x42,0x02,0xEB,0xA1,0xED,0x11,0x8D,0x79,0xB0,0x59,0xCA,0xEE};
static const uint8_t char_t3_name[]  = "t3";

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_user_description   = ESP_GATT_UUID_CHAR_DESCRIPTION;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read_write          = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_notify         = ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ;
//static const uint8_t char_prop_notify              = ESP_GATT_CHAR_PROP_BIT_NOTIFY;
//static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;

static const uint8_t char_ccc[2]   = {0x00, 0x00};
static const uint8_t char_value[] = {0};


/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID_TEST), (uint8_t *)&GATTS_SERVICE_UUID_TEST}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&CHAR_1_SHORT_WR, ESP_GATT_PERM_READ ,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char1_name), sizeof(char1_name), (uint8_t *)char1_name}},

      /* Characteristic Client Configuration Descriptor */
    [IDX_CHAR_CFG_A_2]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(char_ccc), (uint8_t *)char_ccc}},
};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_svc1_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_HEART_RATE), (uint8_t *)&GATTS_SERVICE_HEART_RATE}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&CHAR_HEART_RATE, ESP_GATT_PERM_READ,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_heart_rate_name), sizeof(char_heart_rate_name), (uint8_t *)char_heart_rate_name}},

    /* Characteristic Client Configuration Descriptor */
    [IDX_CHAR_CFG_A_2]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(char_ccc), (uint8_t *)char_ccc}},
};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_svc2_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
        sizeof(GATTS_SERVICE_UUID_CONFIGURATION), sizeof(GATTS_SERVICE_UUID_CONFIGURATION), (uint8_t *)&GATTS_SERVICE_UUID_CONFIGURATION}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_TREATMENT_TIME, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_treatment_name), sizeof(char_treatment_name), (uint8_t *)char_treatment_name}},

    /* Characteristic Declaration */
    [IDX_CHAR_B]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_MAX_INTENSITY, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_B]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_max_intensity_name), sizeof(char_max_intensity_name), (uint8_t *)char_max_intensity_name}},
 
};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_svc3_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
        sizeof(GATTS_SERVICE_UUID_CONFIGURATION), sizeof(GATTS_SERVICE_UUID_CARRIER_WAVE), (uint8_t *)&GATTS_SERVICE_UUID_CARRIER_WAVE}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_CARRIER_FREQUENCY, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_carrier_frequency_name), sizeof(char_carrier_frequency_name), (uint8_t *)char_carrier_frequency_name}},

    /* Characteristic Declaration */
    [IDX_CHAR_B]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_PPW, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_B]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_ppw_name), sizeof(char_ppw_name), (uint8_t *)char_ppw_name}},
};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_svc4_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
        sizeof(GATTS_SERVICE_UUID_EXTERNAL_WAVE), sizeof(GATTS_SERVICE_UUID_EXTERNAL_WAVE), (uint8_t *)&GATTS_SERVICE_UUID_EXTERNAL_WAVE}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_EXTERNAL_FREQUENCY, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_external_frequency_name), sizeof(char_external_frequency_name), (uint8_t *)char_external_frequency_name}},

    /* Characteristic Declaration */
    [IDX_CHAR_B]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_PPWs, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_B]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_ppws_name), sizeof(char_ppws_name), (uint8_t *)char_ppws_name}},
    
};


/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_svc5_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
        sizeof(GATTS_SERVICE_UUID_EXTERNAL_WAVE), sizeof(GATTS_SERVICE_UUID_INTENSITY), (uint8_t *)&GATTS_SERVICE_UUID_INTENSITY}},

    /* Characteristic Declaration */
    [IDX_CHAR_A]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_A] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_T1, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_A]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_t1_name), sizeof(char_t1_name), (uint8_t *)char_t1_name}},

    /* Characteristic Declaration */
    [IDX_CHAR_B]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_B] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_T2, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_B]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_t2_name), sizeof(char_t2_name), (uint8_t *)char_t2_name}},

     /* Characteristic Declaration */
    [IDX_CHAR_C]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      CHAR_DECLARATION_SIZE, CHAR_DECLARATION_SIZE, (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_C] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&CHAR_T3, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      SHORT_CHAR_VAL_LEN, sizeof(char_value), (uint8_t *)char_value}},

    /* Characteristic User Descriptor */
    [IDX_CHAR_CFG_C]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_user_description, ESP_GATT_PERM_READ,
      sizeof(char_t3_name), sizeof(char_t3_name), (uint8_t *)char_t3_name}},

};


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
    #ifdef CONFIG_SET_RAW_ADV_DATA
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #else
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
    #endif
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(EXAMPLE_TAG, "advertising start failed");
            }else{
                ESP_LOGI(EXAMPLE_TAG, "(0) ***** advertising start successfully ***** \n");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(EXAMPLE_TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(EXAMPLE_TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            EXAMPLE_DEBUG(EXAMPLE_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}

void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    EXAMPLE_DEBUG(EXAMPLE_TAG, "prepare write, handle = %d, value len = %d", param->write.handle, param->write.len);
    esp_gatt_status_t status = ESP_GATT_OK;
    if (prepare_write_env->prepare_buf == NULL) {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(EXAMPLE_TAG, "%s, Gatt_server prep no mem", __func__);
            status = ESP_GATT_NO_RESOURCES;
        }
    } else {
        if(param->write.offset > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_OFFSET;
        } else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) {
            status = ESP_GATT_INVALID_ATTR_LEN;
        }
    }
    /*send response when param->write.need_rsp is true */
    if (param->write.need_rsp){
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
        if (gatt_rsp != NULL){
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK){
               ESP_LOGE(EXAMPLE_TAG, "Send response error");
            }
            free(gatt_rsp);
        }else{
            ESP_LOGE(EXAMPLE_TAG, "%s, malloc failed", __func__);
        }
    }
    if (status != ESP_GATT_OK){
        return;
    }
    memcpy(prepare_write_env->prepare_buf + param->write.offset,
           param->write.value,
           param->write.len);
    prepare_write_env->prepare_len += param->write.len;

}
uint8_t long_write[16] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param){
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf){
        if(prepare_write_env->prepare_len == 256) {
            bool long_write_success = true;
            for(uint16_t i = 0; i < prepare_write_env->prepare_len; i ++) {
                if(prepare_write_env->prepare_buf[i] != long_write[i%16]) {
                    long_write_success = false;
                    break;
                }
            }
            if(long_write_success) {
                ESP_LOGI(EXAMPLE_TAG, "(4) ***** long write success ***** \n");
            }
        }
    }else{
        ESP_LOGI(EXAMPLE_TAG,"ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf) {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{

            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(EXAMPLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(EXAMPLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;

            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_DEVICE_BATTERY_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_svc1_db, gatts_if, HRS_IDX_NB, SVC_HEART_RATE_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_svc2_db, gatts_if, HRS_IDX_NB, SVC_DEVICE_CONFIG_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_svc3_db, gatts_if, HRS_IDX_NB, SVC_CARRIER_WAVE_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_svc4_db, gatts_if, HRS_IDX_NB, SVC_EXTERNAL_WAVE_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
            create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_svc5_db, gatts_if, HRS_IDX_NB, SVC_INTENSITY_MODULATION_ID);
            if (create_attr_ret){
                ESP_LOGE(EXAMPLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            //ESP_LOGE(EXAMPLE_TAG, "ESP_GATTS_READ_EVT, handle=0x%d, offset=%d", param->read.handle, param->read.offset);
            if(gatt_db_handle_table[IDX_CHAR_VAL_A] == param->read.handle) {
                ESP_LOGE(EXAMPLE_TAG, "(2) ***** read char1 ***** \n");
            }
       	    break;
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_EXAMPLE_CHAR_VAL_LEN_MAX.
                if(gatt_db_handle_table[IDX_CHAR_VAL_A] == param->write.handle && param->write.len == 2) {
                    uint8_t write_data[2] = {0x88, 0x99};
                    if(memcmp(write_data, param->write.value, param->write.len) == 0) {
                        ESP_LOGI(EXAMPLE_TAG, "(3)***** short write success ***** \n");
                    }
                }

                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* handle prepare write */
                example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT:
            // the length of gattc prepare write data must be less than GATTS_EXAMPLE_CHAR_VAL_LEN_MAX.
            ESP_LOGI(EXAMPLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT, Length=%d",  prepare_write_env.prepare_len);
            example_exec_write_event_env(&prepare_write_env, param);
            break;
        case ESP_GATTS_CONF_EVT:
            EXAMPLE_DEBUG(EXAMPLE_TAG, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            EXAMPLE_DEBUG(EXAMPLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(EXAMPLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(EXAMPLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(EXAMPLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
                ESP_LOGE(EXAMPLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
            }
            else {
                ESP_LOGI(EXAMPLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(gatt_db_handle_table, param->add_attr_tab.handles, sizeof(gatt_db_handle_table));
                esp_ble_gatts_start_service(gatt_db_handle_table[IDX_SVC]);
            }
            break;
        }
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gl_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(EXAMPLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gl_profile_tab[idx].gatts_if) {
                if (gl_profile_tab[idx].gatts_cb) {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void user_ble_start(void)
{
    esp_err_t ret;
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(EXAMPLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(EXAMPLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(EXAMPLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(EXAMPLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(EXAMPLE_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(EXAMPLE_TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(EXAMPLE_TAG, "gatts app register error, error code = %x", ret);
        return;
    }
}
