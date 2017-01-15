#include "asf.h"
#include <stdio.h>
#include "device.h"
#include "cws_wifi_scheduler.h"
#include "ctl_algorithm.h"
#include "gnet.h"
#include "gnet_hvac.h"
#include "ax_task.h"


#define _OLD_CODE 0



extern unsigned long gAcc_MS;


unsigned int main_counter_comp;   		    // Main Counter for the cooling
unsigned int main_counter_for_comp_off;     // Main Counter to turn compressor OFF
unsigned int aux_counter_heat; 	            // Auxiliar counter for the heater
unsigned int main_counter_heat;  		    // Main Counter fro the heater
unsigned int tick_comp_on_and_off;          // Variable used to keep time of compressor ON and OFF
unsigned int control_stage_comp;     		// Variable used to find out is compressor is ON or OFF
unsigned int aux_to_control_stage_comp;     // Auxiar used varible to increase to  control_stage variable
unsigned int keep_time_comp_off;   		    // Variable used to keep the Compressor OFF time
unsigned int keep_time_extented;    		// Variable used to keep the extended time
unsigned int t1_comp_ontime;    		    // Variable used to keep compressor's time ON
unsigned int t2_comp_offtime;    		    // Variable used to keep compressor's time OFF



void ctl_port_init(void) {

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
  
	
	
	set_fan();
	delay_ms(1000);
	set_comp();
	
	

}

static int _Get_fan(void){
	
	return port_pin_get_input_level(FAN_INPUT_PIN);
	
}


static int _is_comp_off(void){
	
	return port_pin_get_input_level(COMPRESSOR_INPUT_PIN);
	
}

static int _is_heater_off(void){
	
	return port_pin_get_input_level(HEATER_INPUT_PIN);
	
}


#define get_fan  (_Get_fan())
#define is_comp_off (_is_comp_off())
#define is_heater_off (_is_heater_off())

void set_fan(void){
	
	delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_SET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_SET,OUTPUT_INACTIVE);
	delay_ms(50);
	
	port_pin_set_output_level(FAN_OUTPUT_PIN_SET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_SET,OUTPUT_INACTIVE);
}


void reset_fan(void){
	
    delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_RESET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_RESET,OUTPUT_INACTIVE);
	delay_ms(50);
	
	port_pin_set_output_level(FAN_OUTPUT_PIN_RESET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(FAN_OUTPUT_PIN_RESET,OUTPUT_INACTIVE);
	
}

void set_comp(void){
	
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_SET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_SET,OUTPUT_INACTIVE);
	delay_ms(50);
	
	port_pin_set_output_level(COMPRESSOR_PIN_SET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_SET,OUTPUT_INACTIVE);
	
}

void reset_comp(void){
	
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_RESET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_RESET,OUTPUT_INACTIVE);
	delay_ms(50);
	
	port_pin_set_output_level(COMPRESSOR_PIN_RESET,OUTPUT_ACTIVE);
	delay_ms(25);
	port_pin_set_output_level(COMPRESSOR_PIN_RESET,OUTPUT_INACTIVE);
	
}



void Clean_vars(void) {
	/****   Set variables to 0    *****/
	main_counter_comp = 0;
	main_counter_for_comp_off = 0;
	main_counter_heat = 0;
	aux_counter_heat = 0;
	aux_to_control_stage_comp = 0;
	keep_time_comp_off = 0;
	tick_comp_on_and_off = 0;
}

void Clean_vars_after_greennet(void) {
	
	/****   Set variables to 0  After Greennet control  *****/
	main_counter_comp = 0;
	main_counter_for_comp_off = 0;
	main_counter_heat = 0;
	aux_counter_heat = 0;
	aux_to_control_stage_comp = 0;
	keep_time_comp_off = 0;
	tick_comp_on_and_off = 0;
	
	keep_time_extented=0;
}

static void Fan_Man(void) {
	
	/****   FAN is ON or AUTO    *****/
	if (get_fan) {
		set_fan(); /* GP2=0; */
		} else {
		reset_fan(); /* GP2=1; */

		}
	}
	
static void Comp_man(void)
{
	/*****  Compressor is ON or OFF ********/
	if(is_comp_off)
    set_comp();
	else
	reset_comp();
}


