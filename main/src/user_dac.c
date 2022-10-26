
#include "user_dac.h"

#include "user_mcpwm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

#include "esp_log.h"

#include "string.h"

static const char TAG[] = "[DAC]";

#define DAC_DEBUG_ENABLE
#ifdef DAC_DEBUG_ENABLE
	#define DAC_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_PURPLE) __VA_ARGS__)
#else
	#define DAC_DEBUG(...)
#endif

#define DAC_CHAN    DAC_CHANNEL_2


// Queue handle used to manipulate the main queue of events
static QueueHandle_t modulation_wave_queue_handle;
modulation_wave_config_t modulation_wave;

uint8_t dac_current_state;
int16_t output_dac;


// ************* TIMEOUT FUNCTIONS BEGIN *************
TimerHandle_t dac_tmr_t1;

void dac_start_t1(void)
{
	if(xTimerIsTimerActive(dac_tmr_t1))
	{
		xTimerStop(dac_tmr_t1,0);
	}

	if( xTimerStart(dac_tmr_t1, 2000/ portTICK_PERIOD_MS ) != pdPASS ) {
    	DAC_DEBUG("%s ERROR",__FUNCTION__);
		return;
    }
}

void dac_stop_t1(void)
{
	if(xTimerIsTimerActive(dac_tmr_t1))
	{
		xTimerStop(dac_tmr_t1,0);
	}
}

/******************************************************/


// ************* TIMEOUT FUNCTIONS BEGIN *************
TimerHandle_t dac_tmr_t2;

void dac_start_t2(void)
{
	if(xTimerIsTimerActive(dac_tmr_t2))
	{
		xTimerStop(dac_tmr_t2,0);
	}

	if( xTimerStart(dac_tmr_t2, 2000/ portTICK_PERIOD_MS ) != pdPASS ) {
    	DAC_DEBUG("%s ERROR",__FUNCTION__);
		return;
    }
}

void dac_stop_t2(void)
{
	if(xTimerIsTimerActive(dac_tmr_t2))
	{
		xTimerStop(dac_tmr_t2,0);
	}
}

// ************* TIMEOUT FUNCTIONS BEGIN *************
TimerHandle_t dac_tmr_t3;

void dac_start_t3(void)
{
	if(xTimerIsTimerActive(dac_tmr_t3))
	{
		xTimerStop(dac_tmr_t3,0);
	}

	if( xTimerStart(dac_tmr_t3, 2000/ portTICK_PERIOD_MS ) != pdPASS ) {
    	DAC_DEBUG("%s ERROR",__FUNCTION__);
		return;
    }
}

void dac_stop_t3(void)
{
	if(xTimerIsTimerActive(dac_tmr_t3))
	{
		xTimerStop(dac_tmr_t3,0);
	}
}

/******************************************************/

TimerHandle_t treatment_timer;

void timer_treatmnet_start(void)
{
	if(xTimerIsTimerActive(treatment_timer))
	{
		xTimerStop(treatment_timer,0);
	}

	if( xTimerStart(treatment_timer, 2000/ portTICK_PERIOD_MS ) != pdPASS ) {
    	DAC_DEBUG("%s ERROR",__FUNCTION__);
		return;
    }

    gpio_set_level(GPIO_NUM_23,1);
}

void timer_treatmnet_stop(void)
{
	if(xTimerIsTimerActive(treatment_timer))
	{
		xTimerStop(treatment_timer,0);
	}
}

void timer_treatmnet_change_period(uint8_t time)
{
	xTimerChangePeriod(treatment_timer,pdMS_TO_TICKS(time*60*1000),100);
}

void timer_treatment_callback( TimerHandle_t xTimer )
{
    dac_app_send_message(DAC_APP_MSG_STOP_OPERATION);
    pwm_carrier_wave_stop();
    dac_modulation_wave_stop();
    gpio_set_level(GPIO_NUM_23,0);
}

/******************************************************/

void dac_callback_timeout( TimerHandle_t xTimer )
{

  if(xTimer == dac_tmr_t1)
  {
      if(dac_current_state == DAC_APP_MSG_RISING_STATE)
        {
            output_dac += (modulation_wave.max_intensity/15);
            if(output_dac >= modulation_wave.max_intensity)
            {
                dac_app_send_message(DAC_APP_MSG_MAX_INTENSITY_STATE);
                dac_stop_t1();
                output_dac =  modulation_wave.max_intensity;
            }
            
            dac_output_voltage(DAC_CHAN, output_dac);
            return;
        }

        if(dac_current_state == DAC_APP_MSG_FALLING_STATE)
        {
            output_dac -= (modulation_wave.max_intensity/15);
            if(output_dac <= 0)
            {
                dac_app_send_message(DAC_APP_MSG_STAY_STATE);
                dac_stop_t1();
                output_dac = 0;
            }

            dac_output_voltage(DAC_CHAN, output_dac);
            return;
        }
  }

  if(xTimer == dac_tmr_t2)
  {
    dac_app_send_message(DAC_APP_MSG_FALLING_STATE);
  }

  if(xTimer == dac_tmr_t3)
  {
    dac_app_send_message(DAC_APP_MSG_RISING_STATE);
  }

}

