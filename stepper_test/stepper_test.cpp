#include "stepper.h"

extern "C"{
    #include "timer_setup.h"
}

#include "Arduino.h"

//#include "stddef.h"
//#include "stdio.h"
//#include <iostream>

// http://www.use-strict.de/sput-unit-testing/tutorial.html

#if defined( __i386__ ) || defined ( __x86_64__ )
#include "sput.h"
#else
#include "sput-ino.h"
#endif

using namespace std;

/**
 * Симуляция таймера: "сгенерировать" нужное количество импульсов -
 * вызвать обработчик прерывания stepper_handle_interrupts нужное количество
 * раз.
 * @param count - количество тиков (импульсов) таймера
 */
void timer_tick(unsigned long count) {
    for(unsigned long i = 0; i < count; i++) {
        _timer_handle_interrupts(3);
    }
}


static void test_lifecycle() {
    // проверим жизненный цикл серии шагов:
    // цикл запущен/остановлен/на паузе и т.п.
    
    stepper sm_x, sm_y, sm_z;
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 1000, 7500);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, INF, INF, 0, 100000000);
    
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    /////////////////
    
    // запускаем все моторы на непрерывное вращение, т.к. границы рабочей
    // области у них не определены (INF), они будут вращаться до тех пор,
    // пока не будут остановлены специально
    // X - на максимальной скорости
    prepare_whirl(&sm_x, 1, 1000);
    // Y в два раза медленнее, чем X
    prepare_whirl(&sm_y, 1, 2000);
    // Z в три раза медленнее, чем X
    prepare_whirl(&sm_z, 1, 3000);
    
    // сначала цикл не запущен
    sput_fail_unless(!stepper_cycle_running(), "not started: stepper_cycle_running() == false");
    
    // теперь будет запущен
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "started: stepper_cycle_running() == true");
    
    // сделаем сколько-нибудь тиков таймера
    timer_tick(50000);
    // все еще должны двигаться
    sput_fail_unless(stepper_cycle_running(), "ticks: stepper_cycle_running() == true");
    
    // попробуем запустить повторно - новый цикл не должен запуститься
    sput_fail_unless(!stepper_start_cycle(), "start while running: stepper_start_cycle() == false");
    // но старый цикл все еще должен работать
    sput_fail_unless(stepper_cycle_running(), "start while running: stepper_cycle_running() == true");
    
    // встанем на паузу
    stepper_pause_cycle();
    timer_tick(50000);
    sput_fail_unless(stepper_cycle_running(), "paused: stepper_cycle_running() == true");
    sput_fail_unless(stepper_cycle_paused(), "paused: stepper_cycle_paused() == true");
    
    // продолжим двигаться
    stepper_resume_cycle();
    timer_tick(50000);
    sput_fail_unless(stepper_cycle_running(), "continued: stepper_cycle_running() == true");
    sput_fail_unless(!stepper_cycle_paused(), "continued: stepper_cycle_paused() == false");
    
    // останавливаемся
    stepper_finish_cycle();
    sput_fail_unless(!stepper_cycle_running(), "finished: stepper_cycle_running() == false");
    timer_tick(50000);
    sput_fail_unless(!stepper_cycle_running(), "finished: stepper_cycle_running() == false");
}

static void test_timer_period() {
    // проверим настройки периода таймера: цикл не должен запускаться,
    // если хотябы у одного из моторов минимальная задержка между шагами
    // не вмещает минимум 3 периода таймера
    
    stepper sm_x, sm_y, sm_z;
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 216000000);
    // Z - пусть у этого мотора аппаратная задержка между шагами
    // будет сильно меньше, чем у X и Y
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 500, 7500);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, INF, INF, 0, 100000000);
    
    unsigned long timer_period_us;
    
    // #1
    // период таймера 200 микросекунд
    // минимальная задержка между шагами моторов X и Y 1000 микросекунд
    // 200*3=600 < 1000 => цикл должен запуститься
    timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    prepare_whirl(&sm_x, 1, 1000);
    prepare_whirl(&sm_y, 1, 2000);
    //prepare_whirl(&sm_z, 1, 3000);
    
    stepper_start_cycle();
    // цикл должен запуститься
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_NONE,
        "period=200uS (ok): stepper_cycle_error() == CYCLE_ERROR_NONE");
    sput_fail_unless(stepper_cycle_running(), "period=200uS (ok): stepper_cycle_running() == true");
    // останавливаемся
    stepper_finish_cycle();
    
    // #2
    // период таймера 200 микросекунд
    // минимальная задержка между шагами моторов X и Y 1000 микросекунд,
    // для мотора Z - 500 микросекунд
    // 200*3=600 < 1000, но 600 > 500 => цикл НЕ должен запуститься
    timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    prepare_whirl(&sm_x, 1, 1000);
    prepare_whirl(&sm_y, 1, 2000);
    prepare_whirl(&sm_z, 1, 3000);
    
    stepper_start_cycle();
    // цикл не должен запуститься
    sput_fail_unless(!stepper_cycle_running(), "period=200uS (too long): stepper_cycle_running() == false");
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_TOO_LONG,
        "period=200uS (too long): stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_TOO_LONG");
    
    // #3
    // период таймера 350 микросекунд
    // минимальная задержка между шагами моторов X и Y 1000 микросекунд
    // 350*3=1050 > 1000 => цикл НЕ должен запуститься
    timer_period_us = 350;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 3500);
    
    prepare_whirl(&sm_x, 1, 1000);
    prepare_whirl(&sm_y, 1, 2000);
    //prepare_whirl(&sm_z, 1, 3000);
    
    stepper_start_cycle();
    // цикл не должен запуститься
    sput_fail_unless(!stepper_cycle_running(), "period=350uS (too long): stepper_cycle_running() == false");
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_TOO_LONG,
        "period=350uS (too long): stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_TOO_LONG");
    
    // #4: повторим тест #1 еще раз в конце для чистоты эксперимента
    // период таймера 200 микросекунд
    // минимальная задержка между шагами моторов X и Y 1000 микросекунд
    // 200*3=600 < 1000 => цикл должен запуститься
    timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    prepare_whirl(&sm_x, 1, 1000);
    prepare_whirl(&sm_y, 1, 2000);
    //prepare_whirl(&sm_z, 1, 3000);
    
    stepper_start_cycle();
    // цикл должен запуститься
    sput_fail_unless(stepper_cycle_running(), "period=200uS (ok): stepper_cycle_running() == true");
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_NONE,
        "period=200uS (ok): stepper_cycle_error() == CYCLE_ERROR_NONE");
    // останавливаемся
    stepper_finish_cycle();
}

static void test_timer_period_aliquant_step_delay() {
    // период таймера некратен минимальной задержке между шагами
    // одного из моторов. Это может привести к тому, что при движении
    // на максимальной скорости минимальная задержка меджу шагами
    // не будет соблюдаться, поэтому просто запретим такие комбинации:
    // см: https://github.com/1i7/stepper_h/issues/6
    
    
    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7500нм=7.5мкм
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 300;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 3000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    //////////////
    
    // готовим какие-то шаги
    prepare_steps(&sm_x, 500, 1000);
    
    // минимальная задержка между шагами мотора: 1000 мкс
    // период таймера: 300 мкс
    // остаток от деления: 1000 % 300 = 100 != 0 =>
    // цикл с такой частотой с таким мотором не должен запуститься
    stepper_start_cycle();
    sput_fail_unless(!stepper_cycle_running(), "period=300uS (aliquant): stepper_cycle_running() == false");
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_STEP_DELAY,
        "period=300uS (aliquant): stepper_cycle_error() == CYCLE_ERROR_TIMER_PERIOD_ALIQUANT_STEP_DELAY");
}