void control_conditioner(void) {
	
	
	static unsigned long algo_delay = 0, algo_delay1 = 0, algo_delay2 = 0, algo_delay3 = 0, algo_delay4 = 0, algo_delay5 =0;
	static unsigned int keep_time_extented_bk = 0, main_counter_heat_bk = 0;

	
	if      (algo_delay)
	goto _ALGO_DELAY;
	else if (algo_delay1)
	goto _ALGO_DELAY1;
	//else if (algo_delay2)
	//goto _ALGO_DELAY2;
	else if (algo_delay3)
	goto _ALGO_DELAY3;
	else if (algo_delay4)
	goto _ALGO_DELAY4;
	else if (algo_delay5)
	goto _ALGO_DELAY5;


	
	/****   FAN ON if switch is ON or (Heat and Cool) are ON    *****/        //Come back to check different scenarios

	if (!get_fan || (!is_comp_off && !is_heater_off)) {
		reset_fan(); /*  GP2 = 1;  // Turn Fan ON */
	}
	
	/****   FAN OFF if switch is AUTO and (Heat and Cool) are OFF    *****/
	
	if (get_fan && (is_comp_off && is_heater_off)) {
		set_fan(); /*  GP2 = 0;  // Turn Fan OFF */
	}
	
	
	/****  Intialize variables    *****/

	tick_comp_on_and_off = 0;
	control_stage_comp=0;
	keep_time_extented=0;


	/****   Cooling condition ON GP0 is Low    *****/
	
	if (!is_comp_off) {
		
		/****   Cooling condition OFF GP0 is High   *****/
		
		Clean_vars();    // Clean Variables
		reset_fan();     /*  GP2 = 1;  // Turn Fan ON */
		delay_ms(500);
		reset_comp(); /*  GP4 = 1;         // Turn Compressor ON */

		if (is_comp_off) {
			
#if 1 // austion version
			algo_delay = gAcc_MS;
_ALGO_DELAY:

			if ((gAcc_MS - algo_delay) > (99 * 20)) {
				printf("algo lead time = %ld\r\n", gAcc_MS - algo_delay);
				algo_delay = 0;
			} else
			return;
			
#endif
			control_stage_comp = 4;      //Avoid going inside of the while loop below
			set_comp();      /* GP4 = 0;     // Turn Comp OFF */
			delay_ms(500); 
			set_fan();     /* GP2 = 0;     // Turn Fan OFF */
		}
		
		
		while (((is_heater_off && !is_comp_off)
		|| (is_heater_off && is_comp_off)
		|| (!is_heater_off && !is_comp_off))
		&& control_stage_comp <= 3) {
			
#if 1 // austion version
			algo_delay1 = gAcc_MS;
_ALGO_DELAY1:

			if ((gAcc_MS - algo_delay1) > (99 * 10)) {
				printf("algo 1 lead time = %ld\r\n", gAcc_MS - algo_delay1);
				algo_delay1 = 0;
			} else
			return;
			
#endif          
              
              if (control_stage_comp == 0 && is_comp_off)
              {
				  control_stage_comp = 4;      //Avoid going inside of the while loop below
			      set_comp();      /* GP4 = 0;     // Turn Comp OFF */
			      delay_ms(500); 
			      set_fan();
				  
              }
			   aux_to_control_stage_comp++;                   //Check timings here
			
			if (aux_to_control_stage_comp > 9)
			{
				aux_to_control_stage_comp = 0;  //Reset aux_to_control_stage_comp every 10 seconds
				tick_comp_on_and_off++; // Increment tick_comp_on_and_off every 10 seconds
				main_counter_comp++; //Increment Timer for compressor every 10 seconds
				
			}
			
			main_counter_heat = 0; // Reset the counter for the Heat
			
			if (main_counter_comp > 90)
			{
				main_counter_comp = 90;               // 15minutes counter
				main_counter_for_comp_off++;         // For Compressor if it is ON more than 30 minutes
				
				if (main_counter_for_comp_off > 95)
				{
					main_counter_for_comp_off = 95;
				}
			}
			
			if (tick_comp_on_and_off > 48)            // variable "c" no greater than 8 minutes
			{
				tick_comp_on_and_off = 48;
			}
			
			if(tick_comp_on_and_off == 1 && aux_to_control_stage_comp == 0)            	//  increase b when c is 1
			{
				control_stage_comp++;
				Fan_Man();
			}
			
			/****Store t1 = c when the compressor is ON for the first time*****/

			if (control_stage_comp == 1 && !is_comp_off) {
				t1_comp_ontime = tick_comp_on_and_off;
				Fan_Man();        // Check FAN input  GP3
				
			}
			
			/****If is_comp_off is low,  reset variables and turn compressor OFF*****/
			
			if (control_stage_comp == 1 && is_comp_off) {
				tick_comp_on_and_off = 0;
				main_counter_comp = 0;
				main_counter_for_comp_off = 0;
				
				set_comp();
				delay_ms(500);
				Fan_Man();       // Check FAN input  GP3
				
				
			}
			
			/**Calculate how long the  compressor is OFF and store it in t2***/
			/**  If compressor is OFF for more than 30 minutes  then  t2=0  ***/
			
			if (control_stage_comp == 2 && is_comp_off) {
				if(main_counter_comp > 89) {                             //come back for timing check
					t2_comp_offtime = 0;
					} else {
					if ((tick_comp_on_and_off % 2) == 0) {
						keep_time_comp_off++;
						if (keep_time_comp_off > 24) {
							keep_time_comp_off = 24;
						}
						t2_comp_offtime = keep_time_comp_off;      // T2 = 7% of the time OFF
					}
				}
				
				main_counter_comp = 0; //Reset the counter
				main_counter_for_comp_off = 0;
				set_comp();
				delay_ms(500);
			
				Fan_Man();             //Check Fan input GP3
				}

			if (control_stage_comp == 2 && !is_comp_off) {  //a == 0)
				tick_comp_on_and_off = 0;
				
				reset_fan(); /* GP2=1; */
				delay_ms(500);
				reset_comp();  /*    GP4=1; */
			}
			
			/** Turn compressor OFF for 3 minutes every 30 Minutes ***/
		//if (main_counter_for_comp_off > 75 && !is_comp_off) {
				//
				//reset_comp(); /*GP4 = 0;     // Compressor OFF */  //testing
		//
//
//#if 1 // austion version
			   //algo_delay2 = gAcc_MS;
			//
//_ALGO_DELAY2:
				///* 3-minutes delay */
				//
				//if ((gAcc_MS - algo_delay2) > (3 * 60 * 1000)) {
					//printf("algo 2 lead time = %ld\r\n", gAcc_MS - algo_delay2);
					//algo_delay2 = 0;
				//} else
				//return;
				//
//#endif
			
			//
			//if (!is_comp_off) {
					 //
					//reset_fan(); /* GP2=1; // Fan ON */
					//delay_ms(500);
					//reset_comp(); /*    GP4=1; // Compressor ON */
					//
					//} else {
					//set_comp(); /*    GP4=0; // Compressor OFF */
					//delay_ms(500);
					//Fan_Man();
				//}
				//control_stage_comp = 4;
			//}
			
			
			/**   Extended time   ***/
			
			if (is_comp_off && control_stage_comp == 3) {
				set_comp(); /* GP4 = 0;    // Compressor OFF */
				reset_fan(); /* GP2=1; // Fan ON */
				keep_time_extented = t1_comp_ontime + t2_comp_offtime + tick_comp_on_and_off; //xx=t1*0.17+ t2*0.07 + c*.0.17

#if 1 // austion version
				keep_time_extented_bk = keep_time_extented;
				algo_delay3 = gAcc_MS;
				_ALGO_DELAY3:
				
				if ((gAcc_MS - algo_delay3) > (keep_time_extented_bk * 15 * 100)) {
					printf("algo 3 lead time = %ld\r\n", gAcc_MS - algo_delay3);
					algo_delay3 = 0;
				} else
				return;
				
#endif
				
				t1_comp_ontime = tick_comp_on_and_off;
				control_stage_comp = 1;
				Fan_Man();           //Check fan input GP3
				main_counter_comp = 0;
				main_counter_for_comp_off = 0;
				tick_comp_on_and_off = 0;
				keep_time_comp_off = 0;
				
			}
		}
	}
	
	
	
	
	
	
	/** Evaluate GP0 to find out if the Heat is Activated **/
	
	if (!is_heater_off && is_comp_off) {
		t1_comp_ontime = tick_comp_on_and_off;
		control_stage_comp = 0;
		main_counter_comp = 0;
		main_counter_for_comp_off=0;
		tick_comp_on_and_off=0;
		keep_time_extented=0;
		t2_comp_offtime=0;

#if 1 // austion version
		algo_delay4 = gAcc_MS;
		_ALGO_DELAY4:

		if ((gAcc_MS - algo_delay4) > (1000)) {
			printf("algo 4 lead time = %ld\r\n", gAcc_MS - algo_delay4);
			algo_delay4 = 0;
		} else
		return;

#endif
		
		aux_counter_heat++; //Increment Auxilar heater counter
		
		if(aux_counter_heat > 9)
		{
			aux_counter_heat = 0; //Reset Auxiliar
			main_counter_heat++;  // Count seconds AC stayed ON T_H++
		}
		
		if (main_counter_heat > 95)
		main_counter_heat = 95;
		if (main_counter_heat > 17)
		reset_fan();
		
		/***** Main delay for Heat Mode ****/

		if (is_heater_off)  // Evaluate GP0 if it is High then Heater is activated
		{
			if (main_counter_heat > 18)     // Apply Delay  if  Heat is ON for more the 3 Minutes
			//Only in GA.  Apply Delay if the Heater is ON for more than 240
			//Sec. (T_C>24)
			{
				reset_fan(); /* GP2 = 1; // Turn Fan ON */
				
				
				#if 1 // austion version
				main_counter_heat_bk = main_counter_heat;
				algo_delay5 = gAcc_MS;
				_ALGO_DELAY5:

				if ((gAcc_MS - algo_delay5) > ((main_counter_heat_bk - 3) * 20 * 105)) {
					printf("algo 5 lead time = %ld\r\n", gAcc_MS - algo_delay5);
					algo_delay5 = 0;
				} else
				return;
				#endif
			}
			Fan_Man();
			main_counter_heat = 0;
			aux_counter_heat = 0;

		}
	}
}



