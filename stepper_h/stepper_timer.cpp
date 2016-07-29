/**
 * stepper_timer.cpp 
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс 
 * драйвера "step-dir".
 *
 * LGPL, 2014
 *
 * @author Антон Моисеев
 */
 
#include "WProgram.h"

extern "C"{
    #include "timer_setup.h"
}

#include "stepper.h"

#define DEBUG_SERIAL

/**
 * Статус цикла вращения мотора
 */
typedef enum {
    /** Ожидает запуска */
    IDLE, 
    
    /** Вращается */
    RUNNING, 
    
    /** Завершил вращение нормально */
    FINISHED_OK, 
    
    /** Завершил вращение из-за срабатывания концевого датчика нижней границы */
    HARD_END_MIN, 
    
    /** Завершил вращение из-за срабатывания концевого датчика верхней границы */
    HARD_END_MAX, 
    
    /** Завершил вращение из-за достижения виртуальной нижней границы */
    SOFT_END_MIN, 
    
    /** Завершил вращение из-за достижения виртуальной верхней границы */
    SOFT_END_MAX
} motor_cycle_status_t;

/**
 * Способы вычисления задержки перед следующим шагом
 */
typedef enum {
    /** Константа */
    CONSTANT, 
    
    /** Буфер задержек */
    BUFFER, 
    
    /** Динамическая задержка */
    DYNAMIC
} delay_source_t;

/**
 * Структура - статус текущего цикла мотора.
 */
typedef struct {
//// Настройки для текущего цикла шагов

    /** 
     * Направление движения
     *  1: вперед (увеличение виртуальной координаты curr_pos), 
     * -1: назад (уменьшение виртуальной координаты curr_pos) 
     */
    int dir;
    
    /** true: вращение без остановки, false: использовать step_count */
    bool non_stop;
    
    /** Количество шагов в текущей серии (если non_stop=false) */
    int step_count;
    
    /** 
     * CONSTANT: вращение с постоянной скоростью (использовать значение step_delay), 
     * BUFFER: вращение с переменной скоростью (использовать delay_buffer)
     * DYNAMIC: вращение с переменной скоростью (использовать next_step_delay)
     */
    delay_source_t delay_source;
    
    /**
     * Задержка между 2мя шагами мотора (определяет скорость вращения, 
     * 0 для максимальной скорости), микросекунды
     * 
     * Используется при delay_source=CONSTANT
     * 
     * Задержка в микросекундах, значения int на 32разрядном процессоре более, чем достаточно:
     * для int:
     * макс задержка=2^31=2147483648микросекунд=2147483миллисекунд=2147секунд=35минут
     * для unsigned int:
     * макс задержка=2^32=4294967296микросекунд=4294967миллисекунд=4294секунд=71минута=~1час
     */
    int step_delay;
    
    /**
     * Массив задержек перед каждым следующим шагом, микросекунды.
     * Размер массива <= step_count
     */
    int* delay_buffer;
    
    /**
     * Указатель на объект, содержащий всю необходимую информацию для вычисления
     * времени до следующего шага (должен подходить для параметра curve_context 
     * функции next_step_delay).
     *
     * Для встроенного алгоритма рисования дуги окружности тип curve_context будет circle_context_t
     *
     * Используется при delay_source=DYNAMIC
     */
    void* curve_context;
    
    /**
     * Ссылка на функцию, вычисляющую динамическую задержку перед следующим шагом мотора 
     * (определяет скорость вращения):
     * - при постоянной задержке мотор движется с постоянной скоростью (рисование прямой линии)
     * - при переменной задержке на 2х моторах движение инструмента криволинейно (рисование дуги окружности)
     *
     * Используется при delay_source=DYNAMIC
     *
     * @param curr_step - номер текущего шага
     * @param curve_context - указатель на объект, содержащий всю необходимую информацию для вычисления
     *     времени до следующего шага
     * @return время до следующего шага, микросекунды
     */
    int (*next_step_delay)(int curr_step, void* curve_context);
    
    /** Режим калибровки */
    calibrate_mode_t calibrate_mode;
    
    /**
     * Проверять выход за виртуальные границы координаты при движении (аппаратные проверяются ВСЕГДА).
     * true: останавливать вращение при выходе за допустимые границы координаты (0, max_pos), 
     * false: не проверять границы (сбрасывать current_pos в 0 при каждом шаге).
     */
    //bool check_bounds;    

//// Динамика
    /** Статус цикла */
    motor_cycle_status_t cycle_status;

    /** Счетчик шагов для текущей серии (убывает) */
    int step_counter;
    /** Счетчик микросекунд для текущего шага (убывает) */
    int step_timer;
} motor_cycle_info_t;