static void test_max_speed_tick_by_tick() {

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7500нм=7.5мкм
    stepper sm_x;
    int x_step = 8;
    init_stepper(&sm_x, 'x', x_step, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // проедем 3 шага с максимальной скоростью
    prepare_steps(&sm_x, 3, 1000);
    
    // разберем шаги по косточкам
    
    // период таймера 200 микросекунд кратен минимальной задержке между шагами
    // моторов 1000 микросекунд (в промежуток мерду шагами умещается ровно 5 тиков таймера),
    // поэтому максимальную скорость моторов не уменьшаем
    // см: https://github.com/1i7/stepper_h/issues/6
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    //
    // тики таймера (200 микросекунд) - каждый 5й порог совпадает с "идеальным" шагом:
    //                       шаг1                            шаг2                           шаг3
    // 200, 400, [600, 800, 1000], 1200, 1400, [1600, 1800, 2000], 2200, 2400, [2600, 2800 3000], 3200 ...
    // 1    2     3    4    5      6     7      8     9     10     11    12     13    14   15
    //
    // пороги реальных шагов совпадают с "идеальными"
    
    // а вот теперь поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    
    // шаг 1
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step1.tick1: pin_val == LOW");
    // взвести step
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 1, "step1.tick2: pin_val == HIGH");
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick2: current_pos == 0");
    //cout<<sm_x.current_pos<<endl;
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step1.tick3: pin_val == LOW");
    sput_fail_unless(sm_x.current_pos == 7500, "step1.tick3: current_pos == 7500");
    //cout<<sm_x.current_pos<<endl;
    
    // шаг 2
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step2.tick1: pin_val == LOW");
    // взвести step
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 1, "step2.tick2: pin_val == HIGH");
    sput_fail_unless(sm_x.current_pos == 7500, "step2.tick2: current_pos == 7500");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step2.tick3: pin_val == LOW");
    sput_fail_unless(sm_x.current_pos == 15000, "step2.tick3: current_pos == 15000");
    
    // шаг 3
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step3.tick1: pin_val == LOW");
    // взвести step
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 1, "step3.tick2: pin_val == HIGH");
    sput_fail_unless(sm_x.current_pos == 15000, "step3.tick2: current_pos == 15000");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(digitalRead(x_step) == 0, "step3.tick3: pin_val == LOW");
    sput_fail_unless(sm_x.current_pos == 22500, "step3.tick3: current_pos == 22500");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_cycle_running(), "step3.tick3: stepper_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 22500, "step3.tick3+1: current_pos == 22500");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
}

static void test_max_speed_30000steps() {

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5мкм (7500нм)
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // пройдем 30000 шагов с максимальной скоростью
    // это займет 30000*1000=30000000 микросекунд = 30 секунд
    // расстояние будет=7500*30000=225000000нм = 225000мкм = 225мм
    prepare_steps(&sm_x, 30000, 1000);
    
    // разберем шаги по косточкам
    
    // период таймера 200 микросекунд кратен минимальной задержке между шагами
    // моторов 1000 микросекунд (в промежуток мерду шагами умещается ровно 5 тиков таймера),
    // поэтому максимальную скорость моторов не уменьшаем
    // см: https://github.com/1i7/stepper_h/issues/6
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    //
    // тики таймера (200 микросекунд) - каждый 5й порог совпадает с "идеальным" шагом:
    //                       шаг1                            шаг2                           шаг3
    // 200, 400, [600, 800, 1000], 1200, 1400, [1600, 1800, 2000], 2200, 2400, [2600, 2800 3000], 3200 ...
    // 1    2     3    4    5      6     7      8     9     10     11    12     13    14   15
    //
    // пороги реальных шагов совпадают с "идеальными"
    
    // а вот теперь поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    
    // в одном шаге на максимальной скорости умещается ровно 5 тиков (импульсов) таймера,
    // чтобы пройти 30000 шагов, таймер должен тикнуть 5*30000+1=150001 раз
    // (последний тик завершает отработавшую серию)
    
    // шагаем 60000+90000+1=150001 тиков таймера
    
    // проверим положение на половине пути
    // 60000/5=12000 шагов, путь=7.5*12000=90000 мкм
    timer_tick(60000);
    sput_fail_unless(sm_x.current_pos == 90000000, "step30K.tick60K: current_pos == 90000000");
    
    // шагаем оставшиеся шаги
    timer_tick(90000);
    sput_fail_unless(sm_x.current_pos == 225000000, "step30K.tick150K: current_pos == 225000000");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_cycle_running(), "step30K.tick150K: stepper_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 225000000, "step30K.tick150K+1: current_pos == 225000000");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
}

static void test_aliquant_speed_tick_by_tick() {
    // движение мотора со скоростью, некратной периоду таймера

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5мкм=7500нм
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // проедем 5 шагов со скоростью, некратной частоте таймера:
    // пусть будет 1105 микросекунд между шагами
    prepare_steps(&sm_x, 5, 1105);
    
    // разберем шаги по косточкам
    
    // период таймера 200 микросекунд кратен минимальной задержке между шагами
    // моторов 1000 микросекунд (в промежуток мерду шагами умещается ровно 5 тиков таймера),
    // поэтому максимальную скорость моторов не уменьшаем
    // см: https://github.com/1i7/stepper_h/issues/6
    
    // "идеальные" шаги - каждые 1300 микросекунд:
    // 1105, 2210, 3315, 4420, 5525? ...
    //
    // тики таймера (200 микросекунд):
    //                       шаг1                                  шаг2
    // 200, 400, [600, 800, 1000], 1200, 1400, 1600, [1800, 2000, 2200], 2400,
    // 1    2     3    4    5      6     7     8      9     10    11     12
    //                    шаг3                                  шаг4                          шаг5 ...
    // 2600, [2800 3000, 3200], 3400, 3600, 3800, [4000, 4200, 4400], 4600, 4800, [5000, 5200, 5400], 5600 ...
    // 13    14    15    16     17    18    19     20    21    22     23    24     25    26    27     28 ...
    // 
    // пороги реальных шагов НЕ совпадают с "идеальными", шаг происходит каждые 5-6 тиков
    
    // а вот теперь поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    
    // шаг 1
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick2: current_pos == 0");
    //cout<<sm_x.current_pos<<endl;
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7500, "step1.tick3: current_pos == 7500");
    //cout<<sm_x.current_pos<<endl;
    
    // шаг 2
    // холостой ход
    timer_tick(3);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7500, "step2.tick2: current_pos == 7500");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15000, "step2.tick3: current_pos == 15000");
    
    // шаг 3
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15000, "step3.tick2: current_pos == 15000");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 22500, "step3.tick3: current_pos == 22500");
    
    // шаг 4
    // холостой ход
    timer_tick(3);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 22500, "step4.tick2: current_pos == 22500");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 30000, "step4.tick3: current_pos == 30000");
    
    // шаг 5
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 30000, "step5.tick2: current_pos == 30000");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 37500, "step5.tick3: current_pos == 37500");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_cycle_running(), "step5.tick3: stepper_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 37500, "step5.tick3+1: current_pos == 37500");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
}

