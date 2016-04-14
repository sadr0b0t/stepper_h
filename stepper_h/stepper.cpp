/**
 * stepper.cpp 
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс 
 * драйвера "step-dir".
 *
 * LGPL, 2014
 *
 * @author Антон Моисеев
 */
 
#include "WProgram.h"
#include "stepper.h"

/**
 * Инициализировать шаговый мотор необходимыми значениями.
 */
void init_stepper(stepper* smotor,  char name, 
        int pin_step, int pin_dir, int pin_en,
        int dir_inv, int pulse_delay,
        double distance_per_step) {
  
    smotor->name = name;
    
    smotor->pin_step = pin_step;
    smotor->pin_dir = pin_dir;
    smotor->pin_en = pin_en;
    
    smotor->dir_inv = dir_inv;
    smotor->pulse_delay = pulse_delay;
    
    smotor->distance_per_step = distance_per_step;
    
    // задать настройки пинов
    pinMode(pin_step, OUTPUT);
    pinMode(pin_dir, OUTPUT);
    pinMode(pin_en, OUTPUT);
    
    // пока выключить мотор
    digitalWrite(pin_en, HIGH);
}

/**
 * Задать настройки границ рабочей области для шагового мотора.
 */
void init_stepper_ends(stepper* smotor,
        int pin_min, int pin_max,
        end_strategy_t min_end_strategy, end_strategy_t max_end_strategy,
        double min_pos, double max_pos) {
          
    smotor->pin_min = pin_min;
    smotor->pin_max = pin_max;
          
    smotor->min_end_strategy = min_end_strategy;
    smotor->max_end_strategy = max_end_strategy;
    
    smotor->min_pos = min_pos;
    smotor->max_pos = max_pos;
    
    // задать настройки пинов
    if(pin_min != -1) {
        pinMode(pin_min, INPUT);
    }
    if(pin_max != -1) {
        pinMode(pin_max, INPUT);
    }
}


