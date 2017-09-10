#ifndef STEPPER_CONFIGURE_TIMER_H
#define STEPPER_CONFIGURE_TIMER_H

// make timer and prescaler constans also available to public
extern "C"{
    #include "timer_setup.h"
}

// Typical freqs

/**
 * freq: 1MHz = 1000000 ops/sec
 * period: 1sec/1000000 = 1us
 */
unsigned long stepper_configure_timer_1MHz(int timer);

/**
 * freq: 500KHz = 500000 ops/sec
 * period: 1sec/500000 = 2us
 */
unsigned long stepper_configure_timer_500KHz(int timer);

/**
 * freq: 200KHz = 200000 ops/sec
 * period: 1sec/200000 = 5us
 */
unsigned long stepper_configure_timer_200KHz(int timer);

/**
 * freq: 100KHz = 100000 ops/sec
 * period: 1sec/100000 = 10us
 */
unsigned long stepper_configure_timer_100KHz(int timer);

/**
 * freq: 50KHz = 50000 ops/sec
 * period: 1sec/50000 = 20us
 */
unsigned long stepper_configure_timer_50KHz(int timer);

/**
 * freq: 20KHz = 20000 ops/sec
 * period: 1sec/20000 = 50us
 */
unsigned long stepper_configure_timer_20KHz(int timer);


/**
 * freq: 10KHz = 10000 ops/sec
 * period: 1sec/10000 = 100us
 */
unsigned long stepper_configure_timer_10KHz(int timer);

/**
 * freq: 5KHz = 5000 ops/sec
 * period: 1sec/5000 = 200us
 */
unsigned long stepper_configure_timer_5KHz(int timer);

/**
 * freq: 2KHz = 2000 ops/sec
 * period: 1sec/2000 = 500us
 */
unsigned long stepper_configure_timer_2KHz(int timer);

/**
 * freq: 1KHz = 1000 ops/sec
 * period: 1sec/1000 = 1ms
 */
unsigned long stepper_configure_timer_1KHz(int timer);

/**
 * freq: 500Hz = 500 ops/sec
 * period: 1sec/500 = 2ms
 */
unsigned long stepper_configure_timer_500Hz(int timer);

/**
 * freq: 200Hz = 200 ops/sec
 * period: 1sec/200 = 5ms
 */
unsigned long stepper_configure_timer_200Hz(int timer);

/**
 * freq: 100Hz = 100 ops/sec
 * period: 1sec/100 = 10ms
 */
unsigned long stepper_configure_timer_100Hz(int timer);

/**
 * freq: 50Hz = 50 ops/sec
 * period: 1sec/50 = 20ms
 */
unsigned long stepper_configure_timer_50Hz(int timer);

/**
 * freq: 20Hz = 20 ops/sec
 * period: 1sec/20 = 50ms
 */
unsigned long stepper_configure_timer_20Hz(int timer);

/**
 * freq: 10Hz = 10 ops/sec
 * period: 1sec/10 = 100ms
 */
unsigned long stepper_configure_timer_10Hz(int timer);

/**
 * freq: 5Hz = 5 ops/sec
 * period: 1sec/5 = 200ms
 */
unsigned long stepper_configure_timer_5Hz(int timer);

/**
 * freq: 2Hz = 2 ops/sec
 * period: 1sec/2 = 500ms
 */
unsigned long stepper_configure_timer_2Hz(int timer);

/**
 * freq: 1Hz = 1 ops/sec
 * period: 1sec
 */
unsigned long stepper_configure_timer_1Hz(int timer);

#endif // STEPPER_CONFIGURE_TIMER_H


