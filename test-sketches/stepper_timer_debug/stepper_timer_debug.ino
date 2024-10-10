#include "stepper.h"

#include "stepper_configure_timer.h"
extern "C"{
    #include "timer_setup.h"
}

// Stepper motors
static stepper sm_x, sm_y, sm_z;

/////////////////////////////////////////////////////
// настройки таймера
unsigned long _timer_period_us;

void configure_timer() {

    // 
    // для периода 1 микросекунда (1млн вызовов в секунду == 1МГц):
    // 
    // - PIC32MX/ChipKIT (вообще не ок)
    // 1 мотор:
    //   ***ERROR: timer handler takes longer than timer period: cycle time=3us, timer period=1us
    // 
    // - AVR/Arduino (вообще не ок):
    // 1 мотор (не ок: снос крыши):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=1us
    
    //_timer_period_us = stepper_configure_timer_1MHz(TIMER_DEFAULT);
    

    // 
    // для периода 5 микросекунд (200тыс вызовов в секунду == 200КГц):
    // 
    // - PIC32MX/ChipKIT (вообще не ок)
    // 1 мотор:
    //   ***ERROR: timer handler takes longer than timer period: cycle time=5us, timer period=5us
    // 2 мотора:
    //   ***ERROR: timer handler takes longer than timer period: cycle time=5us, timer period=5us
    // 
    // - AVR/Arduino (вообще не ок):
    // 1 мотор (не ок: снос крыши):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=5us
    
    //_timer_period_us = stepper_configure_timer_200KHz(TIMER_DEFAULT);
    

    // 
    // для периода 10 микросекунд (100тыс вызовов в секунду == 100КГц):
    // 
    // - PIC32MX/ChipKIT (наименьший период для 2х моторов)
    // 1 мотор (ок):
    //   Finished cycle, max time=4
    // 2 мотора (ок):
    //   Finished cycle, max time=7
    // 3 мотора (не ок):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=10us, timer period=10us
    // 
    // - SAM/Arduino Due (вообще не ок)
    // 1 мотор (не ок: шагает норм, но слетает, похоже, на последнем шаге):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=10us, timer period=10us
    // 2 мотора (не ок):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=11us, timer period=10us
    // 3 мотора (не ок):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=12us, timer period=10us
    // 
    // - AVR/Arduino (вообще не ок):
    // 1 мотор (не ок: снос крыши):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=10us
    // 2 мотора (не ок: снос крыши):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=10us
    
    //_timer_period_us = stepper_configure_timer_100KHz(TIMER_DEFAULT);

    
    // 
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    //
    // - PIC32MX/ChipKIT (наименьший период для 3х моторов)
    // 2 мотора (тем более ок):
    //   Finished cycle, max time=9
    // 3 мотора (ок):
    //   Finished cycle, max time=11
    // дуга-2 мотора (не ок)
    //   совсем не ок для движения по дуге (по 90мкс на acos/asin)
    //
    // - SAM/Arduino Due (наименьший период для 1, 2х и 3х моторов)
    // 1 мотор (ок):
    //   Finished cycle, max time=9
    // 2 мотора (ок):
    //   Finished cycle, max time=13
    // 3 мотора (ок):
    //   Finished cycle, max time=16
    // 
    // - AVR/Arduino (вообще не ок):
    // 1 мотор (не ок: снос крыши)
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=20us
    // 2 мотора (не ок: снос крыши):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=4294966284us, timer period=20us
    //   cycle time=4294966284us - это почти 2^32(=4294967296-4294966284=1012 - разница 1 миллисекунда).
    // Довольно странное значение (по времени это 72 минуты), в stepper_timer.cpp это переменная _cycle_max_time,
    // по коду не очень понятно, как в нее могло попасть это число.
    // Двоичное представление: b11111111_11111111_11111100_00001100
    // Пока что проще всего списать на некий глюк, который будет происходить на контроллере,
    // если обработчик прерывания не укладывается в период между тиками таймера.
    // Если присваивать _cycle_max_time константу, то мусор в переменной не появляется,
    // значит проблема возникает где-то вот в этих 3х строках:
    // 
    //   unsigned long cycle_start = micros();
    //   ...
    //   unsigned long cycle_finish = micros();
    //   unsigned long cycle_time = cycle_finish - cycle_start;
    // после небольшого дебага получили максимальные значения замеров:
    // cycle_start(max) = 5003256
    // cycle_finish(max) = 5003256
    // Возможно, в один из моментов получается, что значение start как-то получется
    // даже больше, чем финиш, в _cycle_max_time появляется отрицательное значение,
    // которое из-за unsigned, превращается в большое положительное.
    
    _timer_period_us = stepper_configure_timer_50KHz(TIMER_DEFAULT);


    // 
    // для периода 50 микросекунд (20тыс вызовов в секунду == 20КГц):
    // 
    // - AVR/Arduino (наименьший период для 1 мотора):
    // 1 мотор (ок):
    //   Finished cycle, max time=40
    // 2 мотора (не ок):
    //   ***ERROR: timer handler takes longer than timer period: cycle time=60us, timer period=50us
    //   (хотя в цикл не умещается, но уже по крайней мере значение корректно)
    
    //_timer_period_us = stepper_configure_timer_20KHz(TIMER_DEFAULT);


    // 
    // для периода 100 микросекунд (10тыс вызовов в секунду == 10КГц):
    // 
    // - AVR/Arduino (наименьший период для 2х моторов):
    // 2 мотора (ок)
    //   Finished cycle, max time=72
    // 3 мотора (не ок)
    //   ***ERROR: timer handler takes longer than timer period: cycle time=100us, timer period=100us
    
    //_timer_period_us = stepper_configure_timer_10KHz(TIMER_DEFAULT);

    // 
    // для периода 200 микросекунд (5тыс вызовов в секунду == 5КГц):
    // 
    // - PIC32MX/ChipKIT
    //   ок для движения по дуге (по 90мкс на acos/asin)
    // 
    // - AVR/Arduino (наименьший период для 3х моторов):
    // 2 мотора (ок)
    //   Finished cycle, max time=72
    // 3 мотора (ок)
    //   Finished cycle, max time=104
    
    //_timer_period_us = stepper_configure_timer_5KHz(TIMER_DEFAULT);
    

    ////////////////
    // 
    // произвольные значения
    //int _timer_id = TIMER_DEFAULT;
    //int _timer_prescaler = TIMER_PRESCALER_1_8;
    //unsigned int _timer_adjustment = 200;
    //_timer_period_us = 20;
    //stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);
}