int control_conditioner1(void)
	{
	
	struct device *pd = get_device_ctx();
	unsigned long tick = get_time_ms();
	static int event_stage = 0;
	static unsigned long event_time_1;
	//static unsigned long demand_event_time_1;
	//static unsigned long demand_current_time_1; 
	//unsigned long result;
	
	unsigned int ran = tick % 30;  //random number for read frequency
	
	
   if (!is_heater_off)
		{
			pd->hvac.demand_resp_code = 0;
			
		}
	 
	 else
	 {
		 //if(pd->hvac.demand_event_stage == 0)
		 //{
	     	//pd->hvac.demand_event_stage = 1;
		    //demand_current_time_1 = pd->hvac.current_time;
		    //demand_event_time_1 = tick;
		 //}
    //
	//result = pd->hvac.demand_resp_time - demand_current_time_1;
	//
	//if (pd->hvac.demand_event_stage == 1 && (result < tick  - demand_event_time_1))
	////if((pd->hvac.current_time - 30000) <= pd->hvac.demand_resp_time && pd->hvac.demand_resp_time <= (pd->hvac.current_time + 30000))
	//
	//{
		
	/* event start */
	if(event_stage == 0)
	 {
		pd->hvac.demand_control_stage = 1;
		event_stage = 1;
		event_time_1 = tick;
		
		delay_ms(pd->hvac.delay + ran);
		set_comp();
		
	  }
	  
//}
	/* check 6-min elapsed */
	
	if (event_stage == 1 && (tick - event_time_1 > 6 * 60 * 1000))
	
		{
			
		   
			event_stage = 0;
			
			
			pd->hvac.demand_resp_code = 0;
			pd->hvac.demand_event_stage_read = 0;
			pd->hvac.demand_resp_code_dup = 0;
			pd->hvac.demand_control_stage =0;
			pd->hvac.demand_date_event = 0;
			pd->hvac.demand_time_event = 0;
			
			//Comp_man();
			
			
			delay_ms(pd->hvac.delay + ran);
			Clean_vars_after_greennet();
			control_stage_comp = 4;      //Avoid going into the loop
			
		}
	

}
	return 0;
}