static void test_draw_triangle() {
    // нарисуем треугольник в 3д,
    // убедимся, что вернулись в исходную точку
    
    stepper sm_x, sm_y, sm_z;
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 216000000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 1000, 7500);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, INF, INF, 0, 100000000);
    
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    ////////////
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    ///////////
    
    // p0 -> p1 -> p2 -> p0
    //   line1 line2 line3
    // cm: (0,0,0) -> (15,5,2) -> (5,15,2) -> (0,0,0)
    // uM: (0,0,0) -> (150000,50000,20000) -> (50000,150000,20000) -> (0,0,0)
    // nm: (0,0,0) -> (150000000,50000000,20000000) -> (50000000,150000000,20000000) -> (0,0,0)
    
    // #1 линия1
    // line1: (0,0,0) -> (150000000,50000000,20000000)
    
    // X - длинная координата - ее проходим с максимальной скоростью
    //long steps_x = 150000000 / 7500;
    long steps_x = (150000000 - sm_x.current_pos) / 7500;
    unsigned long delay_x = 1000;
    unsigned long time_x = delay_x * abs(steps_x);
    
    //long steps_y = 50000000 / 7500;
    long steps_y = (50000000 - sm_y.current_pos) / 7500;
    unsigned long delay_y = time_x / abs(steps_y);
    
    //long steps_z = 20000000 / 7500;
    long steps_z = (20000000 - sm_z.current_pos) / 7500;
    unsigned long delay_z = time_x / abs(steps_z);

    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    prepare_steps(&sm_z, steps_z, delay_z);
    
    // пошагали
    stepper_start_cycle();
    timer_tick(abs(steps_x)*5+1);
    sput_fail_unless(!stepper_cycle_running(), "line1: stepper_cycle_running() == false");
    
    // #2 линия2
    // line2: (150000000,50000000,20000000) -> (50000000,150000000,20000000)
    
    // здесь путь по x и y одинаковый, но точные количества шагов
    // с учетом погрешностей округления могут отличаться на +/- один шаг:
    // реально получится:
    // steps_x=-13333, steps_y=13334
    
    //long steps_y = (150000000 - 50000000) / 7500;
    steps_y = (150000000 - sm_y.current_pos) / 7500;
    delay_y = 1000;
    unsigned long time_y = delay_y * abs(steps_y);
    
    //long steps_x = (50000000 - 150000000) / 7500;
    steps_x = (50000000 - sm_x.current_pos) / 7500;
    delay_x = time_y / abs(steps_y);
    
    // путь по z=0
    //long steps_z = (20000000 - 20000000) / 7500;
    //long steps_z = (20000000 - sm_z.current_pos) / 7500;
    //long delay_z = time_y / abs(steps_z);

    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    //prepare_steps(&sm_z, steps_z, delay_z);
    
    // пошагали
    //cout<<"steps_x="<<steps_x<<", steps_y="<<steps_y<<endl;
    stepper_start_cycle();
    timer_tick(abs(steps_y)*5+1);
    sput_fail_unless(!stepper_cycle_running(), "line2: stepper_cycle_running() == false");
    
    // #3 линия3
    // line3: (50000000,150000000,20000000) -> (0,0,0)
    
    // Y - длинная координата - ее проходим с максимальной скоростью
    //long steps_y = -150000000 / 7500;
    steps_y = (0 - sm_y.current_pos) / 7500;
    delay_y = 1000;
    time_y = delay_y * abs(steps_y);
    
    //long steps_x = -50000000 / 7500;
    steps_x = (0 - sm_x.current_pos) / 7500;
    delay_x = time_y / abs(steps_x);
    
    //long steps_z = -20000000 / 7500;
    steps_z = (0 - sm_z.current_pos) / 7500;
    delay_z = time_y / abs(steps_z);

    // prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    prepare_steps(&sm_x, steps_x, delay_x);
    prepare_steps(&sm_y, steps_y, delay_y);
    prepare_steps(&sm_z, steps_z, delay_z);
    
    // пошагали
    stepper_start_cycle();
    timer_tick(abs(steps_y)*5+1);
    sput_fail_unless(!stepper_cycle_running(), "line3: stepper_cycle_running() == false");
    
    // #4
    // Проехали весь треугольник, убедимся, что мы в исходной точке (0,0,0)
    sput_fail_unless(!stepper_cycle_running(), "done: stepper_cycle_running() == false");
    sput_fail_unless(sm_x.current_pos == 0, "done: sm_x.current_pos == 0");
    sput_fail_unless(sm_y.current_pos == 0, "done: sm_y.current_pos == 0");
    sput_fail_unless(sm_z.current_pos == 0, "done: sm_z.current_pos == 0");
}