////////////////////////////////////////////////
// Минимальная задержка между шагами мотора
// Вариант с индивидуальными задержками для разных моторов

// минимальная задержка между шагами для разных делителей шага
// на разных платформах

// PIC32MX/ChipKIT
// для частоты 50КГц (период=20мкс - достаточно для вращения одновременно 3х моторов)
// step_delay >= period*3, step_delay делится на period без остатка
//    1/1: 1500 -> 1500 мкс/шаг (макс скорость)
//    1/2: 650 -> 660 мкс/шаг (макс скорость)
//    1/4: 330 -> 340 мкс/шаг (макс скорость)
//    1/8: 180 -> 180 мкс/шаг (макс скорость)
//    1/16: 80 -> 80 мкс/шаг (макс скорость)
//    1/32: 30/40 -> 60 мкс/шаг (1/2 макс скорости)

unsigned long _step_delay_1_1_pic32mx = 1500;
unsigned long _step_delay_1_2_pic32mx = 660;
unsigned long _step_delay_1_4_pic32mx = 340;
unsigned long _step_delay_1_8_pic32mx = 180;
unsigned long _step_delay_1_16_pic32mx = 80;
unsigned long _step_delay_1_32_pic32mx = 60;


// AVR/Arduino
// для частоты 5КГц (период=200мкс - достаточно для вращения одновременно 3х моторов)
// step_delay >= period*3, step_delay делится на period без остатка
//    1/1: 1500 -> 1600 мкс/шаг (~макс скорость)
//    1/2: 650 -> 800 мкс/шаг (~макс скорость)
//    1/4: 330 -> 600 мкс/шаг (~1/2 макс скорости)
//    1/8: 180 -> 600 мкс/шаг (~1/3 макс скорости)
//    1/16: 80 -> 600 мкс/шаг (~1/8 макс скорости)
//    1/32: 30/40 -> 600 мкс/шаг (1/20 макс скорости)

