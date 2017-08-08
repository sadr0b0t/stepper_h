/**
 * stepper.cpp
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс
 * драйвера "step-dir".
 *
 * LGPLv3, 2014-2016
 *
 * @author Антон Моисеев 1i7.livejournal.com
 */

#include "Arduino.h"
#include "stepper.h"

/**
 * Инициализировать шаговый мотор необходимыми значениями.
 * 
 * @param smotor
 * @param name - Имя шагового мотора (один символ: X, Y, Z и т.п.)
 * @param pin_step Подача периодического импульса HIGH/LOW будет вращать мотор
 *     (шаг происходит по фронту HIGH > LOW)
 * @param pin_dir - Направление вращения
 *     1 (HIGH): в одну сторону
 *     0 (LOW): в другую
 *
 *     Для движения вправо (в сторону увеличения значения виртуальной координаты):
 *     при invert_dir==false: запись 1 (HIGH) в pin_dir
 *     при invert_dir==true: запись 0 (LOW) в pin_dir
 * 
 * @param pin_en - вкл (0)/выкл (1) мотор
 *     -1 (NO_PIN): выход не подключен
 * @param invert_dir - Инверсия направления вращения
 *     true: инвертировать направление вращения
 *     false: не инвертировать
 * @param step_delay - Минимальная задержка между импульсами, микросекунды
 *     (для движения с максимальной скоростью)
 * @param distance_per_step - Расстояние, проходимое координатой за шаг,
 *     базовая единица измерения мотора.
 *     
 *     На основе значения distance_per_step счетчик шагов вычисляет
 *     текущее положение рабочей координаты.
 * 
 *     Единица измерения выбирается в зависимости от задачи и свойств
 *     передаточного механизма.
 * 
 *     Если брать базовую единицу изменения за нанометры (1/1000 микрометра),
 *     то диапазон значений для рабочей области будет от нуля в одну сторону:
 *     2^31=2147483648-1 нанометров /1000/1000/1000=2.15 метров
 *     в обе строны: [-2.15м, 2.15м], т.е. всего 4.3 метра.
 *
 *     Для базовой единицы микрометр (микрон) рабочая область
 *     от -2км до 2км, всего 4км.
 */
void init_stepper(stepper* smotor, char name,
        int pin_step, int pin_dir, int pin_en,
        bool invert_dir, int step_delay,
        int distance_per_step) {
    
    smotor->name = name;
    
    smotor->pin_step = pin_step;
    smotor->pin_dir = pin_dir;
    smotor->pin_en = pin_en;
    
    smotor->dir_inv = invert_dir ? -1 : 1;
    smotor->step_delay = step_delay;
    
    smotor->distance_per_step = distance_per_step;
    
    // Значения по умолчанию
    // обнулить текущую позицию
    smotor->current_pos = 0;
    
    // по умолчанию рабочая область не ограничена, концевиков нет
    smotor->pin_min = NO_PIN;
    smotor->pin_max = NO_PIN;
    
    smotor->min_end_strategy = INF;
    smotor->max_end_strategy = INF;
    
    smotor->min_pos = 0;
    smotor->max_pos = 0;
    
    
    // задать настройки пинов
    pinMode(pin_step, OUTPUT);
    pinMode(pin_dir, OUTPUT);
    pinMode(pin_en, OUTPUT);
    
    // пока выключить мотор
    digitalWrite(pin_en, HIGH);
}

/**
 * Задать настройки границ рабочей области для шагового мотора.
 * 
 * Примеры:
 * 1) область с заранее известными границами:
 *   init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, CONST, 0, 100000);
 * 
 * Движение влево ограничено значением min_pos, движение вправо ограничено значением max_pos
 * (min_pos<=curr_pos<=max_pos).
 * 
 * При калибровке начальной позиции мотора CALIBRATE_START_MIN_POS
 * текущее положение мотора curr_pos сбрасывается в значение min_pos (curr_pos=min_pos)
 * на каждом шаге.
 * 
 * При калибровке ширины рабочей области CALIBRATE_BOUNDS_MAX_POS
 * текущее положение мотора curr_pos задает значение max_pos (max_pos=curr_pos)
 * на каждом шаге.
 *
 * 2) область с заранее известной позицией min_pos, значение max_pos не ограничено:
 *   init_stepper_ends(&sm_z, NO_PIN, NO_PIN, CONST, INF, 0, 100000);
 *
 * Движение влево ограничено начальной позицией min_pos (curr_pos не может стать меньше,
 * чем min_pos), движение вправо ничем не ограничено (curr_pos>=min_pos).
 * 
 * @param smotor
 * @param pin_min - номер пина для концевого датчика левой границы
 * @param pin_max - номер пина для концевого датчика правой границы
 * @param min_end_strategy - тип левой виртуальной границы:
 *     CONST - константа, фиксированное минимальное значение координаты
 *     INF - ограничения нет
 * @param max_end_strategy - тип правой виртуальной границы:
 *     CONST - константа, фиксированное максимальное значение координаты
 *     INF - ограничения нет
 * @param min_pos - минимальное значение координаты (для min_end_strategy=CONST)
 * @param max_pos - максимальное значение координаты (для max_end_strategy=CONST)
 */
void init_stepper_ends(stepper* smotor,
        int pin_min, int pin_max,
        end_strategy_t min_end_strategy, end_strategy_t max_end_strategy,
        long min_pos, long max_pos) {
    
    smotor->pin_min = pin_min;
    smotor->pin_max = pin_max;
    
    smotor->min_end_strategy = min_end_strategy;
    smotor->max_end_strategy = max_end_strategy;
    
    smotor->min_pos = min_pos;
    smotor->max_pos = max_pos;
    
    // задать настройки пинов
    if(pin_min != NO_PIN) {
        pinMode(pin_min, INPUT);
    }
    if(pin_max != NO_PIN) {
        pinMode(pin_max, INPUT);
    }
}


