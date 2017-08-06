#include "stepper.h"

extern "C"{
    #include "timer_setup.h"
}

// Stepper motors
static stepper sm_x, sm_y, sm_z;

// настройки таймера

// для периода 1 микросекунда (1млн вызовов в секунду):
//   timer handler takes longer than timer period: cycle time=3us, timer period=1us
//int _timer_period_us = 1;
//int _timer = TIMER3; 
//int _timer_prescaler = TIMER_PRESCALER_1_8;
//int _timer_period = 10;

// для периода 5 микросекунд (200тыс вызовов в секунду):
//   timer handler takes longer than timer period: cycle time=5us, timer period=5us
//int _timer_period_us = 5;
//int _timer = TIMER3; 
//int _timer_prescaler = TIMER_PRESCALER_1_8;
//int _timer_period = 50;


// для периода 10 микросекунд (100тыс вызовов в секунду):
// На ChipKIT Uno32 
// 2 мотора (ок):
//   Finished cycle, max time=9
// 3 мотора (не ок):
//   timer handler takes longer than timer period: cycle time=10us, timer period=10us
//int _timer_period_us = 10;
//int _timer = TIMER3; 
//int _timer_prescaler = TIMER_PRESCALER_1_8;
//int _timer_period = 100;

// для периода 20 микросекунд (50тыс вызовов в секунду):
// На ChipKIT Uno32 наименьший вариант ок для движения по линии
// 3 мотора (ок):
//   Finished cycle, max time=11
// 2 мотора (тем более ок):
//   Finished cycle, max time=9
// совсем не ок для движения по дуге (по 90мкс на acos/asin)
int _timer_period_us = 20;
int _timer = TIMER3; 
int _timer_prescaler = TIMER_PRESCALER_1_8;
int _timer_period = 200;

//// для периода 200 микросекунд (5тыс вызовов в секунду):
//int _timer_period_us = 200;
//int _timer = TIMER3; 
//int _timer_prescaler = TIMER_PRESCALER_1_8;
//int _timer_period = 2000;

static void prepare_line3() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode,
    //     stepper_info_t *stepper_info=NULL);

    prepare_steps(&sm_x, 200000, 60);
    prepare_steps(&sm_y, 200000, 60);
    prepare_steps(&sm_z, 200000, 60);
}

void print_cycle_error(stepper_cycle_error_t err) {
    switch(err) {
        case CYCLE_ERROR_NONE:
            Serial.print("CYCLE_ERROR_NONE");
            break;
        case CYCLE_ERROR_TIMER_PERIOD_TOO_LONG:
            Serial.print("CYCLE_ERROR_TIMER_PERIOD_TOO_LONG");
            break;
        case CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_MOTOR_PULSE:
            Serial.print("CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_MOTOR_PULSE");
            break;
        case CYCLE_ERROR_MOTOR_ERROR:
            Serial.print("CYCLE_ERROR_MOTOR_ERROR");
            break;
        case CYCLE_ERROR_HANDLER_TIMING_EXCEEDED:
            Serial.print("CYCLE_ERROR_HANDLER_TIMING_EXCEEDED");
            break;
        default:
            break;
    }
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

    // Минимальный период (для максимальной скорости) шага
    // при периоде таймера 20 микросекунд (лучший вариант для
    // движения по линии 3 мотора одновременно)
    // step_delay=60 микросекунд (20*3)
    
    // X
    init_stepper(&sm_x, 'x', 2, 5, 8, false, 60, 750);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 3, 6, 8, false, 60, 750);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, CONST, CONST, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 4, 7, 8, false, 60, 750);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, CONST, 0, 100000000);

    // настройки таймера
    stepper_configure_timer(_timer_period_us, _timer, _timer_prescaler, _timer_period);
    
    // configure motors before starting steps
    prepare_line3();
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

    // ошибки цикла
    if(stepper_cycle_error_status()) {
        print_cycle_error(stepper_cycle_error_status());
        Serial.println();
    }
    
    unsigned long cycle_time = stepper_cycle_max_time();
    if(cycle_time >= _timer_period_us) {
        Serial.print("***ERROR: timer handler takes longer than timer period: ");
        Serial.print("cycle time=");
        Serial.print(cycle_time);
        Serial.print("us, timer period=");
        Serial.print(_timer_period_us);
        Serial.println("us");
    }
    
    // напечатаем максимальное время цикла один раз после завершения
    static bool print_once = true;
    if(print_once && !stepper_cycle_running()) {
        Serial.print("Finished cycle, max time=");
        Serial.println(cycle_time);
        print_once = false;
    }
    
    // put any code here, it would run while the motors are rotating
}

