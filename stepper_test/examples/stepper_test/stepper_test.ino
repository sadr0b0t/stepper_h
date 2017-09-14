#include "stepper.h"
#include "stepper_test.h"

#ifdef ARDUINO_ARCH_SAM
// implement missing _gettimeofday on SAM/Arduino Due platform (required only for tests with sput.h)
// https://stackoverflow.com/questions/7004743/unable-to-link-to-gettimeofday-on-embedded-system-elapsed-time-suggestions

extern "C" {
#include <sys/time.h>

int _gettimeofday( struct timeval *tv, void *tzvp )
{
    uint64_t t = micros();  // get uptime in microseconds
    tv->tv_sec = t / 1000000;  // convert to seconds
    tv->tv_usec = ( t % 1000000 ) / 1000;  // get remaining microseconds
    return 0;  // return non-zero for error
} // end _gettimeofday()
}
#endif // ARDUINO_ARCH_SAM

void run_test_suite() {
    // Stepper cycle lifecycle
    // avr: ok
    // sam: ok
    // pic32: ok
    stepper_test_suite_lifecycle();
    
    // Stepper cycle timer period settings
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_timer_period();
    
    // Stepper cycle timer period is aliquant part of motor step pulse delay
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_timer_period_aliquant_step_delay();
    
    // Single motor: 3 steps tick by tick on max speed
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_max_speed_tick_by_tick();
    
    // Single motor: 30000 steps on max speed
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_max_speed_30000steps();
    
    // Single motor: 5 steps tick by tick on aliquant speed
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_aliquant_speed_tick_by_tick();
    
    // 3 motors: draw triangle
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_draw_triangle();
    
    // Small step delay error handlers
    // avr/Leonardo: ?(не умещается)
    // sam: ok
    // pic32: fail (sm_x.error)
    //stepper_test_suite_small_step_delay_handlers();
    
    // Moving with variable speed: buffered steps tick by tick
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_buffered_steps_tick_by_tick();
    
    // Moving with variable speed: buffered steps
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_buffered_steps();
    
    // Step-dir driver std divider modes: 1/1, 1/18, 1/16, 1/32
    // avr/Leonardo: ok (целиком не умещается - плата шьется, но виснет; чтобы заработало, нужно закоментить в тестах пару кейсов)
    // sam: ok
    // pic32: ok
    //stepper_test_suite_driver_std_modes();
    
    // Step-dir driver std divider modes for 2 motors at a time
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_driver_std_modes_2motors();
    
    // Single motor: exit bounds (issue #1) - whirl
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_exit_bounds_issue1_whirl();
    
    // Single motor: exit bounds (issue #1) - steps
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_exit_bounds_issue1_steps();
    
    // Single motor: exit bounds (issue #9) - steps
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_exit_bounds_issue9_steps();
    
    // Single motor: test square signal (issue #16)
    // avr: ok
    // sam: ok
    // pic32: ok
    //stepper_test_suite_square_sig_issue16();
}

void setup() {
    Serial.begin(9600);
  
    // disable hardware timer
    stepper_set_timer_enabled(false);
    
    while (!Serial);
  
    Serial.println("#################### Start testing...");
    
    // select tests to compile/run above
    run_test_suite();
    
    // all tests in one call (takes much place)
    //stepper_test_suite();
    
    
    Serial.println("#################### Finished testing");
}

void loop() {
  delay(2000);
}

