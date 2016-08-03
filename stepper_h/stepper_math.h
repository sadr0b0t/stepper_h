/**
 * stepper.h
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс 
 * драйвера "step-dir" - движение в двумерной системе координат.
 *
 * LGPL, 2014
 *
 * @author Антон Моисеев
 */


#ifndef STEPPER_MATH_H
#define STEPPER_MATH_H

#include<stepper.h>

////
// Математика

/**
 * Подготовить линейное перемещение из текущей позиции в заданную точку с заданной скоростью,
 * для одной координаты.
 *
 * @param sm - мотор на выбранной координате
 * @param dl - сдвиг по указанной оси, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 * 
 */
void prepare_line(stepper *sm, double dl, double spd=0);

/**
 * Подготовить линейное перемещение из текущей позиции в заданную точку с заданной скоростью,
 * для двух координат.
 *
 * @param dl1 - сдвиг по оси 1, мм
 * @param dl2 - сдвиг по оси 2, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 */
void prepare_line_2d(stepper *sm1, stepper *sm2, double dl1, double dl2, double spd=0);


void prepare_circle(stepper *sm1, stepper *sm2, double center_c1, double center_c2, double spd);

void prepare_spiral_circle(stepper *sm1, stepper *sm2, stepper *sm3,double target_c3, double center_c1, double center_c2, double spd);

/**
 * @param target_c1 - целевое значение координаты 1, мм
 * @param target_c2 - целевое значение координаты 2, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 */
void prepare_arc(stepper *sm1, stepper *sm2, double target_c1, double target_c2, double center_c1, double center_c2, double spd);

void prepare_spiral_arc(stepper *sm1, stepper *sm2, stepper *sm3, double target_c1, double target_c2, double target_c3, double center_c1, double center_c2, double spd);

/**
 * @param target_c1 - целевое значение координаты 1, мм
 * @param target_c2 - целевое значение координаты 2, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 */
void prepare_arc2(stepper *sm1, stepper *sm2, double target_c1, double target_c2, double radius, double spd);

void prepare_spiral_arc2(stepper *sm1, stepper *sm2, stepper *sm3, double target_c1, double target_c2, double target_c3, double radius, double spd);

#endif // STEPPER_MATH_H