unsigned long _step_delay_1_1_avr = 1600;
unsigned long _step_delay_1_2_avr = 800;
unsigned long _step_delay_1_4_avr = 600;
unsigned long _step_delay_1_8_avr = 600;
unsigned long _step_delay_1_16_avr = 600;
unsigned long _step_delay_1_32_avr = 600;

// SAM/Arduino Due
// для частоты 50КГц (период=20мкс - наименьший вариант для вращения одновременно 1, 2х и 3х моторов)
// step_delay >= period*3, step_delay делится на period без остатка
//    1/1: 1500 -> 1500 мкс/шаг (макс скорость)
//    1/2: 650 -> 660 мкс/шаг (макс скорость)
//    1/4: 330 -> 340 мкс/шаг (макс скорость)
//    1/8: 180 -> 180 мкс/шаг (макс скорость)
//    1/16: 80 -> 80 мкс/шаг (макс скорость)
//    1/32: 30/40 -> 60 мкс/шаг (1/2 макс скорости)

unsigned long _step_delay_1_1_sam = 1500;
unsigned long _step_delay_1_2_sam = 660;
unsigned long _step_delay_1_4_sam = 340;
unsigned long _step_delay_1_8_sam = 180;
unsigned long _step_delay_1_16_sam = 80;
unsigned long _step_delay_1_32_sam = 60;

//
// расстояние за шаг
// Возьмем шкив Берем шкив GT2 (20 зубов, шаг зуба 2мм)
// за полный оборот мотор переместит L=2мм*20зубов=40мм=40000мкм длины ремня.
//    1/1: 200 шагов/оборот, шаг=40000мкм/200 = 200 мкм = 200000 нм
//    1/2: 200 * 2 = 400 шагов/оборот, шаг=40000мкм/400 = 100 мкм = 100000 нм
//    1/4: 200 * 4 = 800 шагов/оборот, шаг=40000мкм/800 = 50 мкм = 50000 нм
//    1/8: 200 * 8 = 1600 шагов/оборот, шаг=40000мкм/1600 = 25 мкм = 25000 нм
//    1/16: 200 * 16 = 3200 шагов/оборот, шаг=40000мкм/3200 = 12.5 мкм = 12500 нм
//    1/32: 300 * 32 = 6400 шагов/оборот, шаг=40000мкм/6400 = 6.25 мкм = 6250 нм

unsigned long _dist_per_step_1_1 = 200000;
unsigned long _dist_per_step_1_2 = 100000;
unsigned long _dist_per_step_1_4 = 50000;
unsigned long _dist_per_step_1_8 = 25000;
unsigned long _dist_per_step_1_16 = 12500;
unsigned long _dist_per_step_1_32 = 6250;


// X

// 1/1
//unsigned long x_min_step_delay_us = _step_delay_1_1_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_1_avr;
unsigned long x_min_step_delay_us = _step_delay_1_1_sam;
unsigned long x_dist_per_step = _dist_per_step_1_1;

// 1/2
//unsigned long x_min_step_delay_us = _step_delay_1_2_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_2_avr;
//unsigned long x_min_step_delay_us = _step_delay_1_2_sam;
//unsigned long x_dist_per_step = _dist_per_step_1_2;

// 1/4
//unsigned long x_min_step_delay_us = _step_delay_1_4_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_4_avr;
//unsigned long x_min_step_delay_us = _step_delay_1_4_sam;
//unsigned long x_dist_per_step = _dist_per_step_1_4;

// 1/8
//unsigned long x_min_step_delay_us = _step_delay_1_8_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_8_avr;
//unsigned long x_dist_per_step = _dist_per_step_1_8;

