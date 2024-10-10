#include "stepper.h"

extern "C"{
    #include "timer_setup.h"
}

// Stepper motors
static stepper sm_x;

// CNC-shield
// http://blog.protoneer.co.nz/arduino-cnc-shield/
// X
//#define STEP_PIN 2
//#define DIR_PIN 5
//#define EN_PIN 8

// Y
#define STEP_PIN 3
#define DIR_PIN 6
#define EN_PIN 8

// Z
//#define STEP_PIN 4
//#define DIR_PIN 7
//#define EN_PIN 8

// период таймера в микросекундах
int _timer_period_us;

// полный оборот с делителем шага 1/1 (без делителя)
static void prepare_test_1_1(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/1 (без деления шага)
    // с периодом таймера 20мкс
    // шагов в полном обороте: 200
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/200 = 200000 нм
    // период таймера: 20 мкс
    // задержка между шагами: 1500 мкс
    // тиков таймера на шаг: 1500мкс/20мкс=75
    // тиков таймера на оборот: 75*200=15000
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 1500; // us
    unsigned long _dist_per_step = 200000; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 200;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
    //prepare_steps(&sm_x, _step_count*turns, 10000000/200); // весь цикл за 10 секунд
}

// полный оборот с делителем шага 1/2
static void prepare_test_1_2(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/2
    // с периодом таймера 20мкс
    // шагов в полном обороте: 200*2=400
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/400 = 100000 нм
    // период таймера: 20 мкс
    // задержка между шагами (кратно 20): 660 мкс
    // тиков таймера на шаг: 660мкс/20мкс=33
    // тиков таймера на оборот: 33*400=13200
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 660; // us
    unsigned long _dist_per_step = 100000; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 400;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
}

// полный оборот с делителем шага 1/4
static void prepare_test_1_4(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/4
    // с периодом таймера 20мкс
    // шагов в полном обороте: 200*4=800
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/800 = 50000 нм
    // период таймера: 20 мкс
    // задержка между шагами (кратно 20): 340 мкс
    // тиков таймера на шаг: 340мкс/20мкс=17
    // тиков таймера на оборот: 17*800=13600
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 340; // us
    unsigned long _dist_per_step = 50000; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 800;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
}

// полный оборот с делителем шага 1/8
static void prepare_test_1_8(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/8
    // с периодом таймера 20мкс
    // шагов в полном обороте: 200*8=1600
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/1600 = 25000 нм
    // период таймера: 20 мкс
    // задержка между шагами (кратно 20): 180 мкс
    // тиков таймера на шаг: 180мкс/20мкс=9
    // тиков таймера на оборот: 9*1600=14400
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 180; // us
    unsigned long _dist_per_step = 25000; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 1600;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
}

// полный оборот с делителем шага 1/16
static void prepare_test_1_16(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/16
    // с периодом таймера 20мкс
    // шагов в полном обороте: 200*16=3200
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/3200 = 12500 нм
    // период таймера: 20 мкс
    // задержка между шагами (кратно 20): 80 мкс
    // тиков таймера на шаг: 80мкс/20мкс=4
    // тиков таймера на оборот: 4*3200=12800
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 80; // us
    unsigned long _dist_per_step = 12500; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 3200;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
}

// полный оборот с делителем шага 1/32
static void prepare_test_1_32(int turns) {
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    ///////////
    // Шагаем полный оборот с делителем 1/32
    // с периодом таймера 20 мкс
    // шагов в полном обороте: 200*32=6400
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/6400 = 6250 нм
    // период таймера: 20 мкс
    // задержка между шагами: 60 мкс
    // тиков таймера на шаг: 60мкс/20мкс=3
    // тиков таймера на оборот: 3*6400=19200
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    int _timer_id = TIMER3;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    unsigned long _min_step_delay_us = 60; // us
    unsigned long _dist_per_step = 6250; // nm
    init_stepper(&sm_x, 'x', STEP_PIN, DIR_PIN, EN_PIN, false, _min_step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    long _step_count = 6400;
    prepare_steps(&sm_x, _step_count*turns, _min_step_delay_us);
}


void print_cycle_error(stepper_cycle_error_t err) {
    switch(err) {
        case CYCLE_ERROR_NONE:
            Serial.print("CYCLE_ERROR_NONE");
            break;
        case CYCLE_ERROR_TIMER_PERIOD_TOO_LONG:
            Serial.print("CYCLE_ERROR_TIMER_PERIOD_TOO_LONG");
            break;
        case CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_STEP_DELAY:
            Serial.print("CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_STEP_DELAY");
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

void print_motor_error(stepper &sm) {
    if(sm.error) {
        if(sm.error & STEPPER_ERROR_SOFT_END_MIN) {
            Serial.print("STEPPER_ERROR_SOFT_END_MIN");
        }
        if(sm.error & STEPPER_ERROR_SOFT_END_MAX) {
            Serial.print("STEPPER_ERROR_SOFT_END_MAX");
        }
        if(sm.error & STEPPER_ERROR_HARD_END_MIN) {
            Serial.print("STEPPER_ERROR_HARD_END_MIN");
        }
        if(sm.error & STEPPER_ERROR_HARD_END_MAX) {
            Serial.print("STEPPER_ERROR_HARD_END_MAX");
        }
        if(sm.error & STEPPER_ERROR_STEP_DELAY_SMALL) {
            Serial.print("STEPPER_ERROR_STEP_DELAY_SMALL");
        }
    } else {
        Serial.print("none");
    }
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting stepper_h test...");
    
    // configure motors before starting steps

    // step divider mode
    // (M0 M1 M2) -> (|||)
    
    // 1/1 (___)
    prepare_test_1_1(100);
    // 1/2 (|__)
    //prepare_test_1_2(1);
    // 1/4 (_|_)
    //prepare_test_1_4(1);
    // 1/8 (||_)
    //prepare_test_1_8(1);
    // 1/16 (__|)
    //prepare_test_1_16(1);
    // 1/32 (|||)
    //prepare_test_1_32(10);

    
    // start motors, non-blocking
    stepper_start_cycle();
}

void loop() {
    static unsigned long prevTime = 0;
    // Debug messages - print current positions of motors once per second
    // while they are rotating, once per 10 seconds when they are stopped
    // (see https://github.com/1i7/stepper_h/blob/master/3pty/arduino/README
    // to fix compile proplem)
    unsigned long currTime = millis();
    if( (stepper_cycle_running() && (currTime - prevTime) >= 1000) || (currTime - prevTime) >= 10000 ) {
        prevTime = currTime;
        Serial.print("X.pos=");
        Serial.print(sm_x.current_pos, DEC);
        Serial.println();
    }
    
    // обработчик таймера не умещается в период
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

        // ошибки цикла
        if(stepper_cycle_error()) {
            Serial.print("Cycle error: ");
            print_cycle_error(stepper_cycle_error());
            Serial.println();
        }
        
        // ошибки моторов
        Serial.println("Motor errors:");
        Serial.print("X: ");
        print_motor_error(sm_x);
        Serial.println();
        
        print_once = false;
    }
    
    // put any code here, it would run while the motors are rotating
}

