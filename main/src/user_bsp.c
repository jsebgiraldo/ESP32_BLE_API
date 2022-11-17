/**
 * @brief 
 * 
 */

#include "esp_log.h"
#include "user_bsp.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"


static const char TAG[] = "[BSP]";

#define BSP_ENABLE
#ifdef BSP_ENABLE
// Tag used for ESP serial console messages
	#define BSP_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_PURPLE) __VA_ARGS__)
#else
	#define BSP_DEBUG(...)
#endif

// Queue handle
static xQueueHandle gpio_evt_queue = NULL;

/**
 * ISR handler for the user button
 * @param arg parameter which can be passed to the ISR handler.
 */
void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;

	gpio_intr_disable(gpio_num);

    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

/**
 * Wifi reset button task reacts to a BOOT button event by sending a message
 * to the Wifi application to disconnect from Wifi and clear the saved credentials.
 * @param pvParam parameter which can be passed to the task.
 */
void irs_task(void *pvParam)
{
	uint32_t io_num;
	for (;;)
	{
		if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {

			BSP_DEBUG("GPIO[%d] intr, val: %d", io_num, gpio_get_level(io_num));
			gpio_intr_enable(io_num);
			switch (io_num)
			{
			case STAND_BY_CHARGER:
                if(gpio_get_level(io_num))
				{
					LED_STAND_BY_ON();
				}
				else
				{
					LED_STAND_BY_OFF();
				}
				break;

			case CHARGING_SIGNAL:
				
				if(gpio_get_level(io_num))
				{
					LED_CHARGING_ON();
				}
				else
				{
					LED_CHARGING_OFF();
				}

				break;
			
			default:
				break;
			}
		}
	}
}

static inline void bsp_gpio_setup_interrupts(void){
	
	//create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));

	// Enable interrupt on the negative edge
	gpio_set_intr_type(STAND_BY_CHARGER, GPIO_INTR_ANYEDGE);
	gpio_set_intr_type(CHARGING_SIGNAL, GPIO_INTR_ANYEDGE);

	// Create the user button task
	xTaskCreatePinnedToCore(&irs_task, "irs_task", 3072, NULL, 6, NULL, 0);

	// Install gpio isr service
	gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

	// Attach the interrupt service routine
	gpio_isr_handler_add(STAND_BY_CHARGER, gpio_isr_handler, (void*)STAND_BY_CHARGER);

	// Attach the interrupt service routine
	gpio_isr_handler_add(CHARGING_SIGNAL, gpio_isr_handler, (void*)CHARGING_SIGNAL);
}

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

static void bsp_leds_gpio_init(void)
{
    gpio_pad_select_gpio(LED_STAND_BY);
	gpio_set_direction(LED_STAND_BY, GPIO_MODE_OUTPUT);

    gpio_pad_select_gpio(LED_CHARGING);
	gpio_set_direction(LED_CHARGING, GPIO_MODE_OUTPUT);

    LED_STAND_BY_OFF();
    LED_CHARGING_OFF();
}

void user_bsp_setup(void)
{
    bsp_step_up_converter();
    bsp_user_button();
    bsp_leds_gpio_init();
    bsp_gpio_setup_interrupts();
}