// 1/16
//unsigned long x_min_step_delay_us = _step_delay_1_16_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_16_avr;
//unsigned long x_dist_per_step = _dist_per_step_1_16;

// 1/32
//unsigned long x_min_step_delay_us = _step_delay_1_32_pic32mx;
//unsigned long x_min_step_delay_us = _step_delay_1_32_avr;
//unsigned long x_min_step_delay_us = _step_delay_1_32_sam;
//unsigned long x_dist_per_step = _dist_per_step_1_32;

// Y

// 1/1
//unsigned long y_min_step_delay_us = _step_delay_1_1_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_1_avr;
unsigned long y_min_step_delay_us = _step_delay_1_1_sam;
unsigned long y_dist_per_step = _dist_per_step_1_1;

// 1/2
//unsigned long y_min_step_delay_us = _step_delay_1_2_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_2_avr;
//unsigned long y_min_step_delay_us = _step_delay_1_2_sam;
//unsigned long y_dist_per_step = _dist_per_step_1_2;

// 1/4
//unsigned long y_min_step_delay_us = _step_delay_1_4_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_4_avr;
//unsigned long y_min_step_delay_us = _step_delay_1_4_sam;
//unsigned long y_dist_per_step = _dist_per_step_1_4;

// 1/8
//unsigned long y_min_step_delay_us = _step_delay_1_8_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_8_avr;
//unsigned long y_min_step_delay_us = _step_delay_1_8_sam;
//unsigned long y_dist_per_step = _dist_per_step_1_8;

// 1/16
//unsigned long y_min_step_delay_us = _step_delay_1_16_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_16_avr;
//unsigned long y_min_step_delay_us = _step_delay_1_16_sam;
//unsigned long y_dist_per_step = _dist_per_step_1_16;

// 1/32
//unsigned long y_min_step_delay_us = _step_delay_1_32_pic32mx;
//unsigned long y_min_step_delay_us = _step_delay_1_32_avr;
//unsigned long y_min_step_delay_us = _step_delay_1_32_sam;
//unsigned long y_dist_per_step = _dist_per_step_1_32;

// Z

// 1/1
//unsigned long z_min_step_delay_us = _step_delay_1_1_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_1_avr;
unsigned long z_min_step_delay_us = _step_delay_1_1_sam;
unsigned long z_dist_per_step = _dist_per_step_1_1;

// 1/2
//unsigned long z_min_step_delay_us = _step_delay_1_2_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_2_avr;
//unsigned long z_min_step_delay_us = _step_delay_1_2_sam;
//unsigned long z_dist_per_step = _dist_per_step_1_2;

// 1/4
//unsigned long z_min_step_delay_us = _step_delay_1_4_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_4_avr;
//unsigned long z_min_step_delay_us = _step_delay_1_4_sam;
//unsigned long z_dist_per_step = _dist_per_step_1_4;

// 1/8
//unsigned long z_min_step_delay_us = _step_delay_1_8_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_8_avr;
//unsigned long z_min_step_delay_us = _step_delay_1_8_sam;
//unsigned long z_dist_per_step = _dist_per_step_1_8;

// 1/16
//unsigned long z_min_step_delay_us = _step_delay_1_16_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_16_avr;
//unsigned long z_min_step_delay_us = _step_delay_1_16_sam;
//unsigned long z_dist_per_step = _dist_per_step_1_16;

// 1/32
//unsigned long z_min_step_delay_us = _step_delay_1_32_pic32mx;
//unsigned long z_min_step_delay_us = _step_delay_1_32_avr;
//unsigned long z_min_step_delay_us = _step_delay_1_32_sam;
//unsigned long z_dist_per_step = _dist_per_step_1_32;

// 1 мотор с максимальной скоростью
static void prepare_line1() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    
    // шагаем с максимальной скоростью
    prepare_steps(&sm_x, 10000, x_min_step_delay_us);
}

// 2 мотора с максимальной скоростью
static void prepare_line2() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    
    // шагаем с максимальной скоростью
    prepare_steps(&sm_x, 10000, x_min_step_delay_us);
    prepare_steps(&sm_y, 10000, y_min_step_delay_us);
}

