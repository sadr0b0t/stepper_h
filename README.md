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
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);

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
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, int step_delay,
    //     int distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long min_pos, long max_pos);
    
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, CONST, CONST, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 1000, 7500);
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
    if( (stepper_cycle_running() && (currTime - prevTime) >= 1000) || (currTime - prevTime) >= 10000 ) {
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
X.pos=6937500, Y.pos=3525000, Z.pos=352500
X.pos=14437500, Y.pos=7275000, Z.pos=727500
X.pos=21930000, Y.pos=11025000, Z.pos=1102500
X.pos=29430000, Y.pos=14775000, Z.pos=1477500
X.pos=36922500, Y.pos=18525000, Z.pos=1852500
X.pos=44422500, Y.pos=22267500, Z.pos=2227500
X.pos=51915000, Y.pos=26017500, Z.pos=2602500
X.pos=59415000, Y.pos=29767500, Z.pos=2977500
X.pos=66907500, Y.pos=33517500, Z.pos=3352500
X.pos=74407500, Y.pos=37260000, Z.pos=3727500
X.pos=81900000, Y.pos=41010000, Z.pos=4102500
X.pos=89400000, Y.pos=44760000, Z.pos=4477500
X.pos=96892500, Y.pos=48510000, Z.pos=4852500
X.pos=104392500, Y.pos=52260000, Z.pos=5227500
X.pos=111885000, Y.pos=56010000, Z.pos=5602500
X.pos=119385000, Y.pos=59752500, Z.pos=5977500
X.pos=126877500, Y.pos=63502500, Z.pos=6352500
X.pos=134377500, Y.pos=67252500, Z.pos=6727500
X.pos=141870000, Y.pos=71002500, Z.pos=7102500
X.pos=149370000, Y.pos=74745000, Z.pos=7477500
X.pos=150000000, Y.pos=75000000, Z.pos=7500000
```

Actually, this lib has some more features like counting steps for each motor, tracking working tool virtual position, making steps with dynamic step delay (to draw curves) etc. Some of them are far from being finished, but some already work mostly fine. Will provide more examples if someone is interested here.

some older videos with older version of this stepper_h lib + ChipKIT + CNC

https://vimeo.com/133592759
https://vimeo.com/93176233
https://vimeo.com/93395529


---
# Внутреннее устройство

Код обработчика таймера handle_interrupts
https://github.com/1i7/stepper_h/blob/master/stepper_h/stepper_timer.cpp#L824

на одном шаге работает в 3 приема:
- первый проход (за 2 импульса до шага) - проверяет границы
- второй проход (за 1 импульс до шага) - взводит ножку step на HIGH
- третий проход (ипульс шага) - делает шаг (сбрасывает step в LOW)

на третьем же проходе сразу происходят проверки, нужно ли делать следующий шаг.

