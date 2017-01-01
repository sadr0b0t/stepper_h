#include "stepper.h"

// Stepper motors
static stepper sm_x, sm_y, sm_z;

static void prepare_line1() {
    // p0 -> p1 -> p2 -> p0
    //   line1 line2 line3
    // cm: (0,0,0) -> (15,5,2) -> (5,15,2) -> (0,0,0)
    // uM: (0,0,0) -> (150000,50000,20000) -> (50000,150000,20000) -> (0,0,0)
    // nm: (0,0,0) -> (150000000,50000000,20000000) -> (50000000,150000000,20000000) -> (0,0,0)
    
    // line1: (0,0,0) -> (150000000,50000000,20000000)
    
    // X - длинная координата - ее проходим с максимальной скоростью
    //int steps_x = 150000000 / 7500;
    int steps_x = (150000000 - sm_x.current_pos) / 7500;
    int delay_x = 1000;
    int time_x = delay_x * abs(steps_x);
    
    //int steps_y = 50000000 / 7500;
    int steps_y = (50000000 - sm_y.current_pos) / 7500;
    int delay_y = time_x / abs(steps_y);
    
    //int steps_z = 20000000 / 7500;
    int steps_z = (20000000 - sm_z.current_pos) / 7500;
    int delay_z = time_x / abs(steps_z);

    // prepare_steps(stepper *smotor, 
    //     int step_count, unsigned long step_delay, 
    //     calibrate_mode_t calibrate_mode, 
    //     stepper_info_t *stepper_info=NULL);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    prepare_steps(&sm_z, steps_z, delay_z);
}

static void prepare_line2() {
    // p0 -> p1 -> p2 -> p0
    //   line1 line2 line3
    // cm: (0,0,0) -> (15,5,2) -> (5,15,2) -> (0,0,0)
    // uM: (0,0,0) -> (150000,50000,20000) -> (50000,150000,20000) -> (0,0,0)
    // nm: (0,0,0) -> (150000000,50000000,20000000) -> (50000000,150000000,20000000) -> (0,0,0)
    
    // line2: (150000000,50000000,20000000) -> (50000000,150000000,20000000)
    
    // здесь путь по x и y одинаковый, но точные количества шагов
    // с учетом погрешностей округления могут отличаться на +/- один шаг:
    // реально получится:
    // steps_x=-13333, steps_y=13334
    
    //int steps_y = (150000000 - 50000000) / 7500;
    int steps_y = (150000000 - sm_y.current_pos) / 7500;
    int delay_y = 1000;
    int time_y = delay_y * abs(steps_y);
    
    //int steps_x = (50000000 - 150000000) / 7500;
    int steps_x = (50000000 - sm_x.current_pos) / 7500;
    int delay_x = time_y / abs(steps_y);
    
    // путь по z=0
    //int steps_z = (20000000 - 20000000) / 7500;
    //int steps_z = (20000000 - sm_z.current_pos) / 7500;
    //int delay_z = time_x / abs(steps_z);

    // prepare_steps(stepper *smotor, 
    //     int step_count, unsigned long step_delay, 
    //     calibrate_mode_t calibrate_mode, 
    //     stepper_info_t *stepper_info=NULL);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    //prepare_steps(&sm_z, steps_z, delay_z);
}

static void prepare_line3() {
    // p0 -> p1 -> p2 -> p0
    //   line1 line2 line3
    // cm: (0,0,0) -> (15,5,2) -> (5,15,2) -> (0,0,0)
    // uM: (0,0,0) -> (150000,50000,20000) -> (50000,150000,20000) -> (0,0,0)
    // nm: (0,0,0) -> (150000000,50000000,20000000) -> (50000000,150000000,20000000) -> (0,0,0)
    
    // line3: (50000000,150000000,20000000) -> (0,0,0)
    
    // Y - длинная координата - ее проходим с максимальной скоростью
    //int steps_y = -150000000 / 7500;
    int steps_y = (0 - sm_y.current_pos) / 7500;
    int delay_y = 1000;
    int time_y = delay_y * abs(steps_y);
    
    //int steps_x = -50000000 / 7500;
    int steps_x = (0 - sm_x.current_pos) / 7500;
    int delay_x = time_y / abs(steps_x);
    
    //int steps_z = -20000000 / 7500;
    int steps_z = (0 - sm_z.current_pos) / 7500;
    int delay_z = time_y / abs(steps_z);

    // prepare_steps(stepper *smotor, 
    // int step_count, unsigned long step_delay, 
    //     calibrate_mode_t calibrate_mode, 
    //     stepper_info_t *stepper_info=NULL);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    prepare_steps(&sm_z, steps_z, delay_z);
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
    
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, CONST, CONST, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 1000, 7500);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, CONST, 0, 100000000);
}



void loop() {
    static int next_line = 1;
    if(!stepper_is_cycle_running()) {
        // цикл завершился, запускаем следующую линию
        if(next_line == 1) {
            Serial.println("Prepare line1: (0,0,0) -> (150000000,50000000,20000000)");
            prepare_line1();
            next_line = 2;
        } else if(next_line == 2) {
            Serial.println("Prepare line2: (150000000,50000000,20000000) -> (50000000,150000000,20000000)");
            prepare_line2();
            next_line = 3;
        } else if(next_line == 3) {
            Serial.println("Prepare line3: (50000000,150000000,20000000) -> (0,0,0)");
            prepare_line3();
            next_line = 1;
        }

        // точные значения текущей позиции перед следующей линией
        Serial.print("X.pos=");
        Serial.print(sm_x.current_pos, DEC);
        Serial.print(", Y.pos=");
        Serial.print(sm_y.current_pos, DEC);
        Serial.print(", Z.pos=");
        Serial.print(sm_z.current_pos, DEC);
        Serial.println();
        
        // start motors, non-blocking
        stepper_start_cycle();
    }

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

