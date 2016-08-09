# stepper_h
Non-blocking stepper motor control library for ChipKIT (Arduino-compatible pic32-based dev board)

Hello,

Here is my library to control multiple stepper motors connected to ChipKIT board via step-dir driver interface
https://github.com/1i7/stepper_h

The major benefit in comparison to Arduino Stepper.h lib https://www.arduino.cc/en/Reference/StepperStep is that my stepper_h calls are non-blocking: you can run simultaneously multiple motors and receive commands via Wify, Serial port and do anything else in the main loop at the same time.

It uses PIC32 timer interrupts to generate step signals in background (basic code to init timer taken from ChipKIT Servo.h library port) and ChipKIT/Arduino API to deal with pins, so it would work only on ChipKIT boards with Arduino-compatible firmware (will not work on classic Arduino or pure PIC32 chip).

to install, just make git clone https://github.com/1i7/stepper_h to ~/Arduino/libraries (for the new ChipKIT IDE)

```bash
cd ~/Arduino/libraries
git clone https://github.com/1i7/stepper_h.git
```

basic example should appear in Arduino examples menu: File/Examples/stepper_h/stepper_h

This one runs 3 stepper motors with different speed at the same time. Motors start to run after calling start_stepper_cycle(). Note, that loop can contain any code (or have no code at all) - the motors would work in background.
```c++
#include "stepper.h"

// Stepper motors
static stepper sm_x, sm_y, sm_z;

static void prepare_line1() {
    //prepare_steps(stepper *smotor, int step_count, int step_delay, stepper_info_t *stepper_info=NULL);

    // make 20000 steps with 1000 microseconds delay
    // X.pos would go from 0 to 7.5*20000=150000micrometers (150millimeters)
    // during 1000*20000=20000000microseconds=20seconds
    prepare_steps(&sm_x, 20000, 1000);
    // make 10000 steps with 2000 microseconds delay
    // Y.pos would go from 0 to 7.5*10000=75000micrometers (75millimeters)
    // during 2000*10000=20000000microseconds=20seconds
    prepare_steps(&sm_y, 10000, 2000);
    // make 1000 steps with 20000 microseconds delay
    // Z.pos would go from 0 to 7.5*1000=7500micrometers (7.5millimeters)
    // during 20000*1000=20000000microseconds=20seconds
    prepare_steps(&sm_z, 1000, 20000);
}

void setup() {
    Serial.begin(9600);   
    Serial.println("Starting stepper_h test...");
   
    // connected stepper motors
    // init_stepper(stepper* smotor,  char* name,
    //     int pin_step, int pin_dir, int pin_en,
    //     int dir_inv, int pulse_delay,
    //     float distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     double min_pos, double max_pos);
   
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, 1, 1000, 7.5);
    init_stepper_ends(&sm_x, -1, -1, CONST, CONST, 0, 300000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, -1, 1000, 7.5);
    init_stepper_ends(&sm_y, -1, -1, CONST, CONST, 0, 216000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, -1, 1000, 7.5);
    init_stepper_ends(&sm_z, -1, -1, CONST, CONST, 0, 100000);
     
    // configure motors before starting steps
    prepare_line1();
    // start motors, non-blocking
    start_stepper_cycle();
}

void loop() {
    static int prevTime = 0;
    // Debug messages - print current positions of motors once per second
    // while they are rotating, once per 10 seconds when they are stopped
    int currTime = millis();
    if( (is_stepper_cycle_running() && (currTime - prevTime) >= 1000) || (currTime - prevTime) >= 10000 ) {
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
```

even if you don't have stepper motor with step-dir driver, you can check out output in the serial monitor window (this one goes from my ChipKIT Uno32)

```
Starting stepper_h test...
X.pos=6937.5000000000, Y.pos=3555.0000000000, Z.pos=360.0000000000
X.pos=14437.5000000000, Y.pos=7312.5000000000, Z.pos=735.0000000000
X.pos=21930.0000000000, Y.pos=11055.0000000000, Z.pos=1110.0000000000
X.pos=29430.0000000000, Y.pos=14805.0000000000, Z.pos=1485.0000000000
X.pos=36922.5000000000, Y.pos=18555.0000000000, Z.pos=1860.0000000000
X.pos=44422.5000000000, Y.pos=22305.0000000000, Z.pos=2235.0000000000
X.pos=51915.0000000000, Y.pos=26047.5000000000, Z.pos=2610.0000000000
X.pos=59415.0000000000, Y.pos=29797.5000000000, Z.pos=2985.0000000000
X.pos=66907.5000000000, Y.pos=33547.5000000000, Z.pos=3360.0000000000
X.pos=74407.5000000000, Y.pos=37297.5000000000, Z.pos=3735.0000000000
X.pos=81900.0000000000, Y.pos=41040.0000000000, Z.pos=4110.0000000000
X.pos=89400.0000000000, Y.pos=44790.0000000000, Z.pos=4485.0000000000
X.pos=96892.5000000000, Y.pos=48540.0000000000, Z.pos=4860.0000000000
X.pos=104392.5000000000, Y.pos=52290.0000000000, Z.pos=5235.0000000000
X.pos=111885.0000000000, Y.pos=56040.0000000000, Z.pos=5610.0000000000
X.pos=119385.0000000000, Y.pos=59790.0000000000, Z.pos=5985.0000000000
X.pos=126877.5000000000, Y.pos=63532.5000000000, Z.pos=6360.0000000000
X.pos=134377.5000000000, Y.pos=67282.5000000000, Z.pos=6735.0000000000
X.pos=141870.0000000000, Y.pos=71032.5000000000, Z.pos=7110.0000000000
X.pos=149370.0000000000, Y.pos=74782.5000000000, Z.pos=7485.0000000000
X.pos=150000.0000000000, Y.pos=75000.0000000000, Z.pos=7500.0000000000
```

Actually, this lib has some more features like counting steps for each motor, tracking working tool virtual position, making steps with dynamic step delay (to draw curves) etc. Some of them are far from being finished, but some already work mostly fine. Will provide more examples if someone is interested here.

some older videos with older version of this stepper_h lib + ChipKIT + CNC

https://vimeo.com/133592759
https://vimeo.com/93176233
https://vimeo.com/93395529
