/*
 * adc_cws_temp.h
 *
 * Created: 1/10/2017 1:58:59 PM
 *  Author: User
 */ 


#ifndef ADC_CWS_TEMP_H_
#define ADC_CWS_TEMP_H_


void configure_adc_temp(void);
float calculate_temperature(uint16_t raw_code);
void adc_temp_sensor(void);
uint16_t adc_start_read_result(void);



#endif /* ADC_CWS_TEMP_H_ */