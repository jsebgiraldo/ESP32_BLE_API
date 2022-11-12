/**
 * @brief 
 * 
 */

#include "esp_log.h"
#include "user_bsp.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"


static const char TAG[] = "[BSP]";

#define BSP_ENABLE
#ifdef BSP_ENABLE
// Tag used for ESP serial console messages
	#define BSP_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_PURPLE) __VA_ARGS__)
#else
	#define BSP_DEBUG(...)
#endif

static void bsp_step_up_converter(void)
{
    gpio_pad_select_gpio(STEP_UP_CONVERTER_PIN);
	gpio_set_direction(STEP_UP_CONVERTER_PIN, GPIO_MODE_OUTPUT);
    DISABLE_STEP_UP_CONVERTER();
}

static void bsp_user_button(void)
{
    const int ext_wakeup_pin = GPIO_NUM_0;
    const uint64_t ext_wakeup_pin_mask = 1ULL << ext_wakeup_pin;

    BSP_DEBUG("Enabling EXT1 wakeup on pin GPIO%d", ext_wakeup_pin);
    esp_sleep_enable_ext1_wakeup(ext_wakeup_pin_mask , ESP_EXT1_WAKEUP_ALL_LOW);

    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    rtc_gpio_pullup_en(ext_wakeup_pin);
    rtc_gpio_pulldown_dis(ext_wakeup_pin);
}

void user_bsp_setup(void)
{
    bsp_step_up_converter();
    bsp_user_button();
}