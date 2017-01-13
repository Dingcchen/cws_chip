#if 1
/************************************************************************/
/*                                                                      */
/************************************************************************/
#include "asf.h"
#include "ctl_algorithm.h"

/************************************************************************/
/*                                                                      */
/************************************************************************/

#define _on		1
#define _off	0

/************************************************************************/
/* GPIO wrapper                                                         */
/************************************************************************/
static void hvac_delay_ms(unsigned long msec)
{
	delay_ms(msec);
// 	unsigned long t0 = get_time_ms();
// 	while (get_time_ms() - t0 > msec);
// 		asm(_nop);
}

static int turn_fan(int on)
{
	//GP2 = on ? 1 : 0;
	port_pin_set_output_level(FAN_OUTPUT_PIN_RESET, on);
	
	return 0;
}

static int turn_comp(int on)
{
	//GP4 = on ? 1 : 0;
	port_pin_set_output_level(COMPRESSOR_PIN_RESET, on);

	return 0;
}

static int _get_fan(void)
{
	//return GP3;
	return port_pin_get_input_level(FAN_INPUT_PIN);
}

static int _get_comp(void)
{
	//return GP0;
	return port_pin_get_input_level(COMPRESSOR_INPUT_PIN);
}

static int _get_heater(void)
{
	//return GP1;
	return port_pin_get_input_level(HEATER_INPUT_PIN);
}

#define get_fan			(_get_fan())
#define is_comp_off		(_get_comp())
#define is_heater_off	(_get_heater())

#define __delay_ms(x) hvac_delay_ms(x)
/************************************************************************/
/*                                                                      */
/************************************************************************/
#if 0 // ignore PIC system dependent.
/*
 * Original codes seen in Algorithm.docx, received from James.
 *
 * These are for PIC12F509 that is not portable;
 * When I review the code;
 *    - timing is tweaked using delay (bad portability)
 *    - delay are made using loops based on CPU clock (bad portability)
 *    - Ports are just value of signal level, not specific meang (not good for human to understand or manageable)
 *
 * Hence, better to refactor with human readable, manageable codes. 
 * See a renamed version in hvac.sam.c
 *
 * -- austin
 */ 
#include "pic12f509.h"
#include <xc.h>
#include <pic.h>
#include <htc.h>
#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */


/***** CONFIGURATION ****/

volatile unsigned char control TRIS @ 0x06;

#pragma config WDT=OFF
#pragma config CP=ON
#pragma config MCLRE=OFF
#pragma config OSC=IntRC

#endif	// ignore PIC system dependent.

/***** VARIABLES DECLARATION ****/

unsigned int tick_for_cooling_10sec;   		// Main Counter for the cooling
unsigned int tick_to_comp_off_1sec;  		// Main Counter to turns compressor OFF
unsigned int tick_heater_aux_1sec; 	// Auxiliar counter for the heater
unsigned int tick_heater_10sec;  		// Main Counter fro the heater
unsigned int Aux;   		// Variable used for the delays
unsigned int Aux_1;		 // Variable used for the delays
unsigned int tick_for_comp_control_10sec;     		// Variable used to keep time of compressor ON and OFF
unsigned int control_stage;     		// Variable used to find out is compressor is ON or OFF
unsigned int comp_check_1sec;     		// Auxiar used varible to increase to  b variable
unsigned int tick_comp_off_20sec;   		// Variable used to keep the Compressor OFF time
unsigned int extended_time;    		// Variable used to keep the extended time
unsigned int t1_comp_ontime;    		// Variable used to keep compressor's time ON
unsigned int t2_comp_off_20sec;    		// Variable used to keep compressor's time OFF

static void init(void)
{
	// 		XX01011	Set get_comp,get_heater and GP3 to input, GP2 and GP4 to output.
	// 	Addreaa 6 == W. Copy contents W to GPIO direction port.
	
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_INPUT;
	
	port_pin_set_config(FAN_INPUT_PIN, &pin_conf);
	port_pin_set_config(COMPRESSOR_INPUT_PIN, &pin_conf);
	port_pin_set_config(HEATER_INPUT_PIN, &pin_conf);

	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	
	port_pin_set_config(FAN_OUTPUT_PIN_SET, &pin_conf);
	port_pin_set_config(FAN_OUTPUT_PIN_RESET, &pin_conf);
	port_pin_set_config(COMPRESSOR_PIN_SET, &pin_conf);
	port_pin_set_config(COMPRESSOR_PIN_RESET, &pin_conf);

}

