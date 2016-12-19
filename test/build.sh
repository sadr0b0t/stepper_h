#!/bin/sh
gcc -c timer_setup_stub.c
g++ -std=c++11 -c \
    -I. -I./sput-1.4.0 -I../stepper_h/ \
    WProgram.cpp \
    ../stepper_h/stepper.cpp \
    ../stepper_h/stepper_timer.cpp \
    stepper_test.cpp
g++ *.o -o stepper_test

