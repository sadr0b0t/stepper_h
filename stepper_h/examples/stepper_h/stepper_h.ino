#include "stepper.h"

// Stepper motors
static stepper sm_x, sm_y, sm_z;

static void prepare_line1() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode,
    //     stepper_info_t *stepper_info=NULL);

    // make 20000 steps with 1000 microseconds delay
    // X.pos would go from 0 to
    // 7500*20000=150000000 nanometers = 150000 micrometers = 150 millimeters
    // during 1000*20000=20000000microseconds=20seconds
    prepare_steps(&sm_x, 20000, 1000);
    // make 10000 steps with 2000 microseconds delay
    // Y.pos would go from 0 to
    // 7500*10000=75000000 nanometers = 75000 micrometers = 75 millimeters
    // during 2000*10000=20000000microseconds=20seconds
    prepare_steps(&sm_y, 10000, 2000);
    // make 1000 steps with 20000 microseconds delay
    // Z.pos would go from 0 to 
    // 7500*1000=7500000 nanometers = 7500 micrometers  = 7.5 millimeters
    // during 20000*1000=20000000microseconds=20seconds
    prepare_steps(&sm_z, 1000, 20000);
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting stepper_h test...");
    
    // connected stepper motors
    // init_stepper(stepper* smotor,  char name, 
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, int pulse_delay,
    //     int distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long min_pos, long max_pos);
    
    // Pinout for CNC-shield
    
    // X
    init_stepper(&sm_x, 'x', 2, 5, 8, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 3, 6, 8, false, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, CONST, CONST, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 4, 7, 8, false, 1000, 7500);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, CONST, 0, 100000000);
    
    // configure motors before starting steps
    prepare_line1();
    // start motors, non-blocking
    stepper_start_cycle();
}

void loop() {
    static int prevTime = 0;
    // Debug messages - print current positions of motors once per second
    // while they are rotating, once per 10 seconds when they are stopped
    int currTime = millis();
    if( (stepper_is_cycle_running() && (currTime - prevTime) >= 1000) || (currTime - prevTime) >= 10000 ) {
        prevTime = currTime;
        Serial.print("X.pos=");
        Serial.print(sm_x.current_pos, DEC);
        Serial.print(", Y.pos=");
        Serial.print(sm_y.current_pos, DEC);
        Serial.print(", Z.pos=");
        Serial.print(sm_z.current_pos, DEC);
        Serial.println();
    }
    
    // put any code here, it would run while the motors are rotating
}

