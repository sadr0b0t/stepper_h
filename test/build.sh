#!/bin/sh
gcc -c timer_setup_stub.c
g++ -std=c++11 -c \
    -I. -I../stepper_h/ -I../stepper_test/ -I../stepper_test/sput-1.4.0 \
    Arduino.cpp \
    ../stepper_h/stepper.cpp \
    ../stepper_h/stepper_timer.cpp \
    ../stepper_test/stepper_test.cpp \
    stepper_test_main.cpp
g++ *.o -o stepper_test