#ifndef MAX_STEPPERS
#define MAX_STEPPERS 6
#endif

int stepper_count = 0;
stepper* smotors[MAX_STEPPERS];
motor_cycle_info_t cstatuses[MAX_STEPPERS];

// Частота таймера, мкс
int timer_freq_us;

// Текущий статус цикла
bool cycle_running = false;


/**
 * Подготовить мотор к запуску ограниченной серии шагов - задать нужное количество 
 * шагов и задержку между шагами для регулирования скорости (0 для максимальной скорости).
 * 
 * @param step_count количество шагов, знак задает направление вращения
 * @param step_delay задержка между двумя шагами, микросекунды (0 для максимальной скорости).
 */
void prepare_steps(stepper *smotor, int step_count, int step_delay) {
  
    // резерв нового места на мотор в списке
    int sm_i = stepper_count;
    stepper_count++;
    
    // ссылка на мотор
    smotors[sm_i] = smotor;
        
    // Подготовить движение
  
    // задать направление
    cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(cstatuses[sm_i].dir * smotor->dir_inv > 0) {
        digitalWrite(smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // скорость вращения
    cstatuses[sm_i].delay_source = CONSTANT;
    if(step_delay <= smotors[sm_i]->pulse_delay) {
        // не будем делать шаги чаще, чем может мотор
        cstatuses[sm_i].step_delay = smotors[sm_i]->pulse_delay;
    } else {
        cstatuses[sm_i].step_delay = step_delay;
    }
    
    // выключить режим калибровки
    cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    cstatuses[sm_i].step_counter = cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    cstatuses[sm_i].step_timer = cstatuses[sm_i].step_delay;
    
    // ожидаем пуска
    cstatuses[sm_i].cycle_status = IDLE;
}

/**
 * Подготовить мотор к запуску на вращение - задать направление и задержку между
 * шагами для регулирования скорости (0 для максимальной скорости).
 *
 * @param dir направление вращения: 1 - вращать вперед (увеличиваем curr_pos), -1 - назад (уменьшаем curr_pos).
 * @param step_delay задержка между двумя шагами, микросекунды (0 для максимальной скорости).
 * @param calibrate_mode - режим калибровки
 *     NONE: режим калибровки выключен - останавливать вращение при выходе за виртуальные границы 
 *           рабочей области [min_pos, max_pos] (аппаратные проверяются ВСЕГДА);
 *     CALIBRATE_START_MIN_POS: установка начальной позиции (сбрасывать current_pos в min_pos при каждом шаге);
 *     CALIBRATE_BOUNDS_MAX_POS: установка размеров рабочей области (сбрасывать max_pos в current_pos при каждом шаге).
 */
void prepare_whirl(stepper *smotor, int dir, int step_delay, calibrate_mode_t calibrate_mode) {
    // резерв нового места на мотор в списке
    int sm_i = stepper_count;
    stepper_count++;
    
    // ссылка на мотор
    smotors[sm_i] = smotor;
    
    // Подготовить движение
  
    // задать направление
    cstatuses[sm_i].dir = dir;
    if(cstatuses[sm_i].dir * smotor->dir_inv > 0) {
        digitalWrite(smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем без остановки
    cstatuses[sm_i].non_stop = true;
    
    // скорость вращения
    cstatuses[sm_i].delay_source = CONSTANT;
    if(step_delay <= smotors[sm_i]->pulse_delay) {
        // не будем делать шаги чаще, чем может мотор
        cstatuses[sm_i].step_delay = smotors[sm_i]->pulse_delay;
    } else {
        cstatuses[sm_i].step_delay = step_delay;
    }
    
    // режим калибровки
    cstatuses[sm_i].calibrate_mode = calibrate_mode;
  
    // взводим счетчики
    // задержка перед первым шагом
    cstatuses[sm_i].step_timer = cstatuses[sm_i].step_delay;
    
    // на всякий случай обнулим
    cstatuses[sm_i].step_count = 0;
    cstatuses[sm_i].step_counter = 0;
    
    // ожидаем пуска
    cstatuses[sm_i].cycle_status = IDLE;
}

/**
 * Подготовить мотор к запуску ограниченной серии шагов с переменной скоростью - задержки на каждом 
 * шаге вычисляются заранее, передаются в буфере.
 * 
 * @param step_count количество шагов, знак задает направление вращения
 * @param delay_buffer - массив задержек перед каждым следующим шагом, микросекунды
 */
void prepare_buffered_steps(stepper *smotor, int step_count, int* delay_buffer) {
    // резерв нового места на мотор в списке
    int sm_i = stepper_count;
    stepper_count++;
    
    // ссылка на мотор
    smotors[sm_i] = smotor;
        
    // Подготовить движение
  
    // задать направление
    cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(cstatuses[sm_i].dir * smotor->dir_inv > 0) {
        digitalWrite(smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // скорость вращения
    cstatuses[sm_i].delay_source = BUFFER;
    cstatuses[sm_i].delay_buffer = delay_buffer;
    
    // выключить режим калибровки
    cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    cstatuses[sm_i].step_counter = cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    cstatuses[sm_i].step_timer = cstatuses[sm_i].delay_buffer[0];
    
    // ожидаем пуска
    cstatuses[sm_i].cycle_status = IDLE;
}

/**
 * Подготовить мотор к запуску ограниченной серии шагов с переменной скоростью - задать нужное количество 
 * шагов и указатель на функцию, вычисляющую задержку перед каждым шагом для регулирования скорости.
 * 
 * @param step_count количество шагов, знак задает направление вращения
 * @param curve_context - указатель на объект, содержащий всю необходимую информацию для вычисления
 *     времени до следующего шага
 * @param next_step_delay указатель на функцию, вычисляющую задержка перед следующим шагом, микросекунды
 */
void prepare_curved_steps(stepper *smotor, int step_count, void* curve_context, int (*next_step_delay)(int curr_step, void* curve_context)) {
    // резерв нового места на мотор в списке
    int sm_i = stepper_count;
    stepper_count++;
    
    // ссылка на мотор
    smotors[sm_i] = smotor;
        
    // Подготовить движение
  
    // задать направление
    cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(cstatuses[sm_i].dir * smotor->dir_inv > 0) {
        digitalWrite(smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // скорость вращения
    cstatuses[sm_i].delay_source = DYNAMIC;
    cstatuses[sm_i].curve_context = curve_context;
    cstatuses[sm_i].next_step_delay = next_step_delay;
    
    // выключить режим калибровки
    cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    cstatuses[sm_i].step_counter = cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    cstatuses[sm_i].step_timer = cstatuses[sm_i].next_step_delay(0, cstatuses[sm_i].curve_context);
    
    // ожидаем пуска
    cstatuses[sm_i].cycle_status = IDLE;
}

/**
 * Запустить цикл шагов на выполнение - запускаем таймер, обработчик прерываний
 * отрабатывать подготовленную программу.
 */
void start_stepper_cycle() {
    cycle_running = true;
    
    // включить моторы
    for(int i = 0; i < stepper_count; i++) {
        cstatuses[i].cycle_status = RUNNING;
        digitalWrite(smotors[i]->pin_en, LOW);
    }
    
    // частота ядра PIC32MX - 80МГц=80млн операций в секунду
    // берем базовый TIMER_PRESCALER_1_8, дальше подбираем 
    // частоту под нужный период
    
    // для периода 1 микросекунда (1млн вызовов в секунду):
    // 80000000/8/1000000=10=0xA
    // (уже подглючивает)
//    timer_freq_us = 1;
//    initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 0xA);
    
    // для периода 5 микросекунд (200тыс вызовов в секунду):
    // 80000000/8/200000=50
    //timer_freq_us = 5;
    //initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 50);
    
    // ок для движения по линии, совсем не ок для движения по дуге (90мкс на acos/asin)
    // Запустим таймер с периодом 10 микросекунд (100тыс вызовов в секунду):
    // 80000000/8/100000=100=0x64
    //timer_freq_us = 10;
    //initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 0x64);
    
    // Запустим таймер с периодом 20 микросекунд (50тыс вызовов в секунду):
    // 80000000/8/50000=200
    //timer_freq_us = 20;
    //initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 200);
    
    // Запустим таймер с периодом 80 микросекунд (12.5тыс вызовов в секунду):
    // 80000000/8/12500=200
    //timer_freq_us = 80;
    //initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 800);
    
    // Запустим таймер с периодом 100 микросекунд (12.5тыс вызовов в секунду):
    // 80000000/8/10000=1000
    //timer_freq_us = 100;
    //initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 1000);
    
    
    // ок для движения движения по дуге (90мкс на acos/asin)
    // Запустим таймер с периодом 100 микросекунд (12.5тыс вызовов в секунду):
    // 80000000/8/5000=2000
    timer_freq_us = 200;
    initTimerISR(TIMER3, TIMER_PRESCALER_1_8, 2000);
}

/**
 * Завершить цикл шагов - остановить таймер, обнулить список моторов.
 */
void finish_stepper_cycle() {
    // остановим таймер
    stopTimerISR(TIMER3);
        
    // выключим все моторы
    for(int i = 0; i < stepper_count; i++) {
        // выключить мотор
        digitalWrite(smotors[i]->pin_en, HIGH);
    }
    
    // цикл завершился
    cycle_running = false;
    
    // обнулим список моторов
    stepper_count = 0;
}

/**
 * Текущий статус цикла:
 * true - в процессе выполнения,
 * false - ожидает.
 */
bool is_cycle_running() {
    return cycle_running;
}

/**
 * Отладочная информация о текущем цикле.
 */
void cycle_status(char* status_str) {
    sprintf(status_str, "stepper_count=%d", stepper_count);
    for(int i = 0; i < stepper_count; i++) {
        sprintf(status_str+strlen(status_str), 
            "; cstatuses[%d]: step_count=%d, step_counter=%d, step_timer=%d",
            i, cstatuses[i].step_count, cstatuses[i].step_counter, cstatuses[i].step_timer);
    }
}

/**
 * Обработчик прерывания от таймера - дёргается каждые timer_freq_us микросекунд.
 *
 * Этот код должен быть максимально быстрым, каждая итерация должна обязательно уложиться 
 * в значение timer_freq_us (10мкс на PIC32 без рисования дуг, т.е. без тригонометрии, - ок; 
 * с тригонометрией для дуг таймер должен быть >15мкс) и еще оставить немного времени 
 * на выполнение всяких других задач за пределами таймера из главного цикла программы.
 *
 * В целом, "тяжелые" вычисления будут происходить далеко не на каждой итерации таймера - 
 * только в те моменты, когда нужно сделать очередной шаг мотором и произвести необходимые
 * вычисления для следующего шага - это будет происходить не так часто (плюс-минус раз в 
 * миллисекунду для обычного шагового мотора), большая часть остальных циклов таймера
 * будет быстро проскакивать через серию проверок if.
 *
 * Следует учитывать, что "тяжелые" вычисления для разных моторов могут попасть на одну
 * итерацию таймера, поэтому период следует выбирать исходя из суммы максимальных времен 
 * для всех задействованных в цикле моторов (как вариант - раскидать вычисления
 * для разных моторов на разные итерации таймера, но для этого придется усложнить
 * алгоритм, сейчас не рализовано).
 */
void handle_interrupts(int timer) {
    #ifdef DEBUG_SERIAL
        unsigned long cycle_start = micros();
    #endif // DEBUG_SERIAL
    
    // завершился ли цикл - все моторы закончили движение
    bool finished = true;
    
    for(int i = 0; i < stepper_count; i++) {
        cstatuses[i].step_timer -= timer_freq_us;
        
        // TODO: перенести проверку концевиков ниже к моменту перед поворотом мотора или сразу после поворота,
        // чтобы digitalRead не ел процессор на каждой итерации таймера
        if( (smotors[i]->pin_min != -1 && digitalRead(smotors[i]->pin_min)) 
                || (smotors[i]->pin_max != -1 && digitalRead(smotors[i]->pin_max)) ) {
            // сработал один из аппаратных концевых датчиков - завершаем вращение для этого мотора
            
            // обновим статус мотора
            if(cstatuses[i].dir < 0) {
                cstatuses[i].cycle_status = HARD_END_MIN;
            } else {
                cstatuses[i].cycle_status = HARD_END_MAX;
            }
            
        } else if( cstatuses[i].calibrate_mode == NONE && 
                (cstatuses[i].dir > 0 ? smotors[i]->current_pos + smotors[i]->distance_per_step > smotors[i]->max_pos :
                                        smotors[i]->current_pos - smotors[i]->distance_per_step < smotors[i]->min_pos) ) {
            // не в режиме калибровки и собираемся выйти за виртуальные границы во время предстоящего шага - завершаем вращение для этого мотора
            
            // обновим статус мотора
            if(cstatuses[i].dir < 0) {
                cstatuses[i].cycle_status = SOFT_END_MIN;
            } else {
                cstatuses[i].cycle_status = SOFT_END_MAX;
            }
            
        } else if( cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS &&
                cstatuses[i].dir < 0 && smotors[i]->current_pos - smotors[i]->distance_per_step < smotors[i]->min_pos ) {
            // в режиме калибровки размера рабочей области собираемся сместиться ниже нижней виртуальной границы
            // во время предстоящего шага - завершаем вращение для этого мотора
            
            // обновим статус мотора
            cstatuses[i].cycle_status = SOFT_END_MIN;
            
        } else  if( cstatuses[i].non_stop || cstatuses[i].step_counter > 0 ) {
            // если хотя бы у одного мотора остались шаги или он запущен нон-стоп,
            // то мы еще не закончили
            finished = false;
          
            // Шаг происходит по фронту сигнала HIGH>LOW, ширина ступени HIGH при этом не важна.
            // Поэтому сформируем ступень HIGH за один цикл таймера до сброса в LOW
            if(cstatuses[i].step_timer < timer_freq_us * 2 && cstatuses[i].step_timer >= timer_freq_us) {
                // cstatuses[i].step_timer ~ timer_freq_us с учетом погрешности таймера (timer_freq_us) =>
                // импульс1 - готовим шаг
                digitalWrite(smotors[i]->pin_step, HIGH);
            } else if(cstatuses[i].step_timer < timer_freq_us) {
                // cstatuses[i].step_timer ~ 0 с учетом погрешности таймера (timer_freq_us) =>
                // импульс2 (спустя timer_freq_us микросекунд после импульса1) - совершаем шаг
                digitalWrite(smotors[i]->pin_step, LOW);
                
                // посчитаем шаг
                if(!cstatuses[i].non_stop) {
                    cstatuses[i].step_counter--;
                }
                
                if(cstatuses[i].calibrate_mode == NONE || cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS) {
                    // не калибруем или калибруем ширину рабочего поля
                  
                    // обновим текущее положение координаты
                    if(cstatuses[i].dir > 0) {
                        smotors[i]->current_pos += smotors[i]->distance_per_step;
                    } else {
                        smotors[i]->current_pos -= smotors[i]->distance_per_step;
                    }
                    
                    // калибруем ширину рабочего поля - сдвинем правую границу в текущее положение
                    if(cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS) {
                        smotors[i]->max_pos = smotors[i]->current_pos;
                    }
                } else if(cstatuses[i].calibrate_mode == CALIBRATE_START_MIN_POS) {
                    // режим калибровки начального положения - сбрасываем current_pos в min_pos на каждом шаге
                    smotors[i]->current_pos = smotors[i]->min_pos;
                }
                
                // сделали последний шаг - установим статус мотора
                if(!cstatuses[i].non_stop && cstatuses[i].step_counter == 0) {
                    cstatuses[i].cycle_status = FINISHED_OK;
                    
                    /*
                    // вывод сообщений занимает много времени => таймер выбивается из графика
                    #ifdef DEBUG_SERIAL
                        Serial.print("Finished motor=");
                        Serial.print(smotors[i]->name);
                        Serial.print(".pos:");
                        Serial.print(smotors[i]->current_pos);
                        Serial.print("um, curr time=");
                        Serial.print(millis(), DEC);
                        Serial.println("ms");
                    #endif // DEBUG_SERIAL
                    */
                }
                
                // взведём таймер на новый шаг
                if(cstatuses[i].delay_source == CONSTANT) {
                    // координата движется с постоянной скоростью
                    
                    // взводим таймер на новый шаг
                    cstatuses[i].step_timer = cstatuses[i].step_delay;
                } if(cstatuses[i].delay_source == BUFFER) {
                    // координата движется с переменной скоростью,
                    // значения задержек получаем из буфера
                    
                    // вычислим время до следующего шага (step_counter уже уменьшили)
                    int step_delay = cstatuses[i].delay_buffer[cstatuses[i].step_count - cstatuses[i].step_counter];
                    
                    // не будем делать шаги чаще, чем может мотор
                    // TODO: не очень хорошее место для тихой коррекции задержек в процессе рисования,
                    // лучше выводить ошибку или предупреждение
                    step_delay = step_delay >= smotors[i]->pulse_delay ? step_delay : smotors[i]->pulse_delay;
                    
                    // взводим таймер на новый шаг
                    cstatuses[i].step_timer = step_delay;
                } else if(cstatuses[i].delay_source == DYNAMIC) {
                    // координата движется с переменной скоростью (например, рисуем дугу),
                    // значения задержек вычисляем динамически
                    
                    // вычислим время до следующего шага (step_counter уже уменьшили)
                    int step_delay = cstatuses[i].next_step_delay(cstatuses[i].step_count - cstatuses[i].step_counter, 
                        cstatuses[i].curve_context);
                        
                    // не будем делать шаги чаще, чем может мотор
                    // TODO: не очень хорошее место для тихой коррекции задержек в процессе рисования,
                    // лучше выводить ошибку или предупреждение
                    step_delay = step_delay >= smotors[i]->pulse_delay ? step_delay : smotors[i]->pulse_delay;
                    
                    // взводим таймер на новый шаг
                    cstatuses[i].step_timer = step_delay;
                }
            }
        }
    }
    
    if(finished) {
        // все моторы сделали все шаги, цикл завершился
        finish_stepper_cycle();
    }
    
    // Serial.print заведомо не уложится в период таймера, включать только для 
    // единичных запусков при отладке вручную
    #ifdef DEBUG_SERIAL
        unsigned long cycle_finish = micros();
        unsigned long cycle_time = cycle_finish - cycle_start;
        if(cycle_time >= timer_freq_us) {
            Serial.print("***ERROR: timer handler takes longer than timer period: ");
            Serial.print("cycle time=");
            Serial.print(cycle_time);
            Serial.print("us, timer period=");
            Serial.print(timer_freq_us);
            Serial.println("us");
        }
    #endif // DEBUG_SERIAL
}


