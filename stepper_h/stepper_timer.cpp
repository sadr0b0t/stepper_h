/**
 * stepper_timer.cpp 
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс 
 * драйвера "step-dir".
 *
 * LGPLv3, 2014-2016
 *
 * @author Антон Моисеев 1i7.livejournal.com
 */
 
#include "Arduino.h"
#include "stdio.h"
#include "string.h"

extern "C"{
    #include "timer_setup.h"
}

#include "stepper.h"

#include "stepper_lib_config.h"

typedef enum {
    /** Игнорировать проблему, продолжать выполнение */
    IGNORE, 
    
    /** 
     * Попытаться исправить проблему (например, установить ближайшее корректное значение)
     * и продолжить выполнение 
     */
    FIX, 
    
    /** Остановить мотор, продолжить вращение остальных моторов */
    STOP_MOTOR,
    
    /** Завершить выполнение всего цикла - остановить все моторы */
    CANCEL_CYCLE
} error_handle_strategy_t;

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
 * Статус текущего цикла мотора. Серия вращения (главный цикл) мотора состоит из 
 * нескольких циклов (подциклов). Каждый цикл включает фиксированное количество шагов, 
 * настройки для направления и задержек между шагами при вращении.
 */
typedef struct {
    /** Количество циклов в текущей серии */
    int cycle_count = 0;
    
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
     */
    int* delay_buffer;
    
    /**
     * Массив с количеством шагов для каждого цикла серии. Знак задает направление вращения.
     */
    int* step_buffer;
    
    /** 
     * Масштабирование шагов (повтор шагов с одинаковой задержкой при использовании буфера задержек) 
     */
    int scale;
    
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

//// Динамика
    /** Счетчик циклов (возрастает) */
    int cycle_counter = 0;
    
    /** Мотор остановлен в процессе работы */
    bool stopped = false;

    /** Счетчик шагов для текущей серии (убывает) */
    int step_counter = 0;
    
    /** Счетчик микросекунд для текущего шага (убывает) */
    int step_timer = 0;
    
    /** Статус цикла - для внешних потребителей */
    stepper_info_t* stepper_info;
} motor_cycle_info_t;

#ifndef MAX_STEPPERS
#define MAX_STEPPERS 6
#endif

static int _stepper_count = 0;
static stepper* _smotors[MAX_STEPPERS];
static motor_cycle_info_t _cstatuses[MAX_STEPPERS];

// Настройки таймера
// значения по умолчанию для таймера 
// с периодом 200 микросекунд (5тыс вызовов в секунду)
// ок для движения по дуге (по 90мкс на acos/asin)
// 80000000/8/5000=2000
static int _timer_id = TIMER3;
static int _timer_prescaler = TIMER_PRESCALER_1_8;
static int _timer_period = 2000;
// Период таймера, мкс
static int _timer_period_us = 200;

// Текущий статус цикла
static bool _cycle_running = false;
// Цикл на паузе (типа работаем, но шаги не делаем)
static bool _cycle_paused = false;
// для внешнего мира
static stepper_cycle_info_t* _cycle_info;

// Стратегия реакции на ошибки
// STOP_MOTOR/CANCEL_CYCLE
//static error_handle_strategy_t _hard_end_handle = STOP_MOTOR;
static error_handle_strategy_t _hard_end_handle = CANCEL_CYCLE;

// STOP_MOTOR/CANCEL_CYCLE
//static error_handle_strategy_t _soft_end_handle = STOP_MOTOR;
static error_handle_strategy_t _soft_end_handle = CANCEL_CYCLE;

// FIX/STOP_MOTOR/CANCEL_CYCLE/IGNORE
static error_handle_strategy_t _small_pulse_delay_handle = FIX;
//static error_handle_strategy_t _small_pulse_delay_handle = STOP_MOTOR;
//static error_handle_strategy_t _small_pulse_delay_handle = CANCEL_CYCLE;
//static error_handle_strategy_t _small_pulse_delay_handle = IGNORE;

// IGNORE/CANCEL_CYCLE
static error_handle_strategy_t _cycle_timing_exceed_handle = CANCEL_CYCLE;


