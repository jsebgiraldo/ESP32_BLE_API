
#include "user_external_wave.h"
#include "user_mcpwm.h"
#include "string.h"

static const char TAG[] = "[EX WAVE]";

#define EXWAVE_DEBUG_ENABLE
#ifdef EXWAVE_DEBUG_ENABLE
	#define EXWAVE_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_CYAN) __VA_ARGS__)
#else
	#define EXWAVE_DEBUG(...)
#endif

mcpwm_config_t wave_config;

static bool external_wave_callback(mcpwm_unit_t mcpwm, mcpwm_capture_channel_id_t cap_channel, const cap_event_data_t *edata,
                                        void *user_data)
{
    if(edata->cap_edge == MCPWM_NEG_EDGE)
    {
        pwm_carrier_wave_stop();
    }

    if(edata->cap_edge == MCPWM_POS_EDGE)
    {
        pwm_carrier_wave_start();
    }

    return false;
}

void external_wave_start(void) {
    if(wave_config.frequency <= 1000)
    {
        EXWAVE_DEBUG("%s",__func__);
        ESP_ERROR_CHECK(mcpwm_init(MCPWM_UNIT_1, MCPWM_TIMER_2, &wave_config));

        // bind output to GPIO
        ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM2B, GPIO_NUM_18));
    
        ESP_ERROR_CHECK(mcpwm_start(MCPWM_UNIT_1, MCPWM_TIMER_2));
    }
    

    
}

void external_wave_setup(void)
{

    mcpwm_capture_config_t config_capture = {
        .cap_edge = MCPWM_BOTH_EDGE,
        .cap_prescale = 1,
        .capture_cb = external_wave_callback,
    };
       
    ESP_ERROR_CHECK(mcpwm_gpio_init(MCPWM_UNIT_1, MCPWM_CAP_0, GPIO_NUM_19));
    ESP_ERROR_CHECK(mcpwm_capture_enable_channel(MCPWM_UNIT_1, MCPWM_SELECT_CAP0 ,&config_capture));
}

void external_wave_config(mcpwm_config_t *config)
{
    memcpy(&wave_config,config,sizeof(mcpwm_config_t));
    EXWAVE_DEBUG("External Frequency: %d , duty_a: %f, duty_b: %f",wave_config.frequency,wave_config.cmpr_a,wave_config.cmpr_b);	
}

void external_wave_stop(void)
{
    //EXWAVE_DEBUG("%s",__func__);

    ESP_ERROR_CHECK(mcpwm_stop(MCPWM_UNIT_1, MCPWM_TIMER_2));

    ESP_ERROR_CHECK(mcpwm_set_signal_low(MCPWM_UNIT_1,MCPWM_TIMER_2,MCPWM_GEN_A));

    gpio_pad_select_gpio(TIMER2_OUTPUT_GPIO);
	gpio_set_direction(TIMER2_OUTPUT_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(TIMER2_OUTPUT_GPIO,0);

}