int control_conditioner2(void)
{
	
	struct device *pd = get_device_ctx();
	unsigned long tick = get_time_ms();
	static int event_stage = 0;
	static unsigned long event_time_2;
	//static unsigned long demand_event_time_2;
	//static unsigned long demand_current_time_2;
	//unsigned long result;
	
	unsigned int ran = tick % 30;  //random number for read frequency
	
	
	if (!is_heater_off)
	{
		pd->hvac.demand_resp_code = 0;
		
	}
	else
	{
		//if (pd->hvac.demand_event_stage == 0)
		//{
			//pd->hvac.demand_event_stage = 2;
			//demand_current_time_2 = pd->hvac.current_time;
			//demand_event_time_2 = tick;
		//}
		//result = pd->hvac.demand_resp_time - demand_current_time_2;
		
		//if((pd->hvac.current_time - 30000) <= pd->hvac.demand_resp_time && pd->hvac.demand_resp_time <= (pd->hvac.current_time + 30000))
		//{
			//
			/* event start */
			if(event_stage == 0)
			{
				pd->hvac.demand_control_stage = 2;
				event_stage = 2;
				event_time_2 = tick;
				
				delay_ms(pd->hvac.delay + ran);
				set_comp();
				
			}
	//	}

			/* check 12-min elapsed */
			
			if (event_stage == 2 && (tick - event_time_2 > 12 * 60 * 1000))
			
			{
				
				event_stage = 0;
			
			
			pd->hvac.demand_resp_code = 0;
			pd->hvac.demand_event_stage_read = 0;
			pd->hvac.demand_resp_code_dup = 0;
			pd->hvac.demand_control_stage =0;
			pd->hvac.demand_date_event = 0;
			pd->hvac.demand_time_event = 0;
			
			//Comp_man();
			
			
			delay_ms(pd->hvac.delay + ran);
			Clean_vars_after_greennet();
			control_stage_comp = 4;      //Avoid going into the loop
				
			}
			
		
	}
	return 0;
}