static void test_small_step_delay_handlers() {
    // Проверим поведение в ситуации, когда задержка между шагами мотора
    // step_delay получается меньше допустимой motor->step_delay.
    // Реакция на такую ошибку зависит от настроек обработчика ошибок
    // stepper_set_error_handle_strategy:small_step_delay_handle
    // варианты: FIX/STOP_MOTOR/CANCEL_CYCLE
    
    stepper sm_x, sm_y;
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7500);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 216000000);
    
    // период таймера 200 микросекунд
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////
    // Запускаем два мотора:
    // - у мотора X в процессе вращения задежка перед очередным шагом
    // получается меньше, чем минимальное ограничение step_delay
    // - у мотора Y все в порядке - он запущен на непрерывное вращение
    
    // с мотором X делаем два шага с переменной скоростью:
    // шаг1: корректная задержка 1000мкс (чтобы проскочить stepper_start_cycle)
    // шаг2: задержка 800мкс<1000мкс - меньше допустимой, ошибка выскочит
    // во время работы цикла, её-то мы и должны поймать и обработать
    int buf_size = 2;
    unsigned long delay_buffer[] = {1000, 800};
    
    // stepper_set_error_handle_strategy(
    //     error_handle_strategy_t hard_end_handle,
    //     error_handle_strategy_t soft_end_handle,
    //     error_handle_strategy_t small_step_delay_handle,
    //     error_handle_strategy_t cycle_timing_exceed_handle)
    
    ////////
    // #1: автоматическое исправление задержки до минимально допустимого
    // small_step_delay_handle=FIX - неправильная задержка 800мкс должна
    // исправиться на 1000мкс
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, FIX, DONT_CHANGE);
    
    prepare_simple_buffered_steps(&sm_x, buf_size, delay_buffer, 1);
    prepare_whirl(&sm_y, 1, 1000, NONE);
    
    stepper_start_cycle();
    // цикл должен запуститься
    sput_fail_unless(stepper_cycle_running(), "handler=FIX: stepper_cycle_running() == true");
    
    // шаг1 - 5 тиков таймера, всё должно быть ок
    timer_tick(4);
    sput_fail_unless(!(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL),
        "handler=FIX.tick4: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == false");
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7500, "handler=FIX.tick5: sm_x.current_pos == 7500");
    sput_fail_unless(sm_y.current_pos == 7500, "handler=FIX.tick5: sm_y.current_pos == 7500");
    
    // на 5м тике должны поймать ошибку с новой задержкой перед новым шагом small_step_delay:
    // для мотора и цикла должны выставиться флаги с указанием проблемы, но цикл должен продолжить
    // работу
    sput_fail_unless(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL,
        "handler=FIX.tick5: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == true");
    
    // шаг2 - должны совершить за 5 тиков, цикл должен работать
    timer_tick(5);
    sput_fail_unless(sm_x.current_pos == 15000, "handler=FIX.tick5+5: sm_x.current_pos == 15000");
    sput_fail_unless(sm_y.current_pos == 15000, "handler=FIX.tick5+5: sm_y.current_pos == 15000");
    sput_fail_unless(stepper_cycle_running(), "handler=FIX.tick5+5: stepper_cycle_running() == true");
    
    // ну и достаточно - останавливаемся
    stepper_finish_cycle();
    
    ////////
    // #2: останавливаем мотор
    // small_step_delay_handle=STOP_MOTOR - мотор X останавливается,
    // мотор Y продолжает вращаться
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, STOP_MOTOR, DONT_CHANGE);
    
    // сбросим текущее положение в 0
    sm_x.current_pos = 0;
    sm_y.current_pos = 0;
    // и готовим те же шаги
    prepare_simple_buffered_steps(&sm_x, buf_size, delay_buffer, 1);
    prepare_whirl(&sm_y, 1, 1000, NONE);
    
    stepper_start_cycle();
    // цикл должен запуститься
    sput_fail_unless(stepper_cycle_running(), "handler=STOP_MOTOR: stepper_cycle_running() == true");
    
    // шаг1 - 5 тиков таймера, всё должно быть ок
    timer_tick(4);
    sput_fail_unless(!(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL),
        "handler=STOP_MOTOR.tick4: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == false");
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "handler=STOP_MOTOR.tick4: sm_x.status == STEPPER_STATUS_RUNNING");
    
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7500, "handler=STOP_MOTOR.tick5: sm_x.current_pos == 7500");
    sput_fail_unless(sm_y.current_pos == 7500, "handler=STOP_MOTOR.tick5: sm_y.current_pos == 7500");
    
    // на 5м тике должны поймать ошибку с новой задержкой перед новым шагом small_step_delay:
    // для мотора и цикла должны выставиться флаги с указанием проблемы, мотор X остановится,
    // но цикл должен продолжить работу
    sput_fail_unless(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL,
        "handler=STOP_MOTOR.tick5: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == true");
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "handler=STOP_MOTOR.tick5: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_y.status == STEPPER_STATUS_RUNNING,
        "handler=STOP_MOTOR.tick5: sm_y.status == STEPPER_STATUS_RUNNING");
    
    // шаг2 - должны совершить за 5 тиков, цикл должен работать
    timer_tick(5);
    sput_fail_unless(sm_x.current_pos == 7500, "handler=STOP_MOTOR.tick5+5: sm_x.current_pos == 7500");
    sput_fail_unless(sm_y.current_pos == 15000, "handler=STOP_MOTOR.tick5+5: sm_y.current_pos == 15000");
    sput_fail_unless(stepper_cycle_running(), "handler=STOP_MOTOR.tick5+5: tepper_cycle_running() == true");
    
    // ну и достаточно - останавливаемся
    stepper_finish_cycle();
    
    ////////
    // #3: останавливаем весь цикл
    // small_step_delay_handle=CANCEL_CYCLE - цикл завершается для всех моторов
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, CANCEL_CYCLE, DONT_CHANGE);
    
    // сбросим текущее положение в 0
    sm_x.current_pos = 0;
    sm_y.current_pos = 0;
    // и готовим те же шаги
    prepare_simple_buffered_steps(&sm_x, buf_size, delay_buffer, 1);
    prepare_whirl(&sm_y, 1, 1000, NONE);
    
    stepper_start_cycle();
    // цикл должен запуститься
    sput_fail_unless(stepper_cycle_running(), "handler=CANCEL_CYCLE: stepper_cycle_running() == true");
    
    // шаг1 - 5 тиков таймера, всё должно быть ок
    timer_tick(4);
    sput_fail_unless(!(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL),
        "handler=CANCEL_CYCLE.tick4: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == false");
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "handler=CANCEL_CYCLE.tick4: sm_x.status == STEPPER_STATUS_RUNNING");
    
    // на 5м тике должны поймать ошибку с новой задержкой перед новым шагом small_step_delay:
    // завершаем весь цикл - останавливаем оба мотора
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7500, "handler=CANCEL_CYCLE.tick5: sm_x.current_pos == 7500");
    // мотор Y даже не успеет шагнуть, т.к. он добавлен в цикл после мотора X
    sput_fail_unless(sm_y.current_pos == 0, "handler=CANCEL_CYCLE.tick5: sm_y.current_pos == 0");
    //sput_fail_unless(sm_y.current_pos == 7500, "handler=CANCEL_CYCLE.tick5: sm_y.current_pos == 7500");
    //cout<<sm_y.current_pos<<endl;
    
    // флаг ошибки
    sput_fail_unless(sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL,
        "handler=CANCEL_CYCLE.tick5: sm_x.error&STEPPER_ERROR_STEP_DELAY_SMALL == true");
    
    // моторы стоят
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "handler=CANCEL_CYCLE.tick5: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_y.status == STEPPER_STATUS_FINISHED,
        "handler=CANCEL_CYCLE.tick5: sm_y.status == STEPPER_STATUS_FINISHED");
    
    // и цикл тоже здесь закончился
    sput_fail_unless(!stepper_cycle_running(), "handler=CANCEL_CYCLE.tick5: tepper_cycle_running() == false");
    
    // шаг2 - не будет совершен
    timer_tick(5);
    sput_fail_unless(sm_x.current_pos == 7500, "handler=CANCEL_CYCLE.tick5+5: sm_x.current_pos == 7500");
    sput_fail_unless(sm_y.current_pos == 0, "handler=CANCEL_CYCLE.tick5+5: sm_y.current_pos == 0");
    sput_fail_unless(!stepper_cycle_running(), "handler=CANCEL_CYCLE.tick5+5: tepper_cycle_running() == false");
}

static void test_buffered_steps_tick_by_tick() {
    // Проверим движение с переменной скоростью с
    // prepare_buffered_steps
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    // мотор
    stepper sm_x;
    
    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5мкм=7500нм
    unsigned long step_delay_us = 1000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, step_delay_us, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // готовим шаги с переменной скоростью
    // void prepare_buffered_steps(stepper *smotor,
    //    int buf_size, unsigned long* delay_buffer, long* step_buffer)
    const int buf_size = 3;
    static unsigned long delay_buffer[buf_size];
    static long step_buffer[buf_size];

    delay_buffer[0] = step_delay_us; // максимальная скорость
    delay_buffer[1] = step_delay_us*10; // в 10 раз медленнее
    delay_buffer[2] = step_delay_us*2; // в 2 раза медленнее

    step_buffer[0] = 4; // 4 шага туда
    step_buffer[1] = -2; // 2 шага обратно
    step_buffer[2] = 4; // 4 шага туда
    
    prepare_buffered_steps(&sm_x, buf_size, delay_buffer, step_buffer);
    
    // шагаем
    stepper_start_cycle();
    
    // #1
    // первый цикл: 4 шага с задержкой 1000 мкс
    // delay_buffer[0] = step_delay_us; // = 1000мкс
    // step_buffer[0] = 4; // = 4шага
    
    // расстояние: 7500нм/шаг * 4шага = 30000нм
    // позиция в конце цикла: 0+30000=300000
    
    // период таймера 200 мкс, задержка между шагами - 1000 мкс:
    // 1000/200=5 тиков таймера на шаг,
    // всего 5тиков/шаг * 4шага = 20 тиков на подцикл
    
    // первый цикл - 20 тиков
    timer_tick(20);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "buffered steps: cycle 1 of 3: sm_x.status == STEPPER_STATUS_RUNNING");
    sput_fail_unless(sm_x.current_pos == 30000,
        "buffered steps: cycle 1 of 3: sm_x.current_pos == 30000");
    
    // #2
    // второй цикл: 2 шага в обратном направлении с задержкой 10000 мкс
    // delay_buffer[1] = step_delay_us*10; // = 1000мкс*10=10000мкс
    // step_buffer[1] = -2; // = -2шага
    
    // расстояние: 7500нм/шаг * 2шага = 15000нм
    //   минус - обратное направление (движемся к нулю,
    //   значение координаты уменьшаем на расстояние)
    // позиция в конце цикла: 30000-15000=15000
    
    // период таймера 200 мкс, задержка между шагами - 10000 мкс:
    // 10000/200=50 тиков таймера на шаг,
    // всего 50тиков/шаг * 2шага = 100 тиков на подцикл
    
    // поехали цикл - 100 тиков
    
    // шаг2.1:
    // новое положение: 30000-7500=22500
    timer_tick(50);
    sput_fail_unless(sm_x.current_pos == 30000-7500,
        "buffered steps: cycle 2 of 3, step1: sm_x.current_pos == 30000-7500 (=22500)");
    
    // шаг2.2:
    // новое положение: 22500-7500=15000
    timer_tick(50);
    sput_fail_unless(sm_x.current_pos == 30000-7500*2,
        "buffered steps: cycle 2 of 3, step2: sm_x.current_pos == 30000-7500*2 (=15000)");
    
    // цикл завершен, всё еще работаем
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "buffered steps: cycle 2 of 3: sm_x.status == STEPPER_STATUS_RUNNING");
    sput_fail_unless(sm_x.current_pos == 30000-7500*2,
        "buffered steps: cycle 2 of 3: sm_x.current_pos == 15000");
    
    // #3
    // третий цикл: 4 шага с задержкой 2000 мкс
    // delay_buffer[2] = step_delay_us*2; // = 1000мкс*2=2000мкс
    // step_buffer[2] = 4; // = 4шага
    
    // расстояние: 7500нм/шаг * 4шага = 30000нм
    // позиция в конце цикла: 15000+30000=45000
    
    // период таймера 200 мкс, задержка между шагами - 2000 мкс:
    // 2000/200=10 тиков таймера на шаг,
    // всего 10тиков/шаг * 4шага = 40 тиков на подцикл
    
    // третий цикл - 40 тиков + 1 завершающий тик
    
    // шаг3.1:
    // новое положение: 15000+7500=22500
    timer_tick(10);
    sput_fail_unless(sm_x.current_pos == 15000+7500,
        "buffered steps: cycle 3 of 3, step1: sm_x.current_pos == 15000+7500 (=22500)");
    
    // шаг3.2:
    // новое положение: 22500+7500=30000
    timer_tick(10);
    sput_fail_unless(sm_x.current_pos == 15000+7500*2,
        "buffered steps: cycle 3 of 3, step2: sm_x.current_pos == 15000+7500*2 (=30000)");
    
    // шаг3.3:
    // новое положение: 30000+7500=37500
    timer_tick(10);
    sput_fail_unless(sm_x.current_pos == 15000+7500*3,
        "buffered steps: cycle 3 of 3, step3: sm_x.current_pos == 15000+7500*3 (=37500)");
    
    // шаг3.4:
    // новое положение: 37500+7500=45000
    timer_tick(10);
    sput_fail_unless(sm_x.current_pos == 15000+7500*4,
        "buffered steps: cycle 3 of 3, step4: sm_x.current_pos == 15000+7500*4 (=45000)");
    
    // завершающий тик
    timer_tick(1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "buffered steps: cycle 3 of 3: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 45000,
        "buffered steps: cycle 3 of 3: sm_x.current_pos == 45000");
}


