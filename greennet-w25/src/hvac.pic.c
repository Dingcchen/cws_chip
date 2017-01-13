#if 0 // not using in SAM series.
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

/***** VARIABLES DECLARATION ****/

#ifndef _XTAL_FREQ
#define _XTAL_FREQ 4000000 //4Mhz FRC internal osc
#define __delay_us(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000000.0)))
#define __delay_ms(x) _delay((unsigned long)((x)*(_XTAL_FREQ/4000.0)))
#endif

unsigned int T_C;   		// Main Counter for the cooling
unsigned int TC_S;  		// Main Counter to turns compressor OFF
unsigned int A_T_H; 	// Auxiliar counter for the heater
unsigned int T_H;  		// Main Counter fro the heater
unsigned int Aux;   		// Variable used for the delays
unsigned int Aux_1;		 // Variable used for the delays
unsigned int c;     		// Variable used to keep time of compressor ON and OFF
unsigned int b;     		// Variable used to find out is compressor is ON or OFF
unsigned int a;     		// Auxiar used varible to increase to  b variable
unsigned int aa;   		// Variable used to keep the Compressor OFF time
unsigned int xx;    		// Variable used to keep the extended time
unsigned int t1;    		// Variable used to keep compressor's time ON
unsigned int t2;    		// Variable used to keep compressor's time OFF

void init(void) {
	#asm
	_asm
	MOVLW 0x0B 	// 		XX01011	Set GP0,GP1 and GP3 to input, GP2 and GP4 to output.
	TRIS 6
	_endasm	 // 	Addreaa 6 == W. Copy contents W to GPIO direction port.
	#endasm
	OPTION = 0b10000101;
}

Clean_vars() {
	/****   Set variables to 0    *****/
	T_C = 0;
	TC_S = 0;
	T_H = 0;
	A_T_H = 0;
	a = 0;
	aa=0;
	c=0;
}
Fan_Man(){
	/****   FAN is ON or AUTO    *****/
	if (GP3){
		GP2=0;
	}
	else {
		GP2=1;
	}
}