// 3 мотора с максимальной скоростью
static void prepare_line3() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    
    // шагаем с максимальной скоростью
    prepare_steps(&sm_x, 10000, x_min_step_delay_us);
    // вызвать CYCLE_ERROR_MOTOR_ERROR
    //prepare_steps(&sm_x, 200000, x_step_delay_us-1);
    prepare_steps(&sm_y, 10000, y_min_step_delay_us);
    prepare_steps(&sm_z, 10000, z_min_step_delay_us);
}

// 2 мотора с максимальной скоростью
static void prepare_whirl2() {
    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    
    // шагаем с максимальной скоростью
    prepare_whirl(&sm_x, 1, x_min_step_delay_us);
    prepare_whirl(&sm_y, 1, y_min_step_delay_us);
}

// 2 мотора: y с переменной скоростью из буфера, x с постоянной скоростью
static void prepare_buffered2() {
    // void prepare_buffered_steps(stepper *smotor,
    //    int buf_size, unsigned long* delay_buffer, long* step_buffer)
 
    static unsigned long delay_buffer[3];
    static long step_buffer[3];

    delay_buffer[0] = y_min_step_delay_us;
    delay_buffer[1] = y_min_step_delay_us*10;
    delay_buffer[2] = y_min_step_delay_us*2;

    step_buffer[0] = 200*10;
    step_buffer[1] = -200*5;
    step_buffer[2] = 200*2;
    
    prepare_buffered_steps(&sm_y, 3, delay_buffer, step_buffer);

    // для икса просто шаги
    prepare_steps(&sm_x, 200000, x_min_step_delay_us);
}

/**
 * Напечатать ошибку цикла
 */
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

/**
 * Напечатать ошибку мотора
 */
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
    while(!Serial) // hack to see 1st messages in serial monitor on Arduino Leonardo
    Serial.println("Starting stepper_h test...");
    
    // connected stepper motors
    // init_stepper(stepper* smotor, char name,
    //     int pin_step, int pin_dir, int pin_en,
    //     bool invert_dir, unsigned long min_step_delay,
    //     unsigned long distance_per_step)
    // init_stepper_ends(stepper* smotor,
    //     end_strategy min_end_strategy, end_strategy max_end_strategy,
    //     long long min_pos, long long max_pos);
    
    // Pinout for CNC-shield

    // ChipKIT PIC32:
    // Минимальный период (для максимальной скорости) шага
    // при периоде таймера 20 микросекунд (лучший вариант для
    // движения по линии 3 мотора одновременно)
    // step_delay=60 микросекунд (20*3)
    
    // X
    
    init_stepper(&sm_x, 'x', 2, 5, 8, false, x_min_step_delay_us, x_dist_per_step);
    //init_stepper(&sm_x, 'x', 2, 5, 8, false, x_min_step_delay_us, 750);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 30000000000);
    
    // Y
    init_stepper(&sm_y, 'y', 3, 6, 8, false, y_min_step_delay_us, y_dist_per_step);
    //init_stepper(&sm_y, 'y', 3, 6, 8, false, y_step_delay_us, 750);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, CONST, CONST, 0, 21600000000);
    
    // Z
    init_stepper(&sm_z, 'z', 4, 7, 8, false, z_min_step_delay_us, z_dist_per_step);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, CONST, 0, 10000000000);

    // настройки таймера
    configure_timer();
    
    // configure motors before starting steps
    //prepare_line1();
    //prepare_line2();
    prepare_line3();
    //prepare_whirl2();
    //prepare_buffered2();
    
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
        Serial.print(", Y.pos=");
        Serial.print(sm_y.current_pos, DEC);
        Serial.print(", Z.pos=");
        Serial.print(sm_z.current_pos, DEC);
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
        Serial.print("Y: ");
        print_motor_error(sm_y);
        Serial.println();
        Serial.print("Z: ");
        print_motor_error(sm_z);
        Serial.println();
        
        print_once = false;
    }
    
    // put any code here, it would run while the motors are rotating
}