static void test_buffered_steps() {
    // Проверим движение с переменной скоростью с
    // prepare_buffered_steps
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    // мотор
    stepper sm_x;
    
    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5мкм=7500нм
    unsigned long step_delay_us = 1000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, step_delay_us, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // готовим шаги с переменной скоростью
    // void prepare_buffered_steps(stepper *smotor,
    //    int buf_size, unsigned long* delay_buffer, long* step_buffer)
    const int buf_size = 3;
    static unsigned long delay_buffer[buf_size];
    static long step_buffer[buf_size];

    delay_buffer[0] = step_delay_us; // максимальная скорость
    delay_buffer[1] = step_delay_us*10; // в 10 раз медленнее
    delay_buffer[2] = step_delay_us*2; // в 2 раза медленнее

    step_buffer[0] = 400*10; // 10 кругов туда
    step_buffer[1] = -400*5; // 5 кругов обратно
    step_buffer[2] = 400*2; // 2 круга туда
    
    prepare_buffered_steps(&sm_x, buf_size, delay_buffer, step_buffer);
    
    // шагаем
    stepper_start_cycle();
    
    // #1
    // первый цикл: 4000 шагов с задержкой 1000 мкс
    // delay_buffer[0] = step_delay_us; // = 1000мкс
    // step_buffer[0] = 400*10; // = 4000шагов
    
    // расстояние: 7500нм/шаг * 4000шагов = 30000000нм
    // позиция в конце цикла: 0+30000000=30000000
    
    // период таймера 200 мкс, задержка между шагами - 1000 мкс:
    // 1000/200=5 тиков таймера на шаг,
    // всего 5тиков/шаг * 4000шагов = 20000 тиков на подцикл
    
    // первый цикл - 20000 тиков
    timer_tick(20000);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "buffered steps: cycle 1 of 3: sm_x.status == STEPPER_STATUS_RUNNING");
    sput_fail_unless(sm_x.current_pos == 30000000,
        "buffered steps: cycle 1 of 3: sm_x.current_pos == 30000000");
    
    // #2
    // второй цикл: 2000 шагов в обратном направлении с задержкой 10000 мкс
    // delay_buffer[1] = step_delay_us*10; // = 1000мкс*10=10000мкс
    // step_buffer[1] = -400*5; // = -2000шагов
    
    // расстояние: 7500нм/шаг * 2000шагов = 15000000нм
    //   минус - обратное направление (движемся к нулю,
    //   значение координаты уменьшаем на расстояние)
    // позиция в конце цикла: 30000000-15000000=15000000
    
    // период таймера 200 мкс, задержка между шагами - 10000 мкс:
    // 10000/200=50 тиков таймера на шаг,
    // всего 50тиков/шаг * 2000шагов = 100000 тиков на подцикл
    
    // второй цикл - 100000 тиков
    
    // проверим первый шаг
    timer_tick(50);
    sput_fail_unless(sm_x.current_pos == 30000000-7500,
        "buffered steps: cycle 2 of 3, step1: sm_x.current_pos == 30000000-7500");
    
    // оставшиеся шаги
    timer_tick(100000-50);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "buffered steps: cycle 2 of 3: sm_x.status == STEPPER_STATUS_RUNNING");
    sput_fail_unless(sm_x.current_pos == 15000000,
        "buffered steps: cycle 2 of 3: sm_x.current_pos == 15000000");
    
    // #3
    // третий цикл: 800 шагов с задержкой 2000 мкс
    // delay_buffer[2] = step_delay_us*2; // = 1000мкс*2=2000мкс
    // step_buffer[2] = 400*2; // = 800шагов
    
    // расстояние: 7500нм/шаг * 800шагов = 6000000нм
    // позиция в конце цикла: 15000000+6000000=21000000
    
    // период таймера 200 мкс, задержка между шагами - 2000 мкс:
    // 2000/200=10 тиков таймера на шаг,
    // всего 10тиков/шаг * 800шагов = 8000 тиков на подцикл
    
    // третий цикл - 8000 тиков + 1 завершающий тик
    timer_tick(8000+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "buffered steps: cycle 3 of 3: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 21000000,
        "buffered steps: cycle 3 of 3: sm_x.current_pos == 21000000");
}

