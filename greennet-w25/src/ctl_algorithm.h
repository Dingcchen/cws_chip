
#ifndef CTL_ALGORITHM_H_INCLUDED
#define CTL_ALGORITHM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <compiler.h>
#include <board.h>
#include <conf_board.h>
#include <port.h>

/* HVAC GPIO mapping for Xplained kit Extension
	This can be utilize OLED1 Xplained kit that has LEDs and BUTTONs.
	Refer to Table 4.2.1 http://www.atmel.com/images/atmel-42077-oled1-xplained-pro_user-guide.pdf
 */
#if BOARD == SAMW25_XPLAINED_PRO
/* W25 Xplained (EXT1 Port) with OLED1 Xplained */
//OUTPUT PINS
#define FAN_OUTPUT_PIN_SET           PIN_PA10  //SET FAN
#define FAN_OUTPUT_PIN_RESET         PIN_PA11  //RESET FAN
#define COMPRESSOR_PIN_SET           PIN_PB03  //Set COMPRESSOR
#define COMPRESSOR_PIN_RESET         PIN_PB02  //RESet COMPRESSOR

//Input PINS
#define FAN_INPUT_PIN             PIN_PA20 /*GP3 from PIC12F509*/ /* OLED1 PIN9 BUTTON1 */
#define COMPRESSOR_INPUT_PIN      PIN_PA21 /*GP0 from PIC12F509*/ /* OLED1 PIN3 BUTTON2 */
#define HEATER_INPUT_PIN          PIN_PA03 /*GP1 from PIC12F509*/ /* OLED1 PIN4 BUTTON3 */
//#define ADC_POSITIVE_INPUT_PIN    PIN_PA02  /* Analog to Digital conversion Pin */

#elif BOARD == SAMD21_XPLAINED_PRO
/* D21 Xplained (EXT3 port) with OLED1 Xpalined */
#define FAN_OUTPUT_PIN            PIN_PA12 /*GP2 from PIC12F509*/ /* OLED1 PIN7 LED1    */
#define COMPRESSOR_OUTPUT_PIN     PIN_PA13 /*GP4 from PIC12F509*/ /* OLED1 PIN8 LED2    */
#define FAN_INPUT_PIN             PIN_PA28 /*GP3 from PIC12F509*/ /* OLED1 PIN9 BUTTON1 */
#define COMPRESSOR_INPUT_PIN      PIN_PA02 /*GP0 from PIC12F509*/ /* OLED1 PIN3 BUTTON2 */
#define HEATER_INPUT_PIN          PIN_PA03 /*GP1 from PIC12F509*/ /* OLED1 PIN4 BUTTON3 */

#endif


#define OUTPUT_ACTIVE 		true
#define OUTPUT_INACTIVE 	false


//#define FAN_OUPUT(VAL)             port_pin_set_output_level(FAN_OUTPUT_PIN, VAL)
//#define COMPRESSOR_OUTPUT(VAL)     port_pin_set_output_level(COMPRESSOR_OUTPUT_PIN, VAL)

//#define FAN_OUPUT_STATUS()         port_pin_get_output_level(FAN_OUTPUT_PIN)
//#define COMPRESSOR_OUTPUT_STATUS() port_pin_get_output_level(COMPRESSOR_OUTPUT_PIN)

//#define FAN_INPUT_STATUS()         port_pin_get_input_level(FAN_INPUT_PIN)
//#define COMPRESSOR_INPUT_STATUS()  port_pin_get_input_level(COMPRESSOR_INPUT_PIN)
//#define HEATER_INPUT_STATUS()      port_pin_get_input_level(HEATER_INPUT_PIN)


void ctl_port_init(void);
void Clean_vars(void);

void control_conditioner(void);
void set_fan(void);
void reset_fan(void);
void set_comp(void);
void reset_comp(void);
int control_conditioner1(void);
int control_conditioner2(void);
int control_conditioner3(void);
void Clean_vars_after_greennet(void);
static void Comp_man(void);

static int _Get_fan(void);
static int _is_comp_off(void);
static int _is_heater_off(void);
//static void _delay_btw_comp_n_fan(void);



#ifdef __cplusplus
}
#endif

#endif /* CTL_ALGORITHM_H_INCLUDED */


