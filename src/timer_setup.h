#ifndef TIMER_SETUP_H
#define TIMER_SETUP_H

// Define timer ids
extern const int _TIMER1;
extern const int _TIMER2;
extern const int _TIMER3;
extern const int _TIMER4;
extern const int _TIMER5;
extern const int _TIMER6;
extern const int _TIMER7;
extern const int _TIMER8;
extern const int _TIMER9;

// 32-bit timers
extern const int _TIMER1_32BIT;
extern const int _TIMER2_32BIT;
extern const int _TIMER3_32BIT;
extern const int _TIMER4_32BIT;
extern const int _TIMER5_32BIT;
extern const int _TIMER6_32BIT;
extern const int _TIMER7_32BIT;
extern const int _TIMER8_32BIT;
extern const int _TIMER9_32BIT;

// Default timer
extern const int TIMER_DEFAULT;

// Define timer prescaler options
extern const int TIMER_PRESCALER_1_1;
extern const int TIMER_PRESCALER_1_2;
extern const int TIMER_PRESCALER_1_4;
extern const int TIMER_PRESCALER_1_8;
extern const int TIMER_PRESCALER_1_16;
extern const int TIMER_PRESCALER_1_32;
extern const int TIMER_PRESCALER_1_64;
extern const int TIMER_PRESCALER_1_128;
extern const int TIMER_PRESCALER_1_256;
extern const int TIMER_PRESCALER_1_1024;

/**
 * Init ISR (Interrupt service routine) for the timer and start timer.
 * 
 * General algorithm
 * http://www.robotshop.com/letsmakerobots/arduino-101-timers-and-interrupts
 * 1. CPU frequency 16Mhz for Arduino
 * 2. maximum timer counter value (256 for 8bit, 65536 for 16bit timer)
 * 3. Divide CPU frequency through the choosen prescaler (16000000 / 256 = 62500)
 * 4. Divide result through the desired frequency (62500 / 2Hz = 31250)
 * 5. Verify the result against the maximum timer counter value (31250 < 65536 success).
 *    If fail, choose bigger prescaler.
 * 
 * Example: to set timer clock period to 20ms (50 operations per second == 50Hz)
 * 
 * 1) on 16MHz CPU (AVR Arduino)
 *   use prescaler 1:8 (TIMER_PRESCALER_1_8) and adjustment=40000-1:
 *   16000000/8/50=40000, minus 1 cause count from zero.
 * 
 * 2) on 80MHz CPU (PIC32MX ChipKIT)
 *   use prescaler 1:64 (TIMER_PRESCALER_1_64) and adjustment=25000-1:
 *   80000000/64/50=25000, minus 1 cause count from zero.
 *
 * 3) on 84MHz CPU (SAM Arduino Due)
 *   use prescaler 1:128 (TIMER_PRESCALER_1_128) and adjustment=13125-1:
 *   80000000/128/50=13125, minus 1 cause count from zero.
 * 
 * Timer interrupt _handler timer_handle_interrupts would be called every 20ms
 * (50 times per second) in this case.
 * 
 * @param timer
 *   system timer id: use TIMER_DEFAULT for default timer
 *   or _TIMER1, _TIMER2, _TIMER3, _TIMER4, TIMER5,
 *   _TIMER2_32BIT or _TIMER4_32BIT for specific timer.
 *   note: _TIMERX constant would be set to '-1' if selected timer
 *   is not available on current platform.
 * @param prescaler
 *   timer prescaler (1, 2, 4, 8, 16, 32, 64, 256),
 *   use constants: PRESCALER_1, PRESCALER_2, PRESCALER_8,
 *   PRESCALER_16, PRESCALER_32, PRESCALER_64, PRESCALER_256
 * @param adjustment
 *   adjustment divider after timer prescaled - timer compare match value.
 */
void _timer_init_ISR(int timer, int prescaler, unsigned int period);

/**
 * Stop ISR (Interrupt service routine) for the timer.
 * 
 * @param timer
 *     system timer id for started ISR
 */
void _timer_stop_ISR(int timer);

/**
 * Timer ISR (Interrupt service routine) handler.
 * Implementation must be provided in module with user code.
 * 
 * @param timer
 *     system timer id for started ISR
 */
void _timer_handle_interrupts(int timer);

#endif