static void test_driver_std_modes() {
    // Проверим стандартные режимы драйвера step-dir
    // на полный проворот с делителями шага 1/1, 1/8, 1/16, 1/32
    // на разных частотах
    // https://github.com/1i7/stepper_h/issues/23
    
    // мотор
    stepper sm_x;
    
    // настройки мотора
    unsigned long _step_delay_us;
    unsigned long _dist_per_step;
    
    // шагов в цикле для полного оборота
    long _step_count;
    
    // настройки таймера
    int _timer_id = TIMER_DEFAULT;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment;
    unsigned long _timer_period_us;
    
    // игнорируем ошибку CYCLE_ERROR_HANDLER_TIMING_EXCEEDED
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, DONT_CHANGE, IGNORE);
    
    ///////////
    // #1
    // Шагаем полный оборот с делителем 1/1 (без деления шага)
    // с периодом таймера 200мкс
    // шагов в полном обороте: 200
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/200 = 200000 нм
    // период таймера: 200 мкс
    // задержка между шагами (кратно 200, чуть менее стабильно, чем 1500): 1400 мкс
    // тиков таймера на шаг: 1400мкс/200мкс=7
    // тиков таймера на оборот: 7*200=1400
    
    // настройки таймера
    // для периода 200 микросекунд (5тыс вызовов в секунду == 5КГц)
    _timer_adjustment = 2000;
    _timer_period_us = 200;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 1400;
    _dist_per_step = 200000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 200;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 1400 тиков + 1 завершающий
    timer_tick(1400+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/1, timer_period=200us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/1, timer_period=200us: sm_x.current_pos == 40000000");
    
    ///////////
    // #2
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 1500;
    _dist_per_step = 200000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 200;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 15000 тиков + 1 завершающий
    timer_tick(15000+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/1, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/1, timer_period=20us: sm_x.current_pos == 40000000");
    
    ///////////
    // #3
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 660;
    _dist_per_step = 100000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 400;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 13200 тиков + 1 завершающий
    timer_tick(13200+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/2, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/2, timer_period=20us: sm_x.current_pos == 40000000");
    
    ///////////
    // #4
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 340;
    _dist_per_step = 50000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 800;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 13600 тиков + 1 завершающий
    timer_tick(13600+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/4, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/4, timer_period=20us: sm_x.current_pos == 40000000");
    
    ///////////
    // #5
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 180;
    _dist_per_step = 25000;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 1600;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 14400 тиков + 1 завершающий
    timer_tick(14400+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/8, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/8, timer_period=20us: sm_x.current_pos == 40000000");
        
    ///////////
    // #6
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 80;
    _dist_per_step = 12500;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 3200;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 12800 тиков + 1 завершающий
    timer_tick(12800+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/16, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/16, timer_period=20us: sm_x.current_pos == 40000000");
    
    ///////////
    // #7
    // Шагаем полный оборот с делителем 1/32
    // с периодом таймера 10 мкс
    // шагов в полном обороте: 200*32=6400
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/6400 = 6250 нм
    // период таймера: 10 мкс
    // задержка между шагами: 40 мкс
    // тиков таймера на шаг: 40мкс/10мкс=4
    // тиков таймера на оборот: 4*6400=25600
    
    // настройки таймера
    // для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
    _timer_adjustment = 100;
    _timer_period_us = 10;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 40;
    _dist_per_step = 6250;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 6400;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 25600 тиков + 1 завершающий
    timer_tick(25600+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/32, timer_period=10us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/32, timer_period=10us: sm_x.current_pos == 40000000");
    
    ///////////
    // #8
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки мотора
    _step_delay_us = 60;
    _dist_per_step = 6250;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, _step_delay_us, _dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    _step_count = 6400;
    prepare_steps(&sm_x, _step_count, _step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // должны уложиться ровно в 19200 тиков + 1 завершающий
    timer_tick(19200+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "step_divider=1/32, timer_period=20us: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "step_divider=1/32, timer_period=20us: sm_x.current_pos == 40000000");
}

static void test_driver_std_modes_2motors() {
    // Проверим стандартные режимы драйвера step-dir
    // на полный проворот с разными делителями шага
    // на разных частотах для 2х моторов одновременно
    // https://github.com/1i7/stepper_h/issues/23
    
    // моторы
    stepper sm_x, sm_y;
    
    // настройки мотора
    unsigned long x_step_delay_us;
    unsigned long x_dist_per_step;
    
    unsigned long y_step_delay_us;
    unsigned long y_dist_per_step;
    
    // шагов в цикле для полного оборота
    long x_step_count;
    long y_step_count;
    
    // настройки таймера
    int _timer_id = TIMER_DEFAULT;
    int _timer_prescaler = TIMER_PRESCALER_1_8;
    unsigned int _timer_adjustment;
    unsigned long _timer_period_us;
    
    // игнорируем ошибку CYCLE_ERROR_HANDLER_TIMING_EXCEEDED
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, DONT_CHANGE, IGNORE);

    ///////////
    // мотор X
    // Шагаем полный оборот с делителем 1/32
    // с периодом таймера 20 мкс
    // шагов в полном обороте: 200*32=6400
    // расстояние при полном обороте: 4см = 40000000 нм
    // длина шага: 40000000нм/6400 = 6250 нм
    // период таймера: 20 мкс
    // задержка между шагами (кратно 20): 60 мкс
    // тиков таймера на шаг: 60мкс/20мкс=3
    // тиков таймера на оборот: 3*6400=19200
    
    ///////////
    // мотор Y
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
    _timer_adjustment = 200;
    _timer_period_us = 20;
    stepper_configure_timer(_timer_period_us, _timer_id, _timer_prescaler, _timer_adjustment);

    // настройки моторов
    x_step_delay_us = 60;
    x_dist_per_step = 6250;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, x_step_delay_us, x_dist_per_step);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    y_step_delay_us = 800;
    y_dist_per_step = 200000;
    init_stepper(&sm_y, 'y', 11, 12, 13, false, y_step_delay_us, y_dist_per_step);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 300000000);
    
    // готовим шаги на полный круг (шагаем с максимальной скоростью)
    x_step_count = 6400;
    prepare_steps(&sm_x, x_step_count, x_step_delay_us);
    
    y_step_count = 200;
    prepare_steps(&sm_y, y_step_count, y_step_delay_us);
    
    // шагаем
    stepper_start_cycle();
    
    // для мотора Y должны уложиться ровно в 15000 тиков + 1 завершающий,
    // X пока продолжает вращение
    timer_tick(15000+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_RUNNING,
        "x+y, tick 15000+1: sm_x.status == STEPPER_STATUS_RUNNING");
    sput_fail_unless(sm_y.status == STEPPER_STATUS_FINISHED,
        "x+y, tick 15000+1: sm_y.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_y.current_pos == 40000000,
        "x+y, tick 15000+1: sm_y.current_pos == 40000000");
    
    // после 19200 тиков + 1 завершающий оба мотора должны остановиться
    // (Y и так стоял, X остановился)
    timer_tick(19200-(15000+1)+1);
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED,
        "x+y, tick 19200+1: sm_x.status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.current_pos == 40000000,
        "x+y, tick 19200+1: sm_x.current_pos == 40000000");
}

static void test_exit_bounds_issue1_whirl() {

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7500нм=7.5мкм
    // рабочая область: 300000000нм=300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000000
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    //
    // Мотор всегда делает первый шаг в цикле, даже если выходит за виртуальные границы
    // в случае, если задать задержку между шагами 0 (должна исправиться на минимальную
    // задержку motor->step_delay, т.е. максимальную скорость).
    // Если указывать задержку значением (motor->step_delay или 1000), то всё ок.
    // https://github.com/1i7/stepper_h/issues/1
    
    // попробуем выйти в минус за 0
    
    // готовим мотор на непрерывное вращение в сторону нуля,
    // задержку между шагами задаём как 0 (должна исправиться автоматом на
    // sm_x->step_delay, т.е. на 1000)
    // void prepare_whirl(stepper *smotor,
    //     int dir, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE);
    prepare_whirl(&sm_x, -1, 0, NONE);
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    // 
    // значение координаты (без учета границ)
    // -7500, -15000, -25000
    //
    // тики таймера (200 микросекунд)
    //                                                       шаг1                            шаг2
    // [<tick1:проверка границ>  <tick2:взвод step>  <tick3: шаг>]
    //   200, 400,         [600,                800,         1000], 1200, 1400, [1600, 1800, 2000], 2200 ...
    //   1    2             3                   4            5      6     7      8     9     10     11
    //
    
    // поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    sput_fail_unless(sm_x.current_pos == 0, "current_pos == 0");
    
    // холостой ход
    timer_tick(2);
    
    // начинаем из нуля
    sput_fail_unless(sm_x.current_pos == 0, "step1.begin: current_pos == 0");
    // внезапно, здесь фейл.
    // а вот и объяснение: если указываем step_delay=0 в prepare_whirl, то первый шаг будет
    // сделан без всех проверок сразу на 1й тик таймера (этапы tick1 и tick2 будут пропущены),
    // т.к. первый тик сразу получается наиближайшим к фронту шага (т.е. нулю).
    // Решение: во всех местах, где устанавливаем значение step_delay для очередного шага,
    // нужно делать проверку, что в нее уместится как минимум 3 тика таймера
    // (сравнения с аппаратными ограничениями задержки для шагов мотора достаточно,
    // т.к. она должна заведомо включать как минимум 3 тика).
    // В нашем случае - это метод prepare_whirl, но другие места (особенно там, где есть динамическая
    // задержка) нужно тоже все проверить.
    
    // проверка границ
    timer_tick(1);
    // здесь мы должны увидеть, что этот шаг приведет к выходу за виртуальную границу,
    // должны быть выставлены коды ошибок, статус мотора обозначен как "остановлен"
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED, "step1.tick1: status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.error&STEPPER_ERROR_SOFT_END_MIN,
        "step1.tick1: error&STEPPER_ERROR_SOFT_END_MIN == true");
    sput_fail_unless(!(sm_x.error&STEPPER_ERROR_SOFT_END_MAX),
        "step1.tick1: error&STEPPER_ERROR_SOFT_END_MAX == false");
    
    // на всякий случай проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick1: current_pos == 0");
    
    // цикл должен остановиться на этом же тике
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
}

static void test_exit_bounds_issue1_steps() {

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7500нм=7.5мкм
    // рабочая область: 300000000нм=300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000000
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // Мотор всегда делает первый шаг в цикле, даже если выходит за виртуальные границы
    // в случае, если задать задержку между шагами 0 (должна исправиться на минимальную
    // задержку motor->step_delay, т.е. максимальную скорость).
    // Если указывать задержку значением (motor->step_delay или 1000), то всё ок.
    // https://github.com/1i7/stepper_h/issues/1
    
    // попробуем выйти в минус за 0
    
    // 
    // готовим мотор на несколько шагов в сторону нуля,
    // задержку между шагами задаём как 0 (должна исправиться автоматом на
    // sm_x->step_delay, т.е. на 1000)
    // void prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE) {
    prepare_steps(&sm_x, -300, 0, NONE);
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    // 
    // значение координаты (без учета границ)
    // -7500, -15000, -25000
    //
    // тики таймера (200 микросекунд)
    //                                                       шаг1                            шаг2
    // [<tick1:проверка границ>  <tick2:взвод step>  <tick3: шаг>]
    //   200, 400,         [600,                800,         1000], 1200, 1400, [1600, 1800, 2000], 2200 ...
    //   1    2             3                   4            5      6     7      8     9     10     11
    //
    
    // поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    sput_fail_unless(sm_x.current_pos == 0, "current_pos == 0");
    
    // холостой ход
    timer_tick(2);
    
    // начинаем из нуля
    sput_fail_unless(sm_x.current_pos == 0, "step1.begin: current_pos == 0");
    // внезапно, здесь фейл.
    // а вот и объяснение: если указываем step_delay=0 в prepare_whirl, то первый шаг будет
    // сделан без всех проверок сразу на 1й тик таймера (этапы tick1 и tick2 будут пропущены),
    // т.к. первый тик сразу получается наиближайшим к фронту шага (т.е. нулю).
    // Решение: во всех местах, где устанавливаем значение step_delay для очередного шага,
    // нужно делать проверку, что в нее уместится как минимум 3 тика таймера
    // (сравнения с аппаратными ограничениями задержки для шагов мотора достаточно,
    // т.к. она должна заведомо включать как минимум 3 тика).
    // В нашем случае - это метод prepare_steps, но другие места (особенно там, где есть динамическая
    // задержка) нужно тоже все проверить.
    
    // проверка границ
    timer_tick(1);
    // здесь мы должны увидеть, что этот шаг приведет в выходу за виртуальную границу,
    // должны быть выставлены коды ошибок, статус мотора обозначен как "остановлен"
    sput_fail_unless(sm_x.status == STEPPER_STATUS_FINISHED, "step1.tick1: status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(sm_x.error&STEPPER_ERROR_SOFT_END_MIN, "step1.tick3: error&STEPPER_ERROR_SOFT_END_MIN == true");
    sput_fail_unless(!(sm_x.error&STEPPER_ERROR_SOFT_END_MAX), "step1.tick3: error&STEPPER_ERROR_SOFT_END_MAX == false");
    
    // на всякий случай проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick1: current_pos == 0");
    
    // цикл должен остановиться на этом же тике
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
}

static void test_exit_bounds_issue9_steps() {
    // https://github.com/1i7/stepper_h/issues/9

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7500нм=7.5мкм
    // рабочая область: 300000000нм=300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000000
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // выходим за границу, в задержку между шагами не умещается
    // 3 тика таймера
    // попробуем выйти в минус за 0
    
    
    // #1: попробуем вариант с завершением цикла
    
    // stepper_set_error_handle_strategy(
    //     error_handle_strategy_t hard_end_handle,
    //     error_handle_strategy_t soft_end_handle,
    //     error_handle_strategy_t small_step_delay_handle,
    //     error_handle_strategy_t cycle_timing_exceed_handle)
    
    // обрываем весь цикл, если встречаем такую ошибку
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, CANCEL_CYCLE, DONT_CHANGE);
    
    // 
    // готовим мотор на несколько шагов в сторону нуля,
    // ставим задержку между шагами меньше, чем 3 периода таймера:
    // 400 < 200*3=600
    // void prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE)
    prepare_steps(&sm_x, -300, 400);
    
    // поехали
    stepper_start_cycle();
    // цикл сразу не должен запуститься
    sput_fail_unless(!stepper_cycle_running(), "cancel cycle: stepper_cycle_running() == false");
    sput_fail_unless(stepper_cycle_error() == CYCLE_ERROR_MOTOR_ERROR,
        "cancel cycle: stepper_cycle_error() == CYCLE_ERROR_MOTOR_ERROR");
    sput_fail_unless(sm_x.current_pos == 0, "cancel cycle: current_pos == 0");
    
    // #2: попробуем вариант с автоматическим исправлением задержки
    
    // stepper_set_error_handle_strategy(
    //     error_handle_strategy_t hard_end_handle,
    //     error_handle_strategy_t soft_end_handle,
    //     error_handle_strategy_t small_step_delay_handle,
    //     error_handle_strategy_t cycle_timing_exceed_handle)
    
    // автоматически исправляем задержку, если встречаем такую ошибку
    stepper_set_error_handle_strategy(DONT_CHANGE, DONT_CHANGE, FIX, DONT_CHANGE);
    
    //
    // готовим мотор на несколько шагов в сторону нуля,
    // ставим задержку между шагами меньше, чем 3 периода таймера:
    // 400 < 200*3=600
    // void prepare_steps(stepper *smotor,
    //     long step_count, unsigned long step_delay,
    //     calibrate_mode_t calibrate_mode=NONE)
    prepare_steps(&sm_x, -300, 400);
    
    // поехали
    // цикл должен запуститься
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "autofix: stepper_cycle_running() == true");
    // холостой ход
    timer_tick(2);
    sput_fail_unless(stepper_cycle_running(), "autofix tick2: stepper_cycle_running() == true");
    // тик на проверку границ - дожны вылететь с ошибкой выхода за гринцы
    timer_tick(1);
    sput_fail_unless(!stepper_cycle_running(), "autofix tick2+1: stepper_cycle_running() == false");
    sput_fail_unless(sm_x.current_pos == 0, "autofix: current_pos == 0");
}

static void test_square_sig_issue16() {
    // https://github.com/1i7/stepper_h/issues/16

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7500нм=7.5мкм
    // рабочая область: 300000000нм=300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000000
    stepper sm_x;
    int x_step = 8;
    init_stepper(&sm_x, 'x', x_step, 9, 10, false, 1000, 7500);
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000000);
    
    // настройки частоты таймера
    unsigned long timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER_DEFAULT, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_cycle_running(), "stepper_cycle_running() == false");
    
    // запускаем шаги, проверяем, что сигнал действительно
    // прямоугольный на каждом шаге
    
    // проедем 30000 шагов с максимальной скоростью
    prepare_steps(&sm_x, 30000, 1000);
    
    // разберем шаги по косточкам
    
    // период таймера 200 микросекунд кратен минимальной задержке между шагами
    // моторов 1000 микросекунд (в промежуток мерду шагами умещается ровно 5 тиков таймера),
    // поэтому максимальную скорость моторов не уменьшаем
    // см: https://github.com/1i7/stepper_h/issues/6
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    //
    // тики таймера (200 микросекунд) - каждый 5й порог совпадает с "идеальным" шагом:
    //                       шаг1                            шаг2                           шаг3
    // 200, 400, [600, 800, 1000], 1200, 1400, [1600, 1800, 2000], 2200, 2400, [2600, 2800 3000], 3200 ...
    // 1    2     3    4    5      6     7      8     9     10     11    12     13    14   15
    //
    // пороги реальных шагов совпадают с "идеальными"
    
    // а вот теперь поехали
    stepper_start_cycle();
    sput_fail_unless(stepper_cycle_running(), "stepper_cycle_running() == true");
    
    // проедем 30000 шагов и для каждого шага будем проверять,
    // что пин step сначала находится в позиции LOW, затем
    // за тик за шага переводится в положение HIGH и
    // в последний тик шага сбрасывается в LOW
    int i = 0;
    bool ok = true;
    for(; i < 30000 && ok; i++) {
        // iй шаг
        // холостой ход
        timer_tick(2);
        // проверка границ
        timer_tick(1);
        if(ok) ok = (digitalRead(x_step) == 0);
        // взвести step
        timer_tick(1);
        if(ok) ok = (digitalRead(x_step) == 1);
        // сделать шаг, взвести счетчики на следующий шаг
        timer_tick(1);
        if(ok) ok = (digitalRead(x_step) == 0);
    }
    sput_fail_unless(ok, "square sig ok");
    //if(!ok) cout<<"square sig failed at "<<i-1<<endl;
}