/**
 * Подготовить мотор к запуску ограниченной серии шагов - задать нужное количество 
 * шагов и задержку между шагами для регулирования скорости (0 для максимальной скорости).
 * 
 * @param step_count количество шагов, знак задает направление вращения
 * @param step_delay задержка между двумя шагами, микросекунды (0 для максимальной скорости)
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_steps(stepper *smotor, int step_count, int step_delay, stepper_info_t *stepper_info) {
    
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
        
    // Подготовить движение
  
    // задать направление
    _cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    _cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    _cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // скорость вращения - постоянная
    _cstatuses[sm_i].delay_source = CONSTANT;
    if(step_delay == 0 ) {
        // 0 - движение с максимальной скоростью
        _cstatuses[sm_i].step_delay = smotor->pulse_delay;
    } else {
        _cstatuses[sm_i].step_delay = step_delay;
    }
    // выключить режим калибровки
    _cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    _cstatuses[sm_i].step_counter = _cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].step_delay;
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
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
 *     CALIBRATE_BOUNDS_MAX_POS: установка размеров рабочей области (сбрасывать max_pos в current_pos при каждом шаге)
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_whirl(stepper *smotor, int dir, int step_delay, calibrate_mode_t calibrate_mode, 
        stepper_info_t *stepper_info) {
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
    
    // Подготовить движение
  
    // задать направление
    _cstatuses[sm_i].dir = dir;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем без остановки
    _cstatuses[sm_i].non_stop = true;
    
    // скорость вращения - постоянная
    _cstatuses[sm_i].delay_source = CONSTANT;
    if(step_delay == 0 ) {
        // 0 - движение с максимальной скоростью
        _cstatuses[sm_i].step_delay = smotor->pulse_delay;
    } else {
        _cstatuses[sm_i].step_delay = step_delay;
    }
    
    // режим калибровки
    _cstatuses[sm_i].calibrate_mode = calibrate_mode;
  
    // взводим счетчики
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].step_delay;
    
    // на всякий случай обнулим
    _cstatuses[sm_i].step_count = 0;
    _cstatuses[sm_i].step_counter = 0;
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
}

/**
 * Подготовить мотор к запуску ограниченной серии шагов с переменной скоростью - задержки на каждом 
 * шаге вычисляются заранее, передаются в буфере delay_buffer.
 * 
 * Масштабирование шага позволяет экономить место в буфере delay_buffer, жертвуя точностью 
 * (минимальной длиной шага в цикле); если цикл содержит серии шагов с одинаковой задержкой,
 * реальноая точность не пострадает. Буфер delay_buffer содержит временные задержки перед каждым следующим шагом.
 * Можно использовать одну и ту же задержку (один элемент буфера) для нескольких последовательных шагов
 * при помощи параметра step_count (масштаб). 
 * 
 * При step_count=1 на каждый элемент буфера delay_buffer ("виртуальный" шаг) мотор будет делать 
 *     один реальный (аппаратный) шаг из delay_buffer.
 * При step_count=2 на каждый элемент буфера delay_buffer (виртуальный шаг) мотор будет делать 
 *     два реальных (аппаратных) шага с одной и той же задержкой из delay_buffer.
 * При step_count=3 на каждый элемент буфера delay_buffer (виртуальный шаг) мотор будет делать 
 *     три реальных (аппаратных) шага с одной и той же задержкой из delay_buffer.
 * 
 * Допустим, в delay_buffer 2 элемента (2 виртуальных шага):
 *     delay_buffer[0]=1000
 *     delay_buffer[1]=2000
 * параметр step_count=3
 * 
 * Мотор сделает 3 аппаратных шага с задержкой delay_buffer[0]=1000 мкс перед каждым шагом и 
 * 3 аппаратных шага с задержкой delay_buffer[1]=2000мкс. Всего 2*3=6 аппаратных шагов, 
 * время на все шаги = 1000*3+2000*3=3000+6000=9000мкс
 * 
 * Значение параметра buf_size указываем 2 (количество элементов в буфере delay_buffer).
 *
 * Аналогичный результат можно достигнуть с delay_buffer[6]
 *     delay_buffer[0]=1000
 *     delay_buffer[1]=1000
 *     delay_buffer[2]=1000
 *     delay_buffer[3]=2000
 *     delay_buffer[4]=2000
 *     delay_buffer[5]=2000
 * step_count=1, buf_size=6
 *
 * Количество аппаратных шагов можно вычислять как buf_size*step_count.
 * 
 * @param buf_size количество элементов в буфере delay_buffer (количество виртуальных шагов)
 * @param delay_buffer - массив задержек перед каждым следующим шагом, микросекунды
 * @param step_count масштабирование шага - количество аппаратных шагов мотора в одном 
 *     виртуальном шаге, знак задает направление вращения мотора.
 * Значение по умолчанию step_count=1: виртуальные шаги соответствуют аппаратным
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_simple_buffered_steps(stepper *smotor, int buf_size, int* delay_buffer, 
        int step_count, stepper_info_t *stepper_info) {
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
        
    // Подготовить движение
  
    // задать направление
    _cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    _cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    _cstatuses[sm_i].step_count = buf_size > 0 ? buf_size*step_count : -buf_size*step_count;
    
    // настройки переменной скорости вращения
    _cstatuses[sm_i].delay_source = BUFFER;
    _cstatuses[sm_i].delay_buffer = delay_buffer;
    _cstatuses[sm_i].scale = step_count;
    
    
    // выключить режим калибровки
    _cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    _cstatuses[sm_i].step_counter = _cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].delay_buffer[0];
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
}

/**
 * @param buf_size количество элементов в буфере delay_buffer
 * @param delay_buffer (step delay buffer) - массив задержек перед каждым следующим шагом, микросекунды
 * @param step_buffer (step count buffer) - массив с количеством шагов для каждого 
 *     значения задержки из delay_buffer. Может содержать положительные и отрицательные значения,
 *     знак задает направление вращения мотора. 
 *     Должен содержать ровно столько же элементов, сколько delay_buffer
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_buffered_steps(stepper *smotor, int buf_size, int* delay_buffer, int* step_buffer, 
        stepper_info_t *stepper_info) {
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
        
    // Подготовить движение
    _cstatuses[sm_i].cycle_count = buf_size;
    _cstatuses[sm_i].cycle_counter = 0;
    _cstatuses[sm_i].step_buffer = step_buffer;

    int step_count = _cstatuses[sm_i].step_buffer[_cstatuses[sm_i].cycle_counter];
    // сделать step_count положительным
    _cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // задать направление
    _cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // скорость вращения - постоянная на каждом цикле
    _cstatuses[sm_i].delay_source = CONSTANT;
    _cstatuses[sm_i].delay_buffer = delay_buffer;
    int step_delay = _cstatuses[sm_i].delay_buffer[_cstatuses[sm_i].cycle_counter];
    if(step_delay <= _smotors[sm_i]->pulse_delay) {
        // не будем делать шаги чаще, чем может мотор
        _cstatuses[sm_i].step_delay = _smotors[sm_i]->pulse_delay;
    } else {
        _cstatuses[sm_i].step_delay = step_delay;
    }
                        
    // Взводим счетчики
    _cstatuses[sm_i].step_counter = _cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].step_delay;
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
}

/**
 * Подготовить мотор к запуску ограниченной серии шагов с переменной скоростью - задать нужное количество 
 * шагов и указатель на функцию, вычисляющую задержку перед каждым шагом для регулирования скорости.
 * 
 * @param step_count количество шагов, знак задает направление вращения
 * @param curve_context - указатель на объект, содержащий всю необходимую информацию для вычисления
 *     времени до следующего шага
 * @param next_step_delay указатель на функцию, вычисляющую задержку перед следующим шагом, микросекунды
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_dynamic_steps(stepper *smotor, int step_count, 
        void* curve_context, int (*next_step_delay)(int curr_step, void* curve_context), 
        stepper_info_t *stepper_info) {
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
        
    // Подготовить движение
  
    // задать направление
    _cstatuses[sm_i].dir = step_count > 0 ? 1 : -1;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем ограниченное количество шагов
    _cstatuses[sm_i].non_stop = false;
    // сделать step_count положительным
    _cstatuses[sm_i].step_count = step_count > 0 ? step_count : -step_count;
    
    // настройки переменной скорости вращения
    _cstatuses[sm_i].delay_source = DYNAMIC;
    _cstatuses[sm_i].curve_context = curve_context;
    _cstatuses[sm_i].next_step_delay = next_step_delay;
    
    // выключить режим калибровки
    _cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    _cstatuses[sm_i].step_counter = _cstatuses[sm_i].step_count;
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].next_step_delay(0, _cstatuses[sm_i].curve_context);
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
}

/**
 * Подготовить мотор к запуску на беспрерывное вращение с переменной скоростью - задать нужное количество 
 * шагов и указатель на функцию, вычисляющую задержку перед каждым шагом для регулирования скорости.
 * 
 * @param dir направление вращения: 1 - вращать вперед, -1 - назад.
 * @param curve_context - указатель на объект, содержащий всю необходимую информацию для вычисления
 *     времени до следующего шага
 * @param next_step_delay указатель на функцию, вычисляющую задержку перед следующим шагом, микросекунды
 * @param stepper_info информация о цикле вращения шагового двигателя, обновляется динамически
 *        в процессе вращения двигателя
 */