void main(void) {
	init();    		 // Enable weak pull-up ports. Timer internal clock.
	GP2 = 0;    // Turn Fan OFF
	GP4 = 0;    // Turn Compressor  OFF
	Clean_vars(); // Clean Variables
	__delay_ms(2000); // Wait for 2 Seconds before turning the FAN ON

	/***** Main Program Starts Here ****/

	while (1) // Endless LOOP
	{
		/****   FAN ON if swith is ON or Heat and Cool are ON    *****/
		
		if (!GP3 || (!GP0 && !GP1)) {
			GP2 = 1; 	// Turn Fan ON
		}
		
		/****   FAN OFF if swith is AUTO or Heat and Cool are OFF    *****/
		
		if (GP3 && (GP0 && GP1)) {
			GP2 = 0; 	// Turn Fan ON
		}
		
		/****  Intialize variables    *****/
		
		c = 0;
		b=0;
		xx=0;
		
		/****   Cooling condition ON GP0 is Low    *****/
		
		if (!GP0){
			
			/****   Cooling condition OFF GP0 is High   *****/
			
			Clean_vars(); 		// Clean Variables
			GP2 = 1;     		// Turn Fan ON
			GP4 = 1;     		// Turn Compressor ON

			if (GP0) {
				for (Aux = 0; Aux < 20; Aux++) __delay_ms(99);
				b=4;   		//Avoid going inside of the while loop below
				GP2 = 0;     // Turn Fan OFF
				GP4 = 0;     // Turn Compressor OFF
			}

			while (((GP1 && !GP0) || (GP1 && GP0) || (!GP1 && !GP0)) && b <= 3) {

				for (Aux = 0; Aux < 10; Aux++) __delay_ms(99);
				a++;
				if (a>9){
					a=0;        // Reset variable every 10 Seconds
					c++;        // Increment C every 10 seconds
					T_C++;      // Increment Timer for compressor every 10 seconds
				}
				T_H = 0;    // Reset the Counter for the Heat;
				if (T_C > 90) {
					T_C = 90;   		// 15 minutes counter
					TC_S++;     		// For Compressor if it is ON more than 30 minutes
					if (TC_S > 95) {
						TC_S = 95; 	// 30:50  minutes:Seconds counter
					}
				}
				if (c > 48) 		{  // variable "c"no greater than 8 minutes
					c = 48;
				}
				if (c == 1 && a==0) {  	//  increase b when c is 1
					b++;
					Fan_Man();        		// Check FAN input  GP3
				}
				
				/****Store t1=c when the compressor in ON is on the first time*****/
				
				if (b == 1 && !GP0){
					t1 = c;
					Fan_Man();        // Check FAN input  GP3
				}
				
				/****If GP0=0 reset variables and turn compressor OFF*****/
				
				if (b == 1 && GP0){
					c = 0;
					T_C = 0;
					TC_S=0;
					GP4 = 0;
					Fan_Man();       // Check FAN input  GP3
				}
				
				/**Calculate how long the  compressor is OFF and store it in t2***/
				/**  If compressor is OFF for more the 30 minutes  then  t2=0  ***/
				
				if (b == 2 && GP0){
					if(T_C>89){
						t2=0;
						}else{
						if((c%2)==0){
							aa++;
							if(aa>24){
								aa=24;
							}
							t2=aa;  		// T2 = 7% of the time OFF
						}
					}

					T_C = 0; // Reset counter
					TC_S=0;
					Fan_Man();        // Check FAN input  GP3
				}
				if (b == 2 && !GP0){ 	//a == 0)
					c = 0;
					GP4=1;
					GP2=1;
				}
				/** Turn compressor OFF for 3 minutes every 30 Minutes ***/
				if (TC_S > 90 && !GP0) {
					GP4 = 0;  	 // Compressor OFF
					/** 3 minutes delay  **/
					for (Aux_1 = 0; Aux_1 < 18; Aux_1++) {
						for (Aux = 0; Aux < 97; Aux++) {
							__delay_ms(104);
						}
					}
					if (!GP0){
						GP4=1; 				// Compressor ON
						GP2=1; 				// Fan ON
						}else{
						GP4=0;            // Compressor OFF
						Fan_Man();       // Check FAN input  GP3
					}
					b=4;

				}
				
				/**   Extended time   ***/
				
				if (GP0 && b == 3) {
					GP4 = 0; 		// Compressor OFF
					GP2 = 1; 		// Turn  FAN ON
					xx = t1 + t2 + c; //xx=t1*0.17+ t2*0.07 + c*.0.17
					for (Aux_1 = 0; Aux_1 < xx; Aux_1++) {
						for (Aux = 0; Aux < 15; Aux++) {
							__delay_ms(100);
						}
					}
					t1 = c;
					b = 1;
					Fan_Man();        // Check FAN input  GP3
					T_C = 0; 			// Reset counter
					TC_S=0;
					c=0;
					aa=0;
				}

			}
		}
		/** Evaluate GP0 to find out if the Heat is Activated **/
		
		if (!GP1 && GP0) {
			t1 = c;
			b = 0;
			T_C = 0;
			TC_S=0;
			c=0;
			xx=0;
			t2=0;

			for (Aux = 0; Aux < 10; Aux++) __delay_ms(102); // 1 Second delay
			A_T_H++; // Increment auxiliar Heater Counter
			if (A_T_H > 9) {
				A_T_H = 0; // Reset the auxiliar Counter for the Heater;
				T_H++; // Count Seconds the AC stayed ON  T_H++;
			}
			if (T_H > 95)  T_H = 95; 	// Reset T_H to avoid OverFlow to about 15 minutes
			if (T_H > 18)  GP2 = 1; 		// Turn Fan ON  after 3 minutes only in Heat
		}

		/***** Main delay for Heat Mode ****/

		if (GP1) 	// Evaluate GP0 if it is High then Heater is activated
		{
			if (T_H > 18)   	// Apply Delay  if  Heat is ON for more the 3 Minutes
			//Only in GA.  Apply Delay if the Heater is ON for more than 240
			//Sec. (T_C>24)
			{
				GP2 = 1; // Turn Fan ON
				for (Aux_1 = 3; Aux_1 <= T_H; Aux_1++) {
					for (Aux = 0; Aux < 20; Aux++) __delay_ms(105);
				}
			}
			GP2 = 0; 		// Turn  Fan OFF
			T_H = 0; 		// Change  T_H  to Zero
			A_T_H = 0;	 // Change  A_T_H  to Zero
		}
	}// Loop while
} // Main
#endif