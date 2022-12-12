/**
 * Application entry point.
 * 
 *      Author: Juan Sebastian Giraldo Duque
 *      gitlab: https://github.com/jsebgiraldo
 */

#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "user_carrier_wave.h"
#include "user_modulation_wave.h"
#include "user_external_wave.h"
#include "user_hvconverter.h"
#include "user_battery_level.h"
#include "user_bsp.h"
#include "user_timer.h"
#include "user_max30102.h"

#include "user_ble.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

static const char TAG[] = "[MAIN]";

#define MAIN_DEBUG_ENABLE
#ifdef MAIN_DEBUG_ENABLE
	#define MAIN_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_BROWN) __VA_ARGS__)
#else
	#define MAIN_DEBUG(...)
#endif


void nvs_flash_setup(void)
{
	 // Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_FRQ 100000
#define I2C_PORT I2C_NUM_0

max30102_config_t max30102 = {};

esp_err_t i2c_master_init(i2c_port_t i2c_port){
    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_SDA;
    conf.scl_io_num = I2C_SCL;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_FRQ;
    i2c_param_config(i2c_port, &conf);
    return i2c_driver_install(i2c_port, I2C_MODE_MASTER, 0, 0, 0);
}

void get_bpm(void* param) {
    printf("MAX30102\n");

     //Init I2C_NUM_0
    ESP_ERROR_CHECK(i2c_master_init(I2C_PORT));
    //Init sensor at I2C_NUM_0
    ESP_ERROR_CHECK(max30102_init( &max30102, I2C_PORT,
                   MAX30102_DEFAULT_OPERATING_MODE,
                   MAX30102_DEFAULT_SAMPLING_RATE,
                   MAX30102_DEFAULT_LED_PULSE_WIDTH,
                   MAX30102_DEFAULT_IR_LED_CURRENT,
                   MAX30102_DEFAULT_START_RED_LED_CURRENT,
                   MAX30102_DEFAULT_MEAN_FILTER_SIZE,
                   MAX30102_DEFAULT_PULSE_BPM_SAMPLE_SIZE,
                   MAX30102_DEFAULT_ADC_RANGE, 
                   MAX30102_DEFAULT_SAMPLE_AVERAGING,
                   MAX30102_DEFAULT_ROLL_OVER,
                   MAX30102_DEFAULT_ALMOST_FULL,
                   false ));

    max30102_data_t result = {};
    /*ESP_ERROR_CHECK(max30102_print_registers(&max30102));*/
    while(true) {
        //Update sensor, saving to "result"
        ESP_ERROR_CHECK(max30102_update(&max30102, &result));
        if(result.pulse_detected) {
            printf("BEAT\n");
            printf("BPM: %f | SpO2: %f%%\n", result.heart_bpm, result.spO2);
        }
        //Update rate: 100Hz
        vTaskDelay(10/portTICK_PERIOD_MS);
    }
}

void app_main(void)
{

	/* Determine wake up reason */
	const char* wakeup_reason;
	esp_sleep_wakeup_cause_t res;
	res = esp_sleep_get_wakeup_cause();
	switch (res) {
		case ESP_SLEEP_WAKEUP_TIMER:
			wakeup_reason = "timer";
			break;
		case ESP_SLEEP_WAKEUP_GPIO:
			wakeup_reason = "pin";
			break;
		case ESP_SLEEP_WAKEUP_EXT1:
			wakeup_reason = "Wake up from GPIO";
			break;
		default:
			wakeup_reason = "other";
			break;
	}

	MAIN_DEBUG("Returned from deep sleep, reason: %s [No: %x]",wakeup_reason, res);
	MAIN_DEBUG("HEAP MEMORY: %d",esp_get_free_heap_size());

	nvs_flash_setup(); 

	user_bsp_setup();

	dac_modulation_wave_setup();
	external_wave_setup();

	//hv_converter_init();
	//battery_level_init();
 
    //Start test task
    //xTaskCreate(get_bpm, "Get BPM", 8192, NULL, 1, NULL);

    user_ble_start();

	//deep_sleep_setup_timer();
	//deep_sleep_timer_start();
	//***********************************************//
	
}

