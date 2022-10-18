/**
 * Application entry point.
 */

#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "user_mcpwm.h"
#include "user_dac.h"



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

void app_main(void)
{

	MAIN_DEBUG("HEAP MEMORY: %d",esp_get_free_heap_size());
	nvs_flash_setup(); 

	//************** Only debug *******************//
	mcpwm_config_t pwm_config = {
		.frequency = 10*1000,
		.cmpr_a = 50,
		.cmpr_b = 50,
		.counter_mode = MCPWM_UP_COUNTER,
		.duty_mode = MCPWM_DUTY_MODE_0,
	};

	pwm_carrier_wave_configure(&pwm_config);
	pwm_carrier_wave_start();


	modulation_wave_config_t wave_config = 
	{
		.T1 = 60,
		.T2 = 150,
		.T3 = 200,
	};
	dac_modulation_wave_configure(&wave_config);
	dac_modulation_wave_setup();
	dac_modulation_wave_start();
	//***********************************************//
	
}