void prepare_dynamic_whirl(stepper *smotor, int dir, 
        void* curve_context, int (*next_step_delay)(int curr_step, void* curve_context), 
        stepper_info_t *stepper_info) {
    // резерв нового места на мотор в списке
    int sm_i = _stepper_count;
    _stepper_count++;
    
    // ссылка на мотор
    _smotors[sm_i] = smotor;
    
    // динамический статус мотора
    _cstatuses[sm_i].stepper_info = stepper_info;
        
    // Подготовить движение
  
    // задать направление
    _cstatuses[sm_i].dir = dir;
    if(_cstatuses[sm_i].dir * _smotors[sm_i]->dir_inv > 0) {
        digitalWrite(_smotors[sm_i]->pin_dir, HIGH); // туда
    } else {
        digitalWrite(_smotors[sm_i]->pin_dir, LOW); // обратно
    }
    
    // шагаем без остановки
    _cstatuses[sm_i].non_stop = true;
    
    // настройки переменной скорости вращения
    _cstatuses[sm_i].delay_source = DYNAMIC;
    _cstatuses[sm_i].curve_context = curve_context;
    _cstatuses[sm_i].next_step_delay = next_step_delay;
    
    // выключить режим калибровки
    _cstatuses[sm_i].calibrate_mode = NONE;
  
    // Взводим счетчики
    // задержка перед первым шагом
    _cstatuses[sm_i].step_timer = _cstatuses[sm_i].next_step_delay(0, _cstatuses[sm_i].curve_context);
    
    // на всякий случай обнулим
    _cstatuses[sm_i].step_count = 0;
    _cstatuses[sm_i].step_counter = 0;
    
    // ожидаем пуска
    if(_cstatuses[sm_i].stepper_info != NULL) {
        _cstatuses[sm_i].stepper_info->status = STEPPER_STATUS_IDLE;
       
        // обнулим ошибки
        _cstatuses[sm_i].stepper_info->error_soft_end_min = false;
        _cstatuses[sm_i].stepper_info->error_soft_end_max = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_min = false;
        _cstatuses[sm_i].stepper_info->error_hard_end_max = false;
        _cstatuses[sm_i].stepper_info->error_pulse_delay_small = false;
    }
    
    _cstatuses[sm_i].stopped = false;
}

