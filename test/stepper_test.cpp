#include "stepper.h"

extern "C"{
    #include "timer_setup.h"
}

#include "stddef.h"
#include "stdio.h"
#include <iostream>


// http://www.use-strict.de/sput-unit-testing/tutorial.html
#include <sput.h>

using namespace std;

/**
 * Симуляция таймера: "сгенерировать" нужное количество импульсов - 
 * вызвать обработчик прерывания handle_interrupts нужное количество
 * раз.
 * @param count - количество тиков (импульсов) таймера
 */
void timer_tick(int count) {
    for(int i = 0; i < count; i++) {
        handle_interrupts(3);
    }
}


static void test_lifecycle() {
    // проверим жизненный цикл серии шагов: 
    // цикл запущен/остановлен/на паузе и т.п.
    
    stepper sm_x, sm_y, sm_z;
    // X
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, INF, INF, 0, 300000);
    // Y
    init_stepper(&sm_y, 'y', 5, 6, 7, true, 1000, 7.5);
    init_stepper_ends(&sm_y, NO_PIN, NO_PIN, INF, INF, 0, 216000);
    // Z
    init_stepper(&sm_z, 'z', 2, 3, 4, true, 1000, 7.5);
    init_stepper_ends(&sm_z, NO_PIN, NO_PIN, INF, INF, 0, 100000);
    
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
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
    sput_fail_unless(!stepper_is_cycle_running(), "not started: stepper_is_cycle_running() == false");
    
    // теперь будет запущен
    stepper_start_cycle();
    sput_fail_unless(stepper_is_cycle_running(), "started: stepper_is_cycle_running() == true");
    
    // сделаем сколько-нибудь тиков таймера
    timer_tick(50000);
    // все еще должны двигаться
    sput_fail_unless(stepper_is_cycle_running(), "ticks: stepper_is_cycle_running() == true");
    
    // встанем на паузу
    stepper_pause_cycle();
    timer_tick(50000);
    sput_fail_unless(stepper_is_cycle_running(), "paused: stepper_is_cycle_running() == true");
    sput_fail_unless(stepper_is_cycle_paused(), "paused: stepper_is_cycle_paused() == true");
    
    // продолжим двигаться
    stepper_continue_cycle();
    timer_tick(50000);
    sput_fail_unless(stepper_is_cycle_running(), "continued: stepper_is_cycle_running() == true");
    sput_fail_unless(!stepper_is_cycle_paused(), "continued: stepper_is_cycle_paused() == false");
    
    // останавливаемся
    stepper_finish_cycle();
    sput_fail_unless(!stepper_is_cycle_running(), "finished: stepper_is_cycle_running() == false");
    timer_tick(50000);
    sput_fail_unless(!stepper_is_cycle_running(), "finished: stepper_is_cycle_running() == false");
}

static void test_max_speed_tick_by_tick() {

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5 мкм
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000);
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен 
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
    
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
    sput_fail_unless(stepper_is_cycle_running(), "stepper_is_cycle_running() == true");
    
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
    sput_fail_unless(sm_x.current_pos == 7.5, "step1.tick3: current_pos == 7.5");
    //cout<<sm_x.current_pos<<endl;
    
    // шаг 2
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7.5, "step2.tick2: current_pos == 7.5");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15, "step2.tick3: current_pos == 15");
    
    // шаг 3
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15, "step3.tick2: current_pos == 15");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 22.5, "step3.tick3: current_pos == 22.5");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_is_cycle_running(), "step3.tick3: stepper_is_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 22.5, "step3.tick3+1: current_pos == 22.5");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
}

static void test_max_speed_30000steps() {

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5 мкм
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000);
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен 
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
    
    // пройдем 30000 шагов с максимальной скоростью
    // это займет 30000*1000=30000000 микросекунд = 30 секунд
    // расстояние будет=7.5*30000=225000 микрометров
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
    sput_fail_unless(stepper_is_cycle_running(), "stepper_is_cycle_running() == true");
    
    // в одном шаге на максимальной скорости умещается ровно 5 тиков (импульсов) таймера,
    // чтобы пройти 30000 шагов, таймер должен тикнуть 5*30000+1=150001 раз 
    // (последний тик завершает отработавшую серию)
    
    // шагаем 60000+90000+1=150001 тиков таймера
    
    // проверим положение на половине пути
    // 60000/5=12000 шагов, путь=7.5*12000=90000 мкм
    timer_tick(60000);
    sput_fail_unless(sm_x.current_pos == 90000, "step30K.tick60K: current_pos == 90000");
    
    // шагаем оставшиеся шаги
    timer_tick(90000);
    sput_fail_unless(sm_x.current_pos == 225000, "step30K.tick150K: current_pos == 225000");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_is_cycle_running(), "step30K.tick150K: stepper_is_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 225000, "step30K.tick150K+1: current_pos == 225000");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
}