// *****************************************************************

void dac_wave(void *pvParameters)
{
    
    DAC_DEBUG("dac wave task started!");

	// Create message queue
	modulation_wave_queue_handle = xQueueCreate(3, sizeof(dac_app_queue_message_t));
    dac_app_queue_message_t msg;

    dac_tmr_t1 = xTimerCreate("dac time t1", pdMS_TO_TICKS(1), pdTRUE, ( void * )1, &dac_callback_timeout);
    dac_tmr_t2 = xTimerCreate("dac time t2", pdMS_TO_TICKS(1), pdFALSE, ( void * )1, &dac_callback_timeout);
    dac_tmr_t3 = xTimerCreate("dac time t2", pdMS_TO_TICKS(1), pdFALSE, ( void * )1, &dac_callback_timeout);

    treatment_timer = xTimerCreate("treatment", pdMS_TO_TICKS(1), pdFALSE, ( void * )1, &timer_treatment_callback);

    for(;;)
    {
		if (xQueueReceive(modulation_wave_queue_handle, &msg, portMAX_DELAY))
		{
			switch (msg.msgID)
			{
                case DAC_APP_MSG_START_OPERATION:
                    DAC_DEBUG("DAC_APP_MSG_START_OPERATION!");
                    dac_current_state = DAC_APP_MSG_START_OPERATION;
                    output_dac = 0;
                    if(modulation_wave.T1/15 >= 1)
                    {
                        xTimerChangePeriod(dac_tmr_t1,pdMS_TO_TICKS(modulation_wave.T1/15),100);
                        dac_start_t1();
                        dac_app_send_message(DAC_APP_MSG_RISING_STATE);
                    }
                    

                    break;
                case DAC_APP_MSG_STOP_OPERATION:
                    DAC_DEBUG("DAC_APP_MSG_STOP_OPERATION!");
                    dac_current_state = DAC_APP_MSG_STOP_OPERATION;
                    dac_stop_t1();
                    dac_stop_t2();
                    dac_stop_t3();
                    dac_output_voltage(DAC_CHAN, 0);
                    gpio_set_level(GPIO_NUM_23,0);
                    break;

                case DAC_APP_MSG_RISING_STATE:
                    dac_current_state = DAC_APP_MSG_RISING_STATE;
                    output_dac = 0;
                    dac_start_t1();
                    break;

                case DAC_APP_MSG_FALLING_STATE:
                    dac_current_state = DAC_APP_MSG_FALLING_STATE;
                    output_dac = modulation_wave.max_intensity;
                    dac_start_t1();
                    break;

                case DAC_APP_MSG_MAX_INTENSITY_STATE:
                    dac_current_state = DAC_APP_MSG_MAX_INTENSITY_STATE;

                    if(modulation_wave.T2 > 0)
                    {
                        xTimerChangePeriod(dac_tmr_t2,pdMS_TO_TICKS(modulation_wave.T2),100);
                        dac_start_t2();
                    }   
                 
                    break;

                case DAC_APP_MSG_STAY_STATE:
                    dac_current_state = DAC_APP_MSG_STAY_STATE;

                    if(modulation_wave.T3 > 0)
                    {
                        xTimerChangePeriod(dac_tmr_t3,pdMS_TO_TICKS(modulation_wave.T3),100);
                        dac_start_t3();
                    }
                    
                    break;

                default:
                    break;
			}
		}
    }
    
    vTaskDelete(NULL);
}



BaseType_t dac_app_send_message(dac_app_message_e msgID)
{
	dac_app_queue_message_t msg;
	msg.msgID = msgID;
	return xQueueSend(modulation_wave_queue_handle, &msg, portMAX_DELAY);
}


void dac_modulation_wave_setup(void)
{
    dac_output_enable(DAC_CHAN);
	dac_output_voltage(DAC_CHAN, 0);

    xTaskCreate( dac_wave, "dac_wave", 2*1024, NULL, 5, NULL); // Task main queue
}

void dac_modulation_wave_configure(modulation_wave_config_t *configuration)
{
    memcpy(&modulation_wave,configuration,sizeof(modulation_wave_config_t));
}

void dac_modulation_wave_start(void)
{
    DAC_DEBUG("%s",__FUNCTION__);
    if(modulation_wave_queue_handle != NULL)
    {
        DAC_DEBUG("T1: %d , T2: %d, T3: %d",modulation_wave.T1,modulation_wave.T2,modulation_wave.T3);
        dac_app_send_message(DAC_APP_MSG_START_OPERATION);
    }
    
}

void dac_modulation_wave_stop(void)
{
    DAC_DEBUG("%s",__FUNCTION__);
    if(modulation_wave_queue_handle != NULL)
    {
        dac_app_send_message(DAC_APP_MSG_STOP_OPERATION);
    }
    
}
    