/**
 * Настроить таймер для шагов.
 * Частота ядра PIC32MX - 80МГц=80млн операций в секунду.
 * Берем базовый предварительный масштаб таймера 
 * (например, TIMER_PRESCALER_1_8), дальше подбираем 
 * частоту под нужный период
 * 
 * Example: to set timer clock period to 20ms (50 operations per second)
 * use prescaler 1:64 (0x0060) and period=0x61A8:
 * 80000000/64/50=25000=0x61A8
 * 
 * для периода 1 микросекунда (1млн вызовов в секунду):
 * // (уже подглючивает)
 * 80000000/8/1000000=10=0xA
 *   target_period_us = 1
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 10
 * 
 * для периода 5 микросекунд (200тыс вызовов в секунду):
 * 80000000/8/1000000=10
 *   target_period_us = 5
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 50
 * 
 * для периода 10 микросекунд (100тыс вызовов в секунду):
 * // ок для движения по линии, совсем не ок для движения по дуге (по 90мкс на acos/asin)
 * 80000000/8/100000=100=0x64
 *   target_period_us = 10
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 100
 *
 * для периода 20 микросекунд (50тыс вызовов в секунду):
 * 80000000/8/50000=200
 *   target_period_us = 20
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 200
 *
 * для периода 80 микросекунд (12.5тыс вызовов в секунду):
 * 80000000/8/12500=200
 *   target_period_us = 80
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 800
 *
 * для периода 100 микросекунд (10тыс вызовов в секунду):
 * 80000000/8/10000=1000
 *   target_period_us = 100
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 1000
 *
 * для периода 200 микросекунд (5тыс вызовов в секунду):
 * // ок для движения по дуге (по 90мкс на acos/asin)
 * 80000000/8/5000=2000
 *   target_period_us = 200
 *   prescalar = TIMER_PRESCALER_1_8 = 8
 *   period = 2000
 *
 * @param target_period_us - целевой период таймера, микросекунды.
 * @param timer - системный идентификатор таймера (должен поддерживаться аппаратно)
 * @param prescalar - предварительный масштаб таймера
 * @param period - значение для периода таймера: делитель частоты таймера
 *     после того, как к ней применен предварительный масштаб (prescaler)
 */
void stepper_configure_timer(int target_period_us, int timer, int prescaler, int period) {
    // не ломать настройки таймера, пока не отарботал старый цикл
    if(_cycle_running) {
        return;
    }
    
    _timer_period_us = target_period_us;
    
    _timer_id = timer;
    _timer_prescaler = prescaler;
    _timer_period = period;
}

/**
 * Запустить цикл шагов на выполнение - запускаем таймер, обработчик прерываний
 * отрабатывать подготовленную программу.
 *
 * @param cycle_info информация о цикле, обновляется динамически в процессе работы цикла
 */
