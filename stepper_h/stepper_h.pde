#include "stepper.h"

// Шаговые моторы
static stepper sm_x, sm_y, sm_z;

static void prepare_line1() {
    //prepare line: dx=30.0000000000, dy=20.0000000000, dt=8.0000000000
    //steps_x=4000, steps_y=2666
    //step_delay_x(1)=1000, step_delay_y(1)=2000
    //step_delay_x=1000, step_delay_y=2000
    //X.pos=15.0000000000, Y.pos=52.5000000000, Z.pos=0.0000000000
    //X.pos=3142.5000000000, Y.pos=2145.0000000000, Z.pos=0.0000000000
    //X.pos=6292.5000000000, Y.pos=4237.5000000000, Z.pos=0.0000000000
    //X.pos=9435.0000000000, Y.pos=6337.5000000000, Z.pos=0.0000000000
    //X.pos=12585.0000000000, Y.pos=8437.5000000000, Z.pos=0.0000000000
    //X.pos=15735.0000000000, Y.pos=10537.5000000000, Z.pos=0.0000000000
    //X.pos=18885.0000000000, Y.pos=12637.5000000000, Z.pos=0.0000000000
    //X.pos=22042.5000000000, Y.pos=14737.5000000000, Z.pos=0.0000000000
    //X.pos=25192.5000000000, Y.pos=16845.0000000000, Z.pos=0.0000000000
    //X.pos=28342.5000000000, Y.pos=18945.0000000000, Z.pos=0.0000000000
    //Finished motor=y: 10410
    //Finished motor=x: 10436
    prepare_line_2d(&sm_x, &sm_y, 30, 20, 8);
}

static void prepare_line2() {
    // 1142*18666+18666000=39982572
    // 14003*2666+2666000 =39997998
    // Finished motor=x: 50924
    // Finished motor=y: 50984
    prepare_line_2d(&sm_x, &sm_y, 140, 20, 40);
}

void setup() {
    Serial.println("Starting stepper_h test...");
    
    // информация о подключенных моторах
    // init_stepper(stepper* smotor,  char* name, 
    //     int pin_step, int pin_dir, int pin_en,
    //     int dir_inv, int pulse_delay,
    //     float distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     double min_pos, double max_pos);
    
    // X - синий драйвер
    init_stepper(&sm_x, 'x', 8, 9, 10, 1, 1000, 7.5); 
    init_stepper_ends(&sm_x, -1, -1, CONST, CONST, 0, 300000);
    // Y - желтый драйвер
    init_stepper(&sm_y, 'y', 5, 6, 7, -1, 1000, 7.5);
    init_stepper_ends(&sm_y, -1, -1, CONST, CONST, 0, 216000);
    // Z - черный драйвер
    init_stepper(&sm_z, 'z', 2, 3, 4, -1, 1000, 7.5);
    init_stepper_ends(&sm_z, -1, -1, CONST, CONST, 0, 100000);
      
        
    prepare_line1();
    start_stepper_cycle();
}

void loop() { 
    static int prevTime = 0;
    // Отладочные сообщение - печатаем текущую позицию печатающего блока
    int currTime = millis();
    if(is_cycle_running() && (currTime - prevTime) >= 1000) {
        prevTime = currTime;
        Serial.print("X.pos=");
        Serial.print(sm_x.current_pos, DEC);
        Serial.print(", Y.pos=");
        Serial.print(sm_y.current_pos, DEC);
        Serial.print(", Z.pos=");
        Serial.print(sm_z.current_pos, DEC);
        Serial.println();
    }
}

