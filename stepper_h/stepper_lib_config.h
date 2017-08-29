#ifndef STEPPER_LIB_CONFIG_H
#define STEPPER_LIB_CONFIG_H

// Настройки библиотеки на этапе компиляции
// Compile time library config

// максимальное количество шаговых моторов
// maximun number of stepper motors
#define MAX_STEPPERS 6

// включить отладку через последовательный порт
// enable serial port debug messages
//#define DEBUG_SERIAL


// Настройки таймера
// значения по умолчанию для таймера будут отличаться для разных архитектур,
// универсальный вариант задать пока не получается.

// из stepper_lib_config.h
#ifdef ARDUINO_ARCH_AVR
// AVR 16МГц

// для периода 200 микросекунд (5тыс вызовов в секунду == 5КГц)
// На AVR/Arduino наименьший вариант для движения по линии 3 моторов
// to set timer clock period to 200us (5000 operations per second == 5KHz) on 16MHz CPU
// use prescaler 1:8 (TIMER_PRESCALER_1_8) and adjustment=400:
// 16000000/8/5000 = 2000000/5000 = 400
#define STEPPER_TIMER_DEFAULT_PRESCALER TIMER_PRESCALER_1_8
#define STEPPER_TIMER_DEFAULT_ADJUSTMENT 400
#define STEPPER_TIMER_DEFAULT_PERIOD_US 200

//#endif // ARDUINO_ARCH_AVR
#elif defined( __PIC32__ )
// PIC32MX 80МГц

// для периода 20 микросекунд (50тыс вызовов в секунду == 50КГц):
// На PIC32MX/ChipKIT наименьший вариант для движения по линии 3 моторов
// to set timer clock period to 20us (50000 operations per second == 50KHz) on 80MHz CPU
// use prescaler 1:8 (TIMER_PRESCALER_1_8) and adjustment=200:
// 80000000/8/50000 = 10000000/50000 = 200
#define STEPPER_TIMER_DEFAULT_PRESCALER TIMER_PRESCALER_1_8
#define STEPPER_TIMER_DEFAULT_ADJUSTMENT 200
#define STEPPER_TIMER_DEFAULT_PERIOD_US 20

//#endif // __PIC32__
#else // unknown arch (most likely in test mode)

// test mode: put some values looking like true
// тестовый режим - зададим какие-нибудь правдоподобные значения
#define STEPPER_TIMER_DEFAULT_PRESCALER TIMER_PRESCALER_1_8
#define STEPPER_TIMER_DEFAULT_ADJUSTMENT 0
#define STEPPER_TIMER_DEFAULT_PERIOD_US 10

#endif


#endif // STEPPER_LIB_CONFIG_H

