/*
 * CFile1.c
 *
 * Created: 1/10/2017 1:55:51 PM
 *  Author: Kumar 
 */ 


//ADC temperature file


	#include <asf.h>
	#include "adc_cws_temp.h"
    #include "device.h"
/* Structure for ADC module instance */
struct adc_module adc_instance;
	


  #define  LOW_TEMP_CONST_VALUE    3000
  #define  LOW_TEMP_DEGREES           4
  #define  HIGH_TEMP_CONST_VALUE   1700
  #define  HIGH_TEMP_DEGREES         33
  #define  TEMP_ALERT                80
  
  /* To store raw_result of ADC output */
  uint16_t raw_result;
 

float calculate_temperature(uint16_t raw_code)
	{
		
		float result_i;
		float diff_deg = -44.8275862;
		
		float raw_adc, low_diff;
		float result_1;
		
		raw_adc = raw_code - LOW_TEMP_CONST_VALUE;
		low_diff = LOW_TEMP_DEGREES * diff_deg;
		
		result_1 = raw_adc + low_diff;
		
		result_i = result_1 / diff_deg;
		

		
		return result_i;
		
}



void configure_adc_temp(void)
	{
		
		
		struct adc_config conf_adc;
		
		adc_get_config_defaults(&conf_adc);
		
		conf_adc.clock_source = GCLK_GENERATOR_0;
		conf_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV512;
		conf_adc.reference = ADC_REFERENCE_AREFB;
		conf_adc.positive_input = ADC_POSITIVE_INPUT_PIN0;
		conf_adc.negative_input = ADC_NEGATIVE_INPUT_IOGND;
		conf_adc.resolution = ADC_RESOLUTION_12BIT;
		//conf_adc.sample_length = ADC_TEMP_SAMPLE_LENGTH;
		
		
		
		
		
		adc_init(&adc_instance, ADC, &conf_adc);
		
		ADC->AVGCTRL.reg = ADC_AVGCTRL_ADJRES(2) | ADC_AVGCTRL_SAMPLENUM_4;
		
		adc_enable(&adc_instance);
}


uint16_t adc_start_read_result(void)
{
	uint16_t adc_result = 0;
	
	adc_start_conversion(&adc_instance);
//	while((adc_get_status(&adc_instance) & ADC_STATUS_RESULT_READY) != 1);
	
	adc_read(&adc_instance, &adc_result);
	
	return adc_result;
}

void adc_temp_sensor(void)
{
	struct device *pd = get_device_ctx();
	float temp;
	int temp_farh;
	
	//system_voltage_reference_enable(SYSTEM_VOLTAGE_REFERENCE_TEMPSENSE);
	
	configure_adc_temp();
	
	//load_calibration_data();
	
	raw_result = adc_start_read_result();
	
	temp = calculate_temperature(raw_result);
	
	temp_farh = (temp * 1.8) + 32;
	
	pd->hvac.temperature = temp_farh;

}