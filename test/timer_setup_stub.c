
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
    
}

void stopTimerISR(int timer) {

}



