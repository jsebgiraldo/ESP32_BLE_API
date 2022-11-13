/**
 * Application entry point.
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
#include "user_bsp.h"
#include "user_timer.h"

#include "wifi_app.h"

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

	wifi_app_start();

	dac_modulation_wave_setup();
	external_wave_setup();

	hv_converter_init();

	deep_sleep_setup_timer();
	deep_sleep_timer_start();

	
	//***********************************************//
	
}

