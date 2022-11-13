
#include "user_hvconverter.h"

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <driver/dac.h>

static const char TAG[] = "[HV CONV]";

#define HV_CONVERTER_ENABLE
#ifdef HV_CONVERTER_ENABLE
	#define HV_DEBUG(...) ESP_LOGI(TAG,LOG_COLOR(LOG_COLOR_BLUE) __VA_ARGS__)
#else
	#define HV_DEBUG(...)
#endif

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

#define DAC_CHAN_1  DAC_CHANNEL_1

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_0;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;
static const adc_atten_t atten = ADC_ATTEN_DB_0;
static const adc_unit_t unit = ADC_UNIT_1;

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        HV_DEBUG("Characterized using Two Point Value");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        HV_DEBUG("Characterized using eFuse Vref");
    } else {
        HV_DEBUG("Characterized using Default Vref");
    }
}

static void check_efuse(void)
{

    //Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        HV_DEBUG("eFuse Two Point: Supported");
    } else {
        HV_DEBUG("eFuse Two Point: NOT supported");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        HV_DEBUG("eFuse Vref: Supported");
    } else {
        HV_DEBUG("eFuse Vref: NOT supported");
    }
}

void adc(void *pvParameters)
{
	//Check if Two Point or Vref are burned into eFuse
    check_efuse();

    dac_output_enable(DAC_CHAN_1);
	dac_output_voltage(DAC_CHAN_1, 0);

	adc1_config_width(width);
	adc1_config_channel_atten(channel, atten);
	adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
	print_char_val_type(val_type);
	for(;;)
	{
    	uint32_t adc_reading = 0;
        //Multisampling
        for (int i = 0; i < NO_OF_SAMPLES; i++) {
            if (unit == ADC_UNIT_1) {
                adc_reading += adc1_get_raw((adc1_channel_t)channel);
            } 
        }
        adc_reading /= NO_OF_SAMPLES;
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
     
        if(voltage >= 1000) voltage = 1000;
        uint8_t dac_output = voltage * 0.255;

        dac_output_voltage(DAC_CHAN_1, dac_output);

        vTaskDelay(pdMS_TO_TICKS(100));

	}
	vTaskDelete(NULL);
}

void hv_converter_init(void)
{
    xTaskCreate( adc, "adc", 2*1024, NULL, 5, NULL); // Task main queue
}

void hv_convertert_output_off(void)
{
    dac_output_voltage(DAC_CHAN_1, 0);
}
