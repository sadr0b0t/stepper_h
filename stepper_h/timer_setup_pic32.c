/************************************************************************/
/*																		*/
/*	int.c	--	Handles timer interrupts for PIC32      				*/
/*																		*/
/************************************************************************/
/*	Author:		Michelle Yu                                             */
/*                          											*/
/*	Copyright 2011, Digilent Inc.										*/
/*  This library is free software; you can redistribute it and/or       */
/*  modify it under the terms of the GNU Lesser General Public          */
/*  License as published by the Free Software Foundation; either        */
/*  version 2.1 of the License, or (at your option) any later version.  */
/*                                                                      */
/*  This library is distributed in the hope that it will be useful,     */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU   */
/*  Lesser General Public License for more details.                     */
/*                                                                      */
/*  You should have received a copy of the GNU Lesser General Public    */
/*  License along with this library; if not, write to the Free Software */
/*  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA           */
/*  02110-1301 USA                                                      */
/************************************************************************/
/*  File Description:													*/
/*	This file declares functions that handle timer interrupts for       */
/*  chipKIT PIC32 boards.												*/
//************************************************************************
//*	Edit History
//************************************************************************
//*	Aug 18,	2011	<MichelleY> file header comment block reformatted
//*	Sep  5,	2011	<GeneA> Added include of p32xxxx.h and plib.h to fix
//*						compile errors intoduced when include of plib.h was removed
//*						HardwareSerial.h
//* Feb  7, 2013	<GeneApperson> Removed dependency on Microchip plib library
//************************************************************************

# ifdef __PIC32__

#define OPT_SYSTEM_INTERNAL
#define OPT_BOARD_INTERNAL
#include <p32xxxx.h>
#include <sys/attribs.h>
#include <pins_arduino.h>

#include "timer_setup.h"

// M00BUG: This is hard coded for specific registers for interrupt priority
// flag bits and enable bits. This code happens to work correctly for all
// currently existing PIC32 devices, but this needs to be rewritten to be
// more generic.

void __attribute__((interrupt(),nomips16)) T3_IntHandler (void){
    handle_interrupts(TIMER3); 
    IFS0CLR = 0x1000; // Clear timer interrupt status flag
}


void __attribute__((interrupt(),nomips16)) T4_IntHandler (void){
    handle_interrupts(TIMER4); 
    IFS0CLR = 0x10000; // Clear timer interrupt status flag
}

void __attribute__((interrupt(),nomips16)) T5_IntHandler (void){
    handle_interrupts(TIMER5); 
    IFS0CLR = 0x100000; // Clear timer interrupt status flag
}

/**
 * Init ISR (Interrupt service routine) for the timer.
 * 
 * @param timer 
 *         system timer id: use TIMER3, TIMER4 or TIMER5
 * @param prescaler 
 *         timer prescaler (1, 2, 4, 8, 16, 32, 64, 256),
 *         use constants: PRESCALER_1, PRESCALER_2, PRESCALER_8,
 *         PRESCALER_16, PRESCALER_32, PRESCALER_64, PRESCALER_256
 * @param period
 *         timer period - adjustment divider after timer prescaled.
 * 
 * Example: to set timer clock period to 20ms (50 operations per second)
 * use prescaler 1:64 (0x0060) and period=0x61A8:
 * 80000000/64/50=25000=0x61A8
 */
void initTimerISR(int timer, int prescaler, int period) {
    if(timer == TIMER3) {

        // set the vector up
        setIntVector(_TIMER_3_VECTOR, T3_IntHandler);

        // set timer 3 clock period 
        T3CON = prescaler; // set prescaler
        TMR3 = 0;
        PR3 = period;
           
        IFS0CLR = 0x1000;// Clear the T3 interrupt flag 
        IEC0SET = 0x1000;// Enable T3 interrupt 
     
        IPC3CLR = 0x0000001F;
        IPC3SET = (_T3_IPL_IPC << 2) | _T3_SPL_IPC;
       
        T3CONSET = 0x8000;// Enable Timer3
    } else if(timer == TIMER4) {
        // set the vector up
        setIntVector(_TIMER_4_VECTOR, T4_IntHandler);
 
        // set timer 4 clock period 
        T4CON = prescaler; // set prescaler
        TMR4 = 0;
        PR4 = period;        
           
        IFS0CLR = 0x10000;// Clear the T4 interrupt flag 
        IEC0SET = 0x10000;// Enable T4 interrupt 
     
        IPC4CLR = 0x0000001F;
        IPC4SET = (_T4_IPL_IPC << 2) | _T4_SPL_IPC;   
       
        T4CONSET = 0x8000;// Enable Timer4	 
    } else if(timer == TIMER5) {
        // set the vector up
        setIntVector(_TIMER_5_VECTOR, T5_IntHandler);

        // set timer 5 clock period
        T5CON = prescaler; // set prescaler
        TMR5 = 0;
        PR5 = period;        
           
        IFS0CLR = 0x100000;// Clear the T5 interrupt flag 
        IEC0SET = 0x100000;// Enable T5 interrupt 
     
        IPC5CLR = 0x0000001F;
        IPC5SET = (_T5_IPL_IPC << 2) | _T5_SPL_IPC;
       
        T5CONSET = 0x8000;// Enable Timer5
    }  
}

void stopTimerISR(int timer) {
    //disable use of the given timer
    if (timer == TIMER3) {
        IEC0CLR = 0x1000;		// disable T3 interrupt 
    } else if (timer == TIMER4) {
        IEC0CLR = 0x10000;		// disable T4 interrupt 
    } else if (timer == TIMER5) {
        IEC0CLR = 0x100000;		// disable T5 interrupt 
    }
}

#endif // __PIC32__