/////////////////////////////////////////////////////////
// test suites

/** Stepper cycle lifecycle */
int stepper_test_suite_lifecycle() {
    sput_start_testing();

    sput_enter_suite("Stepper cycle lifecycle");
    sput_run_test(test_lifecycle);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Stepper cycle timer period settings */
int stepper_test_suite_timer_period() {
    sput_start_testing();

    sput_enter_suite("Stepper cycle timer period settings");
    sput_run_test(test_timer_period);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Stepper cycle timer period is aliquant part of motor step pulse delay */
int stepper_test_suite_timer_period_aliquant_step_delay() {
    sput_start_testing();
    
    sput_enter_suite("Stepper cycle timer period is aliquant part of motor step pulse delay");
    sput_run_test(test_timer_period_aliquant_step_delay);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: 3 steps tick by tick on max speed */
int stepper_test_suite_max_speed_tick_by_tick() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: 3 steps tick by tick on max speed");
    sput_run_test(test_max_speed_tick_by_tick);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: 30000 steps on max speed */
int stepper_test_suite_max_speed_30000steps() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: 30000 steps on max speed");
    sput_run_test(test_max_speed_30000steps);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: 5 steps tick by tick on aliquant speed */
int stepper_test_suite_aliquant_speed_tick_by_tick() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: 5 steps tick by tick on aliquant speed");
    sput_run_test(test_aliquant_speed_tick_by_tick);

    sput_finish_testing();
    return sput_get_return_value();
}

/** 3 motors: draw triangle */
int stepper_test_suite_draw_triangle() {
    sput_start_testing();
    
    sput_enter_suite("3 motors: draw triangle");
    sput_run_test(test_draw_triangle);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Small step delay error handlers */
int stepper_test_suite_small_step_delay_handlers() {
    sput_start_testing();
    
    sput_enter_suite("Small step delay error handlers");
    sput_run_test(test_small_step_delay_handlers);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Moving with variable speed: buffered steps tick by tick */
int stepper_test_suite_buffered_steps_tick_by_tick() {
    sput_start_testing();
    
    sput_enter_suite("Moving with variable speed: buffered steps tick by tick");
    sput_run_test(test_buffered_steps_tick_by_tick);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Moving with variable speed: buffered steps */
int stepper_test_suite_buffered_steps() {
    sput_start_testing();
    
    sput_enter_suite("Moving with variable speed: buffered steps");
    sput_run_test(test_buffered_steps);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Step-dir driver std divider modes: 1/1, 1/18, 1/16, 1/32 */
int stepper_test_suite_driver_std_modes() {
    sput_start_testing();
    
    sput_enter_suite("Step-dir driver std divider modes: 1/1, 1/18, 1/16, 1/32");
    sput_run_test(test_driver_std_modes);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Step-dir driver std divider modes for 2 motors at a time */
int stepper_test_suite_driver_std_modes_2motors() {
    sput_start_testing();
    
    sput_enter_suite("Step-dir driver std divider modes for 2 motors at a time");
    sput_run_test(test_driver_std_modes_2motors);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: exit bounds (issue #1) - whirl */
int stepper_test_suite_exit_bounds_issue1_whirl() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: exit bounds (issue #1) - whirl");
    sput_run_test(test_exit_bounds_issue1_whirl);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: exit bounds (issue #1) - steps */
int stepper_test_suite_exit_bounds_issue1_steps() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: exit bounds (issue #1) - steps");
    sput_run_test(test_exit_bounds_issue1_steps);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: exit bounds (issue #9) - steps */
int stepper_test_suite_exit_bounds_issue9_steps() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: exit bounds (issue #9) - steps");
    sput_run_test(test_exit_bounds_issue9_steps);

    sput_finish_testing();
    return sput_get_return_value();
}

/** Single motor: test square signal (issue #16) */
int stepper_test_suite_square_sig_issue16() {
    sput_start_testing();
    
    sput_enter_suite("Single motor: test square signal (issue #16)");
    sput_run_test(test_square_sig_issue16);
    
    sput_finish_testing();
    return sput_get_return_value();
}

/** All tests in one bundle */
int stepper_test_suite() {
    sput_start_testing();

    sput_enter_suite("Stepper cycle lifecycle");
    sput_run_test(test_lifecycle);
    
    sput_enter_suite("Stepper cycle timer period settings");
    sput_run_test(test_timer_period);
    
    sput_enter_suite("Stepper cycle timer period is aliquant part of motor step pulse delay");
    sput_run_test(test_timer_period_aliquant_step_delay);
    
    sput_enter_suite("Single motor: 3 steps tick by tick on max speed");
    sput_run_test(test_max_speed_tick_by_tick);
    
    sput_enter_suite("Single motor: 30000 steps on max speed");
    sput_run_test(test_max_speed_30000steps);
    
    sput_enter_suite("Single motor: 5 steps tick by tick on aliquant speed");
    sput_run_test(test_aliquant_speed_tick_by_tick);
    
    sput_enter_suite("3 motors: draw triangle");
    sput_run_test(test_draw_triangle);
    
    sput_enter_suite("Small step delay error handlers");
    sput_run_test(test_small_step_delay_handlers);
    
    sput_enter_suite("Moving with variable speed: buffered steps tick by tick");
    sput_run_test(test_buffered_steps_tick_by_tick);
    
    sput_enter_suite("Moving with variable speed: buffered steps");
    sput_run_test(test_buffered_steps);
    
    
    sput_enter_suite("Step-dir driver std divider modes: 1/1, 1/18, 1/16, 1/32");
    sput_run_test(test_driver_std_modes);
    
    sput_enter_suite("Step-dir driver std divider modes for 2 motors at a time");
    sput_run_test(test_driver_std_modes_2motors);
    
    
    sput_enter_suite("Single motor: exit bounds (issue #1) - whirl");
    sput_run_test(test_exit_bounds_issue1_whirl);
    
    sput_enter_suite("Single motor: exit bounds (issue #1) - steps");
    sput_run_test(test_exit_bounds_issue1_steps);
    
    sput_enter_suite("Single motor: exit bounds (issue #9) - steps");
    sput_run_test(test_exit_bounds_issue9_steps);
    
    sput_enter_suite("Single motor: test square signal (issue #16)");
    sput_run_test(test_square_sig_issue16);
    
    sput_finish_testing();
    return sput_get_return_value();
}

