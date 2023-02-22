
#include "user_carrier_wave.h"
#include "string.h"

static const char TAG[] = "[CARRIER]";

#define CARRIER_DEBUG_ENABLE
#ifdef CARRIER_DEBUG_ENABLE
	#define CARRIER_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_BROWN) __VA_ARGS__)
#else
	#define CARRIER_DEBUG(...)
#endif

mcpwm_config_t pwm_config;

void pwm_carrier_wave_start(void) {

    ESP_ERROR_CHECK(mcpwm_gpio_init(TARGET_MCPWM_UNIT, MCPWM0A, TIMER0_OUTPUT_GPIO));
    ESP_ERROR_CHECK(mcpwm_gpio_init(TARGET_MCPWM_UNIT, MCPWM1A, TIMER1_OUTPUT_GPIO));

    ESP_ERROR_CHECK(mcpwm_start(TARGET_MCPWM_UNIT, MCPWM_TIMER_0));
    ESP_ERROR_CHECK(mcpwm_start(TARGET_MCPWM_UNIT, MCPWM_TIMER_1));
}

void pwm_carrier_wave_configure(mcpwm_config_t *config)
{
    memcpy(&pwm_config,config,sizeof(mcpwm_config_t));
    CARRIER_DEBUG("Carrier Frequency: %d , duty_a: %f, duty_b: %f",pwm_config.frequency,pwm_config.cmpr_a,pwm_config.cmpr_b);
   
    if(pwm_config.cmpr_a == 0 || pwm_config.cmpr_b == 0)
    {
        return;
    }
   
    if(pwm_config.frequency <= 1*1000*1000)
    {
        ESP_ERROR_CHECK(mcpwm_init(TARGET_MCPWM_UNIT, MCPWM_TIMER_0, &pwm_config));
        ESP_ERROR_CHECK(mcpwm_init(TARGET_MCPWM_UNIT, MCPWM_TIMER_1, &pwm_config));

        mcpwm_sync_config_t sync_conf = {
            .sync_sig = MCPWM_SELECT_TIMER0_SYNC,
            .timer_val = 0,
            .count_direction = MCPWM_TIMER_DIRECTION_UP,
        };

        ESP_ERROR_CHECK(mcpwm_sync_configure(TARGET_MCPWM_UNIT, MCPWM_TIMER_0, &sync_conf));
        sync_conf.timer_val = 500;
        ESP_ERROR_CHECK(mcpwm_sync_configure(TARGET_MCPWM_UNIT, MCPWM_TIMER_1, &sync_conf));
        ESP_ERROR_CHECK(mcpwm_set_timer_sync_output(TARGET_MCPWM_UNIT, MCPWM_TIMER_0, MCPWM_SWSYNC_SOURCE_TEZ));


        // bind output to GPIO
        ESP_ERROR_CHECK(mcpwm_gpio_init(TARGET_MCPWM_UNIT, MCPWM0A, TIMER0_OUTPUT_GPIO));
        ESP_ERROR_CHECK(mcpwm_gpio_init(TARGET_MCPWM_UNIT, MCPWM1A, TIMER1_OUTPUT_GPIO));
    }

}

void pwm_carrier_wave_stop(void)
{
    
    //ESP_ERROR_CHECK(mcpwm_stop(TARGET_MCPWM_UNIT, MCPWM_TIMER_0));
    //ESP_ERROR_CHECK(mcpwm_stop(TARGET_MCPWM_UNIT, MCPWM_TIMER_1));

    //ESP_ERROR_CHECK(mcpwm_set_signal_low(TARGET_MCPWM_UNIT,MCPWM_TIMER_0,MCPWM_GEN_A));
    //ESP_ERROR_CHECK(mcpwm_set_signal_low(TARGET_MCPWM_UNIT,MCPWM_TIMER_1,MCPWM_GEN_A));
    
	gpio_pad_select_gpio(TIMER0_OUTPUT_GPIO);
	gpio_set_direction(TIMER0_OUTPUT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TIMER0_OUTPUT_GPIO,0);

    gpio_pad_select_gpio(TIMER1_OUTPUT_GPIO);
	gpio_set_direction(TIMER1_OUTPUT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TIMER1_OUTPUT_GPIO,0);
    

}