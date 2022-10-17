
#include "user_mcpwm.h"


static const char TAG[] = "[MCPWM]";

#define MCPWM_DEBUG_ENABLE
#ifdef MCPWM_DEBUG_ENABLE
	#define MCPWM_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_BROWN) __VA_ARGS__)
#else
	#define MCPWM_DEBUG(...)
#endif


void pwm_carrier_setup(void) {

    MCPWM_DEBUG("%s",__func__);

    // init MCPWM: 10% duty cycle on all three timers in MCPWM unit 0 (currently not synchronous)
    mcpwm_config_t pwm_config = {
        .frequency = 10*1000,
        .cmpr_a = 50,
        .cmpr_b = 50,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };
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

    pwm_carrier_stop();

    
}


void pwm_carrier_start(void)
{
    MCPWM_DEBUG("%s",__func__);

    ESP_ERROR_CHECK(mcpwm_start(TARGET_MCPWM_UNIT, MCPWM_TIMER_0));
    ESP_ERROR_CHECK(mcpwm_start(TARGET_MCPWM_UNIT, MCPWM_TIMER_1));
}

void pwm_carrier_stop(void)
{
    MCPWM_DEBUG("%s",__func__);

    ESP_ERROR_CHECK(mcpwm_stop(TARGET_MCPWM_UNIT, MCPWM_TIMER_0));
    ESP_ERROR_CHECK(mcpwm_stop(TARGET_MCPWM_UNIT, MCPWM_TIMER_1));
}