static void test_aliquant_speed_tick_by_tick() {

    // мотор - минимальная задержка между шагами 1000 мкс,
    // расстояние за шаг - 7.5 мкм
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000);
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен 
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
    
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
    sput_fail_unless(stepper_is_cycle_running(), "stepper_is_cycle_running() == true");
    
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
    sput_fail_unless(sm_x.current_pos == 7.5, "step1.tick3: current_pos == 7.5");
    //cout<<sm_x.current_pos<<endl;
    
    // шаг 2
    // холостой ход
    timer_tick(3);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 7.5, "step2.tick2: current_pos == 7.5");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15, "step2.tick3: current_pos == 15");
    
    // шаг 3
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 15, "step3.tick2: current_pos == 15");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 22.5, "step3.tick3: current_pos == 22.5");
    
    // шаг 4
    // холостой ход
    timer_tick(3);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 22.5, "step4.tick2: current_pos == 22.5");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 30, "step4.tick3: current_pos == 30");
    
    // шаг 5
    // холостой ход
    timer_tick(2);
    // проверка границ
    timer_tick(1);
    // взвести step
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 30, "step5.tick2: current_pos == 30");
    // сделать шаг, взвести счетчики на следующий шаг
    timer_tick(1);
    sput_fail_unless(sm_x.current_pos == 37.5, "step5.tick3: current_pos == 37.5");
    
    // все шаги сделали, но для завершения цикла нужен еще 1 финальный тик
    sput_fail_unless(stepper_is_cycle_running(), "step5.tick3: stepper_is_cycle_running() == true");
    
    // завершающий тик - для завершения цикла серии шагов
    timer_tick(1);
    // еще раз проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 37.5, "step5.tick3+1: current_pos == 37.5");
    
    ////////
    // проверим, что серия шагов завершилась, можно запускать новые шаги
    // цикл не должен быть запущен
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
}