int control_conditioner3()
{
	
  struct device *pd = get_device_ctx();
  unsigned long tick = get_time_ms();
  static int event_stage = 0;
  static unsigned long event_time_3;
  //static unsigned long demand_event_time_3;
  //static unsigned long demand_current_time_3;
  //unsigned long result;
  
  unsigned int ran = tick % 30;  //random number for read frequency
  
  
  if (!is_heater_off)
  {
	  pd->hvac.demand_resp_code = 0;
	  
  }
  else
  {
	  //if (pd->hvac.demand_event_stage == 0)
	  //{
		//  pd->hvac.demand_event_stage = 3;
		  //demand_current_time_3 = pd->hvac.current_time;
		  //demand_event_time_3 = tick;
	  //}
	//  result = pd->hvac.demand_resp_time - demand_current_time_3;
	  
	// if((pd->hvac.current_time - 30000) <= pd->hvac.demand_resp_time && pd->hvac.demand_resp_time <= (pd->hvac.current_time + 30000))
	//  {
		  
		  /* event start */
		  if(event_stage == 0)
		  {
			  pd->hvac.demand_control_stage = 3;
			  event_stage = 3;
			  event_time_3 = tick;
			  
			  delay_ms(pd->hvac.delay + ran);
			  set_comp();
			  delay_ms(500);
			  set_fan();
			  
		  }
	 // }

		  /* check 12-min elapsed */
		  
		  if (event_stage == 3 && (tick - event_time_3 > 12 * 60 * 1000))
		  
		  {
			 event_stage = 0;
			
			
			pd->hvac.demand_resp_code = 0;
			pd->hvac.demand_event_stage_read = 0;
			pd->hvac.demand_resp_code_dup = 0;
			pd->hvac.demand_control_stage =0;
			pd->hvac.demand_date_event = 0;
			pd->hvac.demand_time_event = 0;
			
			//Comp_man();
			
			
			delay_ms(pd->hvac.delay + ran);
			Clean_vars_after_greennet();
			control_stage_comp = 4;      //Avoid going into the loop
			  
		  }
		  
	  
  }
  return 0;
  }

int check_time_for_event()
{
	struct device *pd = get_device_ctx();
	static unsigned long event_time_demand;
    unsigned long tick = get_time_ms();
	static unsigned long result;
	
	if (pd->hvac.demand_event_stage_time == 0 && pd->hvac.demand_time_event == 0)
	{
		pd->hvac.demand_event_stage_time = 1;
		event_time_demand = tick;
		result = pd->hvac.demand_resp_time - pd->hvac.current_time;
	}
	
	if (pd->hvac.demand_event_stage_time == 1 && (tick - event_time_demand > result))
	{
		pd->hvac.demand_time_event = 1;
	}
	
	return 0;
}