void stepper_start_cycle(stepper_cycle_info_t *cycle_info) {
    // не запускать новый цикл, если старый не отработал
    if(_cycle_running) {
        return;
    }

    _cycle_info = cycle_info;

    _cycle_running = true;
    _cycle_paused = false;
    
    // обновим информацию о цикле для внешнего мира
    if(_cycle_info != NULL) {
        _cycle_info->error_status = CYCLE_ERROR_NONE;
        _cycle_info->is_running = true;
        _cycle_info->is_paused = false;
    }
    
    // включить моторы
    for(int i = 0; i < _stepper_count; i++) {
        // обновим статусы
        if(_cstatuses[i].stepper_info != NULL) {
            _cstatuses[i].stepper_info->status = STEPPER_STATUS_RUNNING;
        }
        // аппаратная ножка Enable->LOW (вкл), если задана
        if(_smotors[i]->pin_en != NO_PIN) {
            digitalWrite(_smotors[i]->pin_en, LOW);
        }
    }
    
    // Запустим таймер с периодом _timer_period_us, для этого
    // должны быть заданы правильные _timer_prescaler и _timer_period
    initTimerISR(_timer_id, _timer_prescaler, _timer_period);
}

/**
 * Завершить цикл шагов - остановить таймер, обнулить список моторов.
 */
void stepper_finish_cycle() {
    // остановим таймер
    stopTimerISR(TIMER3);
        
    // выключим все моторы
    for(int i = 0; i < _stepper_count; i++) {
        // аппаратная ножка Enable->HIGH (выкл), если задана
        if(_smotors[i]->pin_en != NO_PIN) {
            digitalWrite(_smotors[i]->pin_en, HIGH);
        }
        
        // обновим статусы (на случай, если это уже не сделано заранее)
        if(_cstatuses[i].stepper_info != NULL) {
            _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
        }
    }
    
    // цикл завершился
    _cycle_running = false;
    _cycle_paused = false;
    
    // обнулим список моторов
    _stepper_count = 0;
    
    // обновим информацию о цикле для внешнего мира
    if(_cycle_info != NULL) {
        _cycle_info->is_running = false;
        _cycle_info->is_paused = false;
    }
}

/**
 * Поставить вращение на паузу, не прирывая всего цикла
 */
void stepper_pause_cycle() {
    _cycle_paused = true;
    
    // обновим информацию о цикле для внешнего мира
    if(_cycle_info != NULL) {
        _cycle_info->is_paused = true;
    }
}

/**
 * Продолжить вращение, если оно было поставлено на паузу
 */
void stepper_continue_cycle() {
    _cycle_paused = false;
    
    // обновим информацию о цикле для внешнего мира
    if(_cycle_info != NULL) {
        _cycle_info->is_paused = false;
    }
}

/**
 * Текущий статус цикла:
 * true - в процессе выполнения,
 * false - ожидает запуска.
 */
bool stepper_is_cycle_running() {
    return _cycle_running;
}

/**
 * Проверить, на паузе ли цикл:
 * true - цикл на паузе (выполняется)
 * false - цикл не на паузе (выполняется или остановлен).
 */
bool stepper_is_cycle_paused() {
    return _cycle_paused;
}

/**
 * Отладочная информация о текущем цикле.
 */
void stepper_cycle_debug_status(char* status_str) {
    sprintf(status_str, "stepper_count=%d", _stepper_count);
    for(int i = 0; i < _stepper_count; i++) {
        sprintf(status_str+strlen(status_str), 
            "; cstatuses[%d]: step_count=%d, step_counter=%d, step_timer=%d",
            i, _cstatuses[i].step_count, _cstatuses[i].step_counter, _cstatuses[i].step_timer);
    }
}