static void test_exit_bounds_issue3_whirl() {

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7.5 микрометров
    // рабочая область: 300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000);
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен 
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
    
    // 
    // Мотор всегда делает первый шаг в цикле, даже если выходит за виртуальные границы
    // в случае, если задать задержку между шагами 0 (должна исправиться на минимальную
    // задержку motor->pulse_delay, т.е. максимальную скорость).
    // Если указывать задержку значением (motor->pulse_delay или 1000), то всё ок.
    // https://github.com/1i7/stepper_h/issues/1
    
    // попробуем выйти в минус за 0
    
    // получать информацию о статусе мотора и цикла
    stepper_info_t stepper_info;
    stepper_cycle_info_t cycle_info;
    
    // готовим мотор на непрерывное вращение в сторону нуля,
    // задержку между шагами задаём как 0 (должна исправиться автоматом на
    // sm_x->pulse_delay, т.е. на 1000)
    // void prepare_whirl(stepper *smotor, 
    //     int dir, int step_delay, 
    //     calibrate_mode_t calibrate_mode=NONE, stepper_info_t *stepper_info=NULL);
    prepare_whirl(&sm_x, -1, 0, NONE, &stepper_info);
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    // 
    // значение координаты (без учета границ)
    // -7.5, -15, -25
    //
    // тики таймера (200 микросекунд)
    //                                                       шаг1                            шаг2
    // [<tick1:проверка границ>  <tick2:взвод step>  <tick3: шаг>]
    //   200, 400,         [600,                800,         1000], 1200, 1400, [1600, 1800, 2000], 2200 ...
    //   1    2             3                   4            5      6     7      8     9     10     11
    //
    
    // поехали
    stepper_start_cycle(&cycle_info);
    sput_fail_unless(stepper_is_cycle_running(), "stepper_is_cycle_running() == true");
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
    // здесь мы должны увидеть, что этот шаг приведет в выходу за виртуальную границу,
    // должны быть выставлены коды ошибок, статус мотора обозначен как "остановлен"
    sput_fail_unless(stepper_info.status == STEPPER_STATUS_FINISHED, "step1.tick1: status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(stepper_info.error_soft_end_min, "step1.tick3: error_soft_end_min == true");
    sput_fail_unless(!stepper_info.error_soft_end_max, "step1.tick3: error_soft_end_max == false");
    
    // на всякий случай проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick1: current_pos == 0");
    
    // цикл должен остановиться на этом же тике
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
}

static void test_exit_bounds_issue3_steps() {

    // мотор - минимальная задежка между шагами: 1000 микросекунд
    // расстояние за шаг: 7.5 микрометров
    // рабочая область: 300000мкм=300мм=30см
    // нижняя граница координаты: ограничена=0
    // верхняя граница координаты: ограничена=300000
    stepper sm_x;
    init_stepper(&sm_x, 'x', 8, 9, 10, false, 1000, 7.5); 
    init_stepper_ends(&sm_x, NO_PIN, NO_PIN, CONST, CONST, 0, 300000);
    
    // настройки частоты таймера
    int timer_period_us = 200;
    stepper_configure_timer(timer_period_us, TIMER3, TIMER_PRESCALER_1_8, 2000);
    
    //////////////
    
    // на всякий случай: цикл не должен быть запущен 
    // (если запущен, то косяк в предыдущем тесте)
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
    
    // Мотор всегда делает первый шаг в цикле, даже если выходит за виртуальные границы
    // в случае, если задать задержку между шагами 0 (должна исправиться на минимальную
    // задержку motor->pulse_delay, т.е. максимальную скорость).
    // Если указывать задержку значением (motor->pulse_delay или 1000), то всё ок.
    // https://github.com/1i7/stepper_h/issues/1
    
    // попробуем выйти в минус за 0
    
    // получать информацию о статусе мотора и цикла
    stepper_info_t stepper_info;
    stepper_cycle_info_t cycle_info;
    
    // 
    // готовим мотор на несколько шагов в сторону нуля,
    // задержку между шагами задаём как 0 (должна исправиться автоматом на
    // sm_x->pulse_delay, т.е. на 1000)
    // void prepare_steps(stepper *smotor, 
    //     int step_count, int step_delay, 
    //     stepper_info_t *stepper_info) {
    prepare_steps(&sm_x, -300, 0, &stepper_info);
    
    // "идеальные" шаги - каждые 1000 микросекунд:
    // 1000, 2000, 3000
    // 
    // значение координаты (без учета границ)
    // -7.5, -15, -25
    //
    // тики таймера (200 микросекунд)
    //                                                       шаг1                            шаг2
    // [<tick1:проверка границ>  <tick2:взвод step>  <tick3: шаг>]
    //   200, 400,         [600,                800,         1000], 1200, 1400, [1600, 1800, 2000], 2200 ...
    //   1    2             3                   4            5      6     7      8     9     10     11
    //
    
    // поехали
    stepper_start_cycle(&cycle_info);
    sput_fail_unless(stepper_is_cycle_running(), "stepper_is_cycle_running() == true");
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
    sput_fail_unless(stepper_info.status == STEPPER_STATUS_FINISHED, "step1.tick1: status == STEPPER_STATUS_FINISHED");
    sput_fail_unless(stepper_info.error_soft_end_min, "step1.tick3: error_soft_end_min == true");
    sput_fail_unless(!stepper_info.error_soft_end_max, "step1.tick3: error_soft_end_max == false");
    
    // на всякий случай проверим финальное положение
    sput_fail_unless(sm_x.current_pos == 0, "step1.tick1: current_pos == 0");
    
    // цикл должен остановиться на этом же тике
    sput_fail_unless(!stepper_is_cycle_running(), "stepper_is_cycle_running() == false");
}

int main() {
    sput_start_testing();

    sput_enter_suite("Stepper cycle lifecycle");
    sput_run_test(test_lifecycle);
    
    sput_enter_suite("Single motor: 3 steps tick by tick on max speed");
    sput_run_test(test_max_speed_tick_by_tick);
    
    sput_enter_suite("Single motor: 30000 steps on max speed");
    sput_run_test(test_max_speed_30000steps);
    
    sput_enter_suite("Single motor: 5 steps tick by tick on aliquant speed");
    sput_run_test(test_aliquant_speed_tick_by_tick);
    
    
    sput_enter_suite("Single motor: exit bounds (issue #3) - whirl");
    sput_run_test(test_exit_bounds_issue3_whirl);
    
    sput_enter_suite("Single motor: exit bounds (issue #3) - steps");
    sput_run_test(test_exit_bounds_issue3_steps);
    
    sput_finish_testing();
    return sput_get_return_value();
}

