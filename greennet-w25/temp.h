
/* Author: Kumar 
Temperature sensor */


#ifndef ADC_TEMP_H_INCLUDED
#define ADC_TEMP_H_INCLUDED

/* TEMP_LOG Base Address*/
#define TEMP_LOG_READ_NVM1					(*(RwReg*)NVMCTRL_TEMP_LOG)
/* TEMP_LOG Next Address*/
#define TEMP_LOG_READ_NVM2					(*(RwReg*)NVMCTRL_TEMP_LOG + 4)
#define ADC_TEMP_SAMPLE_LENGTH				4
#define INT1V_VALUE_FLOAT					1.0
#define INT1V_DIVIDER_1000					1000.0
#define ADC_12BIT_FULL_SCALE_VALUE_FLOAT	4095.0




/* Function Prototypes for the ADC Temp Sensor Operation */
void configure_adc_temp(void);
float convert_dec_to_frac(uint8_t val);
void load_calibration_data(void);
float calculate_temperature(uint16_t raw_code);
static void adc_temp_sensor(void);

#endif /* ADC_TEMP_H_INCLUDED */