/**
 * Обработчик прерывания от таймера - дёргается каждые _timer_period_us микросекунд.
 *
 * Этот код должен быть максимально быстрым, каждая итерация должна обязательно уложиться 
 * в значение _timer_period_us (10мкс на PIC32 без рисования дуг, т.е. без тригонометрии, - ок; 
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

    // если на паузе, вообще ничего не трогаем
    if(_cycle_paused) {
        return;
    }

    // вращаем моторы - делаем шаги, как запланировали
    // способы обработки ошибок в процессе: останов с кодом ошибки, игнор, исправление по возможности,
    // код ошибки/предупреждения помещаем в объект со статусом мотора
    // возможные ошибки: 
    // - выход за виртуальные границы координаты (останов всего цикла или запрет движения только одного мотора), 
    // - концевой датчик (останов всего цикла или запрет движения только одного мотора), 
    // - задержка между двумя импульсами меньше, чем оптимальное значение (_smotors[i]->pulse_delay)
    // - время выполнения обработчика таймера превышает задержку между двумя вызовами обработчика по таймеру
    // (код слишком медленный) - лучше останавливать весь цикл с ошибкой

    // засечем время выполнения обработчика
    unsigned long cycle_start = micros();
    
    // завершился ли цикл - все моторы закончили движение
    bool finished = true;
    // завершился ли цикл - что-то пошло не так, сворачиваемся раньше времени
    bool canceled = false;
    
    // цикл по всем моторам
    for(int i = 0; i < _stepper_count && !canceled; i++) {
        _cstatuses[i].step_timer -= _timer_period_us;
        
        if( (_cstatuses[i].non_stop || _cstatuses[i].step_counter > 0) && !_cstatuses[i].stopped) {
        
            // если хотя бы у одного мотора остались шаги или он запущен нон-стоп, при этом
            // не остановлен по другой причине (например, из-за концевого датчика), 
            // то мы еще не закончили
            finished = false;
        
        
            if(_cstatuses[i].step_timer < _timer_period_us*3 && _cstatuses[i].step_timer >= _timer_period_us*2) {
                // >>>За 2 импульса до обнуления таймера
                // проверим пограничные значения координат и концевики непосредственно перед шагом
                // (если все ок, то на следующем импульсе пин мотора пойдет в HIGH, а еще на следующем - в LOW)
                
                
                // различать левый и правый концевой датчик: 
                // при срабатывании левого датчика запрещать движение влево, но разрешать движение вправо,
                // при срабатывании правого датчика запрещать движение вправо, но разрешать движение влево
                // запрет приоритетнее разрешения (если подключить оба датчика в один вход, мотор не будет крутиться вообще)
                // Это важно, т.к. если мы в одном цикле, например, зажали левый концевой датчик и заблокировали
                // мотор, при старте следующего цикла датчик все еще будет нажат и у нас должна быть возможность
                // уйти вправо (влево блок, как и в прошлый раз).
                
                if(_smotors[i]->pin_min != NO_PIN && digitalRead(_smotors[i]->pin_min) && _cstatuses[i].dir < 0) {
                    // сработал левый аппаратный концевой датчик и мы движемся влево -
                    // завершаем вращение для этого мотора
                    _cstatuses[i].stopped = true;
                    
                    // обновим статус мотора
                    if(_cstatuses[i].stepper_info != NULL) {
                        _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                    
                        // обозначим ошибку
                        _cstatuses[i].stepper_info->error_hard_end_min = true;
                    }
                    
                    // как себя вести - остановить только этот мотор (в любом случае) или
                    // сразу завершить весь цикл
                    if(_hard_end_handle == CANCEL_CYCLE) {
                        // завершаем весь цикл
                        canceled = true;
                    } // иначе STOP_MOTOR - останавливается только этот мотор 
                    
                } else if(_smotors[i]->pin_max != NO_PIN && 
                        digitalRead(_smotors[i]->pin_max) && _cstatuses[i].dir > 0) {
                    // сработал правый аппаратный концевой датчик и мы движемся вправо - 
                    // завершаем вращение для этого мотора
                    _cstatuses[i].stopped = true;
                    
                    
                    // обновим статус мотора
                    if(_cstatuses[i].stepper_info != NULL) {
                        _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                        
                        // обозначим ошибку
                        _cstatuses[i].stepper_info->error_hard_end_max = true;
                    }
                    
                    // как себя вести - остановить только этот мотор (в любом случае) или
                    // сразу завершить весь цикл
                    if(_hard_end_handle == CANCEL_CYCLE) {
                        // завершаем весь цикл
                        canceled = true;
                    } // иначе STOP_MOTOR - останавливается только этот мотор
                    
                } else if( _cstatuses[i].calibrate_mode == NONE && 
                        (_cstatuses[i].dir > 0 ? 
                            _smotors[i]->max_end_strategy != INF && 
                                _smotors[i]->current_pos + _smotors[i]->distance_per_step > _smotors[i]->max_pos :
                            _smotors[i]->min_end_strategy != INF && 
                                _smotors[i]->current_pos - _smotors[i]->distance_per_step < _smotors[i]->min_pos) ) {
                    // выход за пределы виртуальной границы:
                    // не в режиме калибровки, включены виртуальные границы координаты и 
                    // собираемся выйти за виртуальные границы во время предстоящего шага - 
                    // завершаем вращение для этого мотора
                    _cstatuses[i].stopped = true;
                    
                    // обновим статус мотора
                    if(_cstatuses[i].stepper_info != NULL) {
                        _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                        
                        // обозначим ошибку
                        if(_cstatuses[i].dir < 0) {
                            _cstatuses[i].stepper_info->error_soft_end_min = true;
                        } else {
                            _cstatuses[i].stepper_info->error_soft_end_max = true;
                        }
                    }
                    
                    // как себя вести - остановить только этот мотор (в любом случае) или
                    // сразу завершить весь цикл
                    if(_soft_end_handle == CANCEL_CYCLE) {
                        // завершаем весь цикл
                        canceled = true;
                    } // иначе STOP_MOTOR - останавливается только этот мотор
                    
                } else if( _cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS &&
                        _cstatuses[i].dir < 0 && 
                        _smotors[i]->current_pos - _smotors[i]->distance_per_step < _smotors[i]->min_pos ) {
                    // в режиме калибровки размера рабочей области при движении влево
                    // собираемся сместиться ниже нижней виртуальной границы
                    // во время предстоящего шага - завершаем вращение для этого мотора
                    _cstatuses[i].stopped = true;
                    
                    // обновим статус мотора
                    if(_cstatuses[i].stepper_info != NULL) {
                        _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                        
                        // обозначим ошибку мотора
                        _cstatuses[i].stepper_info->error_soft_end_min = true;
                    }
                    
                    // как себя вести - остановить только этот мотор (в любом случае) или
                    // сразу завершить весь цикл
                    if(_soft_end_handle == CANCEL_CYCLE) {
                        // завершаем весь цикл
                        canceled = true;
                    } // иначе STOP_MOTOR - останавливается только этот мотор
                    
                }
            } else if(_cstatuses[i].step_timer < _timer_period_us*2 && _cstatuses[i].step_timer >= _timer_period_us) {
                // >>>За 1 импульс до обнуления таймера
                // Шаг происходит по фронту сигнала HIGH>LOW, ширина ступени HIGH при этом не важна.
                // Поэтому сформируем ступень HIGH за один цикл таймера до сброса в LOW
            
                // _cstatuses[i].step_timer ~ _timer_period_us с учетом погрешности таймера (_timer_period_us) =>
                // импульс1 - готовим шаг
                digitalWrite(_smotors[i]->pin_step, HIGH);
            } else if(_cstatuses[i].step_timer < _timer_period_us) {
                // >>>Таймер обнулился
                // Шагаем
                // _cstatuses[i].step_timer ~ 0 с учетом погрешности таймера (_timer_period_us) =>
                // импульс2 (спустя _timer_period_us микросекунд после импульса1) - совершаем шаг
                digitalWrite(_smotors[i]->pin_step, LOW);
                
                // шагнули, отметимся в разных местах и приготовимся к следующему шагу (если он будет)
                
                // посчитаем шаг
                if(!_cstatuses[i].non_stop) {
                    _cstatuses[i].step_counter--;
                }
                
                // Текущее положение координаты
                if(_cstatuses[i].calibrate_mode == NONE || _cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS) {
                    // не калибруем или калибруем ширину рабочего поля
                  
                    // обновим текущее положение координаты
                    if(_cstatuses[i].dir > 0) {
                        _smotors[i]->current_pos += _smotors[i]->distance_per_step;
                    } else {
                        _smotors[i]->current_pos -= _smotors[i]->distance_per_step;
                    }
                    
                    // калибруем ширину рабочего поля - сдвинем правую границу в текущее положение
                    if(_cstatuses[i].calibrate_mode == CALIBRATE_BOUNDS_MAX_POS) {
                        _smotors[i]->max_pos = _smotors[i]->current_pos;
                    }
                } else if(_cstatuses[i].calibrate_mode == CALIBRATE_START_MIN_POS) {
                    // режим калибровки начального положения - сбрасываем current_pos в min_pos на каждом шаге
                    _smotors[i]->current_pos = _smotors[i]->min_pos;
                }
                
                // сделали последний шаг в цикле
                if(!_cstatuses[i].non_stop && _cstatuses[i].step_counter == 0) {
                    // увеличиваем счетчик циклов
                    _cstatuses[i].cycle_counter++;
                
                    // загружаем настройки для нового цикла
                    if (_cstatuses[i].cycle_counter < _cstatuses[i].cycle_count) {
                        // заходим на новый цикл внутри текущей серии
                        int step_count = _cstatuses[i].step_buffer[_cstatuses[i].cycle_counter];
                        // сделать step_count положительным
                        _cstatuses[i].step_count = step_count > 0 ? step_count : -step_count;
    
                        // задать направление
                        _cstatuses[i].dir = step_count > 0 ? 1 : -1;
                        if(_cstatuses[i].dir * _smotors[i]->dir_inv > 0) {
                            digitalWrite(_smotors[i]->pin_dir, HIGH); // туда
                        } else {
                            digitalWrite(_smotors[i]->pin_dir, LOW); // обратно
                        }
                        
                        // скорость вращения
                        int step_delay = _cstatuses[i].delay_buffer[_cstatuses[i].cycle_counter];
                        
                        // Взводим счетчики
                        _cstatuses[i].step_counter = _cstatuses[i].step_count;
                        // задержка перед первым шагом
                        _cstatuses[i].step_timer = _cstatuses[i].step_delay;
    
                    } else {
                        // сделали последний шаг в последнем цикле
                        if(_cstatuses[i].stepper_info != NULL) {
                            _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                        }
                    }
                }
                
                // вычисляем задержку перед следующим шагом
                int step_delay;
                if(_cstatuses[i].delay_source == CONSTANT) {
                    // координата движется с постоянной скоростью
                    step_delay = _cstatuses[i].step_delay;
                } if(_cstatuses[i].delay_source == BUFFER) {
                    // координата движется с переменной скоростью,
                    // значения задержек получаем из буфера
                    
                    // вычислим время до следующего шага (step_counter уже уменьшили)
                    step_delay = _cstatuses[i].delay_buffer[
                          (_cstatuses[i].step_count - _cstatuses[i].step_counter)/_cstatuses[i].scale
                        ];
                } else if(_cstatuses[i].delay_source == DYNAMIC) {
                    // координата движется с переменной скоростью (например, рисуем дугу),
                    // значения задержек вычисляем динамически
                    
                    // вычислим время до следующего шага (step_counter уже уменьшили)
                    step_delay = _cstatuses[i].next_step_delay(
                            _cstatuses[i].step_count - _cstatuses[i].step_counter, 
                            _cstatuses[i].curve_context);
                }
                
                // проверим, корректна ли задержка
                if(step_delay < _smotors[i]->pulse_delay) {
                    // посмотрим, что делать с ошибкой
                    if(_small_pulse_delay_handle == FIX) {
                        #ifdef DEBUG_SERIAL
                            Serial.print("***WARNING: fixing step_delay to match minimal pulse_delay ");
                            Serial.print("step_delay=");
                            Serial.print(step_delay);
                            Serial.print("us, pulse_delay=");
                            Serial.print(_smotors[i]->pulse_delay);
                            Serial.println("us");
                        #endif // DEBUG_SERIAL
                        
                        // попробуем исправить:
                        // не будем делать шаги чаще, чем может мотор
                        // (следует понимать, что корректность вращения уже нарушена)
                        step_delay = _smotors[i]->pulse_delay;
                    } else if(_small_pulse_delay_handle == STOP_MOTOR) {
                        // останавливаем мотор
                        _cstatuses[i].stopped = true;
                        
                        if(_cstatuses[i].stepper_info != NULL) {
                            _cstatuses[i].stepper_info->status = STEPPER_STATUS_FINISHED;
                        }
                    } else if(_small_pulse_delay_handle == CANCEL_CYCLE) {
                        // завершаем весь цикл
                        canceled = true;
                    }
                    // иначе, игнорируем
                   
                    // в любом случае, обозначим ошибку
                    if(_cstatuses[i].stepper_info != NULL) {
                        _cstatuses[i].stepper_info->error_pulse_delay_small = true;
                    }
                }
                
                // взводим таймер на новый шаг с учетом погрешности 
                // (неиспользованных микросекунд) предыдущего шага
                _cstatuses[i].step_timer = step_delay + _cstatuses[i].step_timer;
            }
        }
    }
    
    if(finished || canceled) {
        // все моторы сделали все шаги, цикл завершился
        stepper_finish_cycle();
    }
    
    // проверим, уложились ли в желаемое время
    unsigned long cycle_finish = micros();
    unsigned long cycle_time = cycle_finish - cycle_start;
    if(cycle_time >= _timer_period_us) {
        // обработчик работает дольше, чем таймер генерирует импульсы,
        // тайминг может быть нарушен
        // обновим информацию о цикле для внешнего мира
        if(_cycle_info != NULL) {
            _cycle_info->error_status = CYCLE_ERROR_HANDLER_TIMING_EXCEEDED;
        }
        
        // что с этим делать
        if(_cycle_timing_exceed_handle == CANCEL_CYCLE) {
            // ничего хорошего - все завершаем
            stepper_finish_cycle();
        } // иначе игнорируем
        
        // Serial.print заведомо не уложится в период таймера, включать только для 
        // единичных запусков при отладке вручную
        #ifdef DEBUG_SERIAL
            Serial.print("***ERROR: timer handler takes longer than timer period: ");
            Serial.print("cycle time=");
            Serial.print(cycle_time);
            Serial.print("us, timer period=");
            Serial.print(_timer_period_us);
            Serial.println("us");
        #endif // DEBUG_SERIAL
    }
}