static void clean_vars(void)
{
	/****   Set variables to 0    *****/
	tick_for_cooling_10sec = 0;
	tick_to_comp_off_1sec = 0;
	tick_heater_10sec = 0;
	tick_heater_aux_1sec = 0;
	comp_check_1sec = 0;
	tick_comp_off_20sec=0;
	tick_for_comp_control_10sec=0;
}

static void Fan_Man(void)
{
	/****   FAN is ON or AUTO    *****/
	if (get_fan){
		turn_fan(0);
	}
	else {
		turn_fan(1);
	}
}


static void hvac_main(void)
{
	init();    		 // Enable weak pull-up ports. Timer internal clock.
	turn_fan(0);    // Turn Fan OFF
	turn_comp(0);    // Turn Compressor  OFF
	clean_vars(); // Clean Variables
	__delay_ms(2000); // Wait for 2 Seconds before turning the FAN ON

	/***** Main Program Starts Here ****/

	while (1) // Endless LOOP
	{
		/****   FAN ON if swith is ON or Heat and Cool are ON    *****/
		
		if (!get_fan || (!is_comp_off && !is_heater_off)) {
			turn_fan(1); 	// Turn Fan ON
		}
		
		/****   FAN OFF if swith is AUTO or Heat and Cool are OFF    *****/
		
		if (get_fan && (is_comp_off && is_heater_off)) {
			turn_fan(0);	// Turn Fan ON		(austin: original comment seems wrong, should fan OFF)
		}
		
		/****  Intialize variables    *****/
		
		tick_for_comp_control_10sec = 0;
		control_stage = 0;
		extended_time = 0;
		
		/****   Cooling condition ON GP0 is Low    *****/
		
		if (!is_comp_off) {
			
			/****   Cooling condition OFF GP0 is High   *****/
			
			Clean_vars(); 		// Clean Variables
			turn_fan(1);		// Turn Fan ON
			turn_comp(1);     	// Turn Compressor ON

			if (is_comp_off) {
				__delay_ms(2000);
				control_stage = 4;   		//Avoid going inside of the while loop below
				turn_fan(0);	// Turn fan OFF
				turn_comp(0);    // Turn Compressor OFF
			}

			while (((is_heater_off && !is_comp_off) || (is_heater_off && is_comp_off) || (!is_heater_off && !is_comp_off)) && control_stage <= 3) {

				__delay_ms(1000);
				comp_check_1sec++;
				if (comp_check_1sec > 9){
					comp_check_1sec = 0;        // Reset variable every 10 Seconds
					tick_for_comp_control_10sec++;        // Increment C every 10 seconds
					tick_for_cooling_10sec++;      // Increment Timer for compressor every 10 seconds
				}
				tick_heater_10sec = 0;    // Reset the Counter for the Heat;
				if (tick_for_cooling_10sec > 90) {
					tick_for_cooling_10sec = 90;   		// 15 minutes counter
					tick_to_comp_off_1sec++;     		// For Compressor if it is ON more than 30 minutes
					if (tick_to_comp_off_1sec > 95) {
						tick_to_comp_off_1sec = 95; 	// 30:50  minutes:Seconds counter
					}
				}
				if (tick_for_comp_control_10sec > 48) 		{  // variable "c"no greater than 8 minutes
					tick_for_comp_control_10sec = 48;
				}
				if (tick_for_comp_control_10sec == 1 && comp_check_1sec == 0) {  	//  increase b when c is 1
					control_stage++;
					Fan_Man();        		// Check FAN input  GP3
				}
				
				/****Store t1=c when the compressor in ON is on the first time*****/
				
				if (control_stage == 1 && !is_comp_off){
					t1_comp_ontime = tick_for_comp_control_10sec;
					Fan_Man();        // Check FAN input  GP3
				}
				
				/****If get_comp=0 reset variables and turn compressor OFF*****/
				
				if (control_stage == 1 && is_comp_off){
					tick_for_comp_control_10sec = 0;
					tick_for_cooling_10sec = 0;
					tick_to_comp_off_1sec=0;
					turn_comp(0);
					Fan_Man();       // Check FAN input  GP3
				}
				
				/**Calculate how long the  compressor is OFF and store it in t2***/
				/**  If compressor is OFF for more the 30 minutes  then  t2=0  ***/
				
				if (control_stage == 2 && is_comp_off) {
					if(tick_for_cooling_10sec > 89) {					// austin: T2 reset in 15-min (90 x 10sec), above comment is incorrect.
						t2_comp_off_20sec = 0;
					} else {
						if((tick_for_comp_control_10sec % 2)==0) {		// austin: T2 updated in every 20-sec, max 8-min (24 x 20-sec)
							tick_comp_off_20sec++;
							if(tick_comp_off_20sec > 24){
								tick_comp_off_20sec = 24;
							}
							t2_comp_off_20sec = tick_comp_off_20sec;  		// T2 = 7% of the time OFF
						}
					}

					tick_for_cooling_10sec = 0; // Reset counter
					tick_to_comp_off_1sec = 0;
					Fan_Man();        // Check FAN input  GP3
				}
				if (control_stage == 2 && !is_comp_off){ 	//a == 0)
					tick_for_comp_control_10sec = 0;
					turn_comp(1);
					turn_fan(1);
				}
				/** Turn compressor OFF for 3 minutes every 30 Minutes ***/
				if (tick_to_comp_off_1sec > 90 && !is_comp_off) {
					turn_comp(0);  	 // Compressor OFF

					/** 3 minutes delay  **/
					__delay_ms(3 * 60 * 1000);

					if (!is_comp_off){
						turn_comp(1);		// Compressor ON
						turn_fan(1);		// turn fan ON
					} else {
						turn_comp(0);		// Compressor OFF
						Fan_Man();			// Check FAN input  GP3
					}
					control_stage=4;

				}
				
				/**   Extended time   ***/
				
				if (is_comp_off && control_stage == 3) {
					turn_comp(0); 		// Compressor OFF
					turn_fan(1);		// Turn  FAN ON
					extended_time = t1_comp_ontime + t2_comp_off_20sec + tick_for_comp_control_10sec; //xx=t1*0.17+ t2*0.07 + c*.0.17
					for (Aux_1 = 0; Aux_1 < extended_time; Aux_1++) {
						for (Aux = 0; Aux < 15; Aux++) {
							__delay_ms(100);
						}
					}
					t1_comp_ontime = tick_for_comp_control_10sec;
					control_stage = 1;
					Fan_Man();        // Check FAN input  GP3
					tick_for_cooling_10sec = 0; 			// Reset counter
					tick_to_comp_off_1sec=0;
					tick_for_comp_control_10sec=0;
					tick_comp_off_20sec=0;
				}

			}
		}
		/** Evaluate GP0 to find out if the Heat is Activated **/
		
		if (!is_heater_off && is_comp_off) {
			t1_comp_ontime = tick_for_comp_control_10sec;
			control_stage = 0;
			tick_for_cooling_10sec = 0;
			tick_to_comp_off_1sec=0;
			tick_for_comp_control_10sec=0;
			extended_time=0;
			t2_comp_off_20sec=0;

			__delay_ms(1000); // 1 Second delay
			
			tick_heater_aux_1sec++; // Increment auxiliar Heater Counter
			if (tick_heater_aux_1sec > 9) {
				tick_heater_aux_1sec = 0; // Reset the auxiliar Counter for the Heater;
				tick_heater_10sec++; // Count Seconds the AC stayed ON  tick_heater++;
			}
			if (tick_heater_10sec > 95)  tick_heater_10sec = 95; 	// Reset T_H to avoid OverFlow to about 15 minutes
			if (tick_heater_10sec > 18)  turn_fan(1); 		// Turn Fan ON  after 3 minutes only in Heat
		}

		/***** Main delay for Heat Mode ****/

		if (is_heater_off) 	// Evaluate GP0 if it is High then Heater is activated
		{
			if (tick_heater_10sec > 18)   	// Apply Delay  if  Heat is ON for more the 3 Minutes
			//Only in GA.  Apply Delay if the Heater is ON for more than 240
			//Sec. (tick_cooler>24)
			{
				turn_fan(1);	// Turn Fan ON
				for (Aux_1 = 3; Aux_1 <= tick_heater_10sec; Aux_1++) {
					for (Aux = 0; Aux < 20; Aux++) __delay_ms(105);
				}
			}
			turn_fan(0);	//GP2 = 0; 		// Turn  Fan OFF
			tick_heater_10sec = 0; 		// Change  tick_heater  to Zero
			tick_heater_aux_1sec = 0;	 // Change  tick_heater_aux  to Zero
		}
	}// Loop while
} // Main

#endif