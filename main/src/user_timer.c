
#include "user_timer.h"
#include "user_hvconverter.h"
#include "driver/gpio.h"
#include <driver/dac.h>
#include <driver/rtc_io.h>
#include "soc/rtc.h"

#include "user_bsp.h"

static const char TAG[] = "[SLEEP]";

#define SLEEP_ENABLE
#ifdef SLEEP_ENABLE
	#define SLEEP_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_RED) __VA_ARGS__)
#else
	#define SLEEP_DEBUG(...)
#endif

TimerHandle_t deep_sleep_timer;

static void deep_sleep_timer_callback( TimerHandle_t xTimer )
{
	SLEEP_DEBUG("Going to sleep");
	dac_output_disable(DAC_CHANNEL_1);
	rtc_gpio_isolate(GPIO_NUM_25);
	rtc_gpio_isolate(STAND_BY_CHARGER);
	rtc_gpio_isolate(CHARGING_SIGNAL);
	dac_output_disable(DAC_CHANNEL_2);
    esp_deep_sleep_start();
}

void deep_sleep_timer_start(void)
{
	if(xTimerIsTimerActive(deep_sleep_timer))
	{
		xTimerStop(deep_sleep_timer,0);
	}

	if( xTimerStart(deep_sleep_timer, 2000/ portTICK_PERIOD_MS ) != pdPASS ) {
		return;
    }
}

void deep_sleep_timer_stop(void)
{
	if(xTimerIsTimerActive(deep_sleep_timer))
	{
		xTimerStop(deep_sleep_timer,0);
	}
}

void deep_sleep_setup_timer(void)
{
    deep_sleep_timer = xTimerCreate("Sleep timer", pdMS_TO_TICKS(DEEP_SLEEP_TIMEOUT), pdTRUE, ( void * )1, &deep_sleep_timer_callback);
}