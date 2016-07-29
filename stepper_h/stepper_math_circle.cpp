/**
 * stepper_math.cpp 
 *
 * Библиотека управления шаговыми моторами, подключенными через интерфейс 
 * драйвера "step-dir".
 *
 * LGPL v3, 2014-2016
 *
 * @author Антон Моисеев
 */
 
#include "WProgram.h"
#include "stepper.h"

/******************************************************************/
/* Путешествие по дуге окружости */

/**
 * Разная информация о процессе движения точки по дуге окружности.
 */
typedef struct {
    // радиус окружности, мм
    double r;
    
    // скорость перемещения, мм/с
    double f;
    
    // стартовая дочка дуги
    double x_0;
    double y_0;
    
    // целевая точка дуги
    double x_1;
    double y_1;
    
    // шаг по x, микрометры
    double dx;
    // шаг по y, микрометры
    double dy;
    
    ////
    // Вычисляемые значения
    // радиус в микрометрах
    double r_mkm; // = r * 1000;
    // коэффициент для рассчета времени на прохождение пути по координате, микросекунды
    double k; // = r / f * 1000000;
    
    ////
    // Динамические значения
    // значение tx на предыдущей итерации
    int tx_prev;
    // значение ty на предыдущей итерации
    int ty_prev;
    // дополнительная задержка для tx (одноразовая, на старте)
    int tx_delay;
    // дополнительная задержка для ty (одноразовая, на старте)
    int ty_delay;
} circle_context_t;

// контекст для текущей окружности
circle_context_t circle_context;

/**
 * Время до следующего шага по оси X при путешествии по дуге окружности, микросекунды.
 */
int next_step_delay_circle_x(int curr_step, void* circle_context) {
    #ifdef DEBUG_SERIAL
        // время начала вычисления в микросекундах
        unsigned long t1 = micros();
    #endif // DEBUG_SERIAL
        
    circle_context_t* context = (circle_context_t*)circle_context;
    
    // время до достижения целевого x из положения alpha=0 (x=r, y=0), микросекунды
    int tx;
    // время до достижения целевого x из текущего положения, микросекунды
    int dtx;
    
    // целевое положение координаты x
    double x = context->r_mkm - context->dx*(curr_step + 1);
      
    // сколько ехать до новой точки из положения alpha=0, микросекунды
    tx = context->k*acos(x/context->r_mkm);

    // сколько ехать до новой точки из текущего положения, микросекунды
    dtx = tx - context->tx_prev;
    // учтем одноразовую первоначальную задержку
    if(context->tx_delay) {
        dtx += context->tx_delay;
        context->tx_delay = 0;
    }
    
    // сохраним для следующей итерации (чтобы не вычислять два раза)
    context->tx_prev = tx;
    
    /*
    Serial.print("X: curr_step=");
    Serial.print(curr_step);
    Serial.print(", x=");
    Serial.print(x);
    Serial.print(", x/context->r_mkm=");
    Serial.print(x/context->r_mkm);
    Serial.print(", k=");
    Serial.print(context->k);
    Serial.print(", acos(x/context->r_mkm)=");
    Serial.print(acos(x/context->r_mkm));
    Serial.print(", tx=");
    Serial.print(tx);
    Serial.print(", dtx=");
    Serial.println(dtx);*/
    
    
    #ifdef DEBUG_SERIAL
        // время конца вычисления в микросекундах 
        // (в миллисекундах на PIC32 время вычисления будет 0)
        unsigned long t2 = micros();
        unsigned long dt = t2-t1;
    
        if(dt >= 100) {
            Serial.print("#####X dt=");
            Serial.print(dt);
            Serial.print(", x=");
            Serial.print(x);
            Serial.print(", context->r_mkm=");
            Serial.print(context->r_mkm);
            Serial.print(", x/context->r_mkm=");
            Serial.print(x/context->r_mkm);
            Serial.print(", acos(x/context->r_mkm)=");
            Serial.print(acos(x/context->r_mkm));
            Serial.print(", curr_step=");
            Serial.println(curr_step);
        }
    #endif // DEBUG_SERIAL
    
    return dtx;
}

/**
 * Время до следующего шага по оси Y при путешествии по дуге окружности, микросекунды.
 */
int next_step_delay_circle_y(int curr_step, void* circle_context) {
    #ifdef DEBUG_SERIAL
        // время начала вычисления в микросекундах
        unsigned long t1 = micros();
    #endif // DEBUG_SERIAL
    
    circle_context_t* context = (circle_context_t*)circle_context;
    
    // время до достижения целевого y из положения alpha=0 (x=r, y=0)
    double ty;
    // время до достижения целевого y из текущего положения
    double dty;
    
    // целевое положение координаты y, микрометры
    double y = context->dy*(curr_step + 1);
      
    // сколько ехать до новой точки из положения alpha=0
    ty = context->k*asin(y/context->r_mkm);

    // сколько ехать до новой точки из текущего положения
    dty = ty - context->ty_prev;
    // учтем одноразовую первоначальную задержку
    if(context->ty_delay) {
        dty += context->ty_delay;
        context->ty_delay = 0;
    }
    
    // сохраним для следующей итерации (чтобы не вычислять два раза)
    context->ty_prev = ty;
    
    /*Serial.print("Y: curr_step=");
    Serial.print(curr_step);
    Serial.print(", y=");
    Serial.print(y);
    Serial.print(", y/context->r_mkm=");
    Serial.print(y/context->r_mkm);
    Serial.print(", k=");
    Serial.print(context->k);
    Serial.print(", asin(y/context->r_mkm)=");
    Serial.print(asin(y/context->r_mkm));
    Serial.print(", ty=");
    Serial.print(ty);
    Serial.print(", dty=");
    Serial.println(dty);*/
    
    #ifdef DEBUG_SERIAL
        // время конца вычисления в микросекундах 
        // (в миллисекундах на PIC32 время вычисления будет 0)
        unsigned long t2 = micros();
        unsigned long dt = t2-t1;
    
        if(dt >= 100) {
            Serial.print("#####Y dt=");
            Serial.print(dt);
            Serial.print(", curr_step=");
            Serial.println(curr_step);
        }
    #endif // DEBUG_SERIAL
        
    return dty;
}


void prepare_circle(stepper *sm1, stepper *sm2, double center_c1, double center_c2, double spd) {
}

void prepare_spiral_circle(stepper *sm1, stepper *sm2, stepper *sm3,double target_c3, double center_c1, double center_c2, double spd) {
}

/**
 * @param target_c1 - целевое значение координаты 1, мм
 * @param target_c2 - целевое значение координаты 2, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 */
void prepare_arc(stepper *sm1, stepper *sm2, double target_c1, double target_c2, double center_c1, double center_c2, double spd) {
}

void prepare_spiral_arc(stepper *sm1, stepper *sm2, stepper *sm3, double target_c1, double target_c2, double target_c3, double center_c1, double center_c2, double spd) {
}


/**
 * @param target_c1 - целевое значение координаты 1, мм
 * @param target_c2 - целевое значение координаты 2, мм
 * @param spd - скорость перемещения, мм/с, 0 для максимальное скорости
 */
void prepare_arc2(stepper *sm1, stepper *sm2, double target_c1, double target_c2, double radius, double spd) {
    #ifdef DEBUG_SERIAL
        Serial.print("prepare arc:");
        Serial.print(" (");
        Serial.print(sm1->name);
        Serial.print("1=");
        Serial.print(sm1->current_pos / 1000, DEC);
        Serial.print("mm, ");
        Serial.print(sm2->name);
        Serial.print("1=");
        Serial.print(sm2->current_pos / 1000, DEC);
        Serial.print("mm) -> (");
        Serial.print(sm1->name);
        Serial.print("2=");
        Serial.print(target_c1, DEC);
        Serial.print("mm, ");
        Serial.print(sm2->name);
        Serial.print("2=");
        Serial.print(target_c2, DEC);
        Serial.print("mm); speed=");
        Serial.print(spd, DEC);
        Serial.println("mm/s");
    #endif // DEBUG_SERIAL
    
    // y |___
    //   |    \
    //   |     \
    //   |______|_ x
    
    // определим координаты центра окружности, 
    // шаговый мотор1 (мотор_x) sm1 будет x, шаговый мотор2 (мотор_y) sm2 будет y, мм
    double center_x = 0; // !!!TODO = xxx;
    double center_y = 0; // !!!TODO = yyy;
    
    // перейдем в декартову систему координат с центром в начале окружности, мм
    double start_x = 0; // !!!TODO = xxx - center_x;
    double start_y = 0; // !!!TODO = yyy - center_y;
    
    double target_x = target_c1 - center_x;
    double target_y = target_c2 - center_y;
    
    // Заполним первоначальные значения полей для контекста, который будет использоваться 
    // на множестве итераций в процессе рисования до завршения процесса
    
    // радиус окружности, мм
    circle_context.r = radius;
    
    // скорость перемещения, мм/с
    circle_context.f = spd;
    
    // начнем движение из крайней правой точки против часовой стрелки, мм
    circle_context.x_0 = start_x;//circle_context.r;
    circle_context.y_0 = start_y;//0;
    
    // завершим движение в верхней точке окружности, мм
    circle_context.x_1 = target_x;//0;
    circle_context.y_1 = target_y;//circle_context.r;
    
    // шаг по x и y, микрометры
    circle_context.dx = sm1->distance_per_step;
    circle_context.dy = sm2->distance_per_step;
    
    // 
    // вычисляемые значения
    // радиус в микрометрах
    circle_context.r_mkm = circle_context.r * 1000;
    // коэффициент для вычисления задежки, микросекунды
    circle_context.k = 1000000*circle_context.r/circle_context.f;
            
    ///
    // количество шагов по координате
    // todo
    int steps_sm1 = (target_x-start_x)*1000000 / sm1->distance_per_step;
    int steps_sm2 = (target_y-start_y)*1000000 / sm2->distance_per_step;
    
    #ifdef DEBUG_SERIAL
        Serial.print("steps-sm1=");
        Serial.print(steps_sm1);
        Serial.print(", steps-sm2=");
        Serial.println(steps_sm2);
    #endif // DEBUG_SERIAL
    
    
    // учтем возможность старта из произвольной точки дуги (alpha > 0)
    // немного нетривиально: посчитаем, на каком шаге мы бы были, если
    // бы начинали с нулевого угла, причем для x и y номера непройденных
    // шагов будут отличаться; отсчет на запуск моторов начнется со сдвигом
    // так, чтобы учесть их стартовое относительное положение
    
    // виртуальная шкала времени, как если бы движение началось t=0 из точки alpha=0
    // стартовая позиция:
    //     t_start_x
    // ----------|
    //                                t_start_y
    // -----------------------------------|
    // ----------|<-------ty_delay------->|
    // 0--------------------------------------------------> t
    
    // первые шаги по осям
    //       t_start_x   t_(start_x+dx)
    // ----------|--------------|
    //                               t_start_y   t_(start_y+dy)
    // -----------------------------------|---------|
    // ----------|<-------ty_delay------->|
    // 0--------------------------------------------------> t
    
    // но т.к. мы начинаем реальное движение из точки start_x, start_y,
    // первый шаг по x должен произойти через время dtx=t_(start_x+dx)-t_start_x
    // первый шаг по y должен произойти через время dty=t_(start_y+dy)-t_start_y+ty_delay
    // после того, как первый шаг по y будет сделан, значение ty_delay можно сбросить в 0
    
    // количество виртуальных шагов, "пройденных" до реальной стартовой точки
    // todo
    double start_step_x = start_x / sm1->distance_per_step;
    double start_step_y = start_y / sm2->distance_per_step;;
    
    // время, за которое мотор_x должен попасть в положение start_x из положения alpha=0 (x=r, y=0)
    int t_start_x = next_step_delay_circle_x(start_step_x, &circle_context);
    // время, за которое мотор_y должен попасть в положение start_y из положения alpha=0 (x=r, y=0)
    int t_start_y = next_step_delay_circle_y(start_step_y, &circle_context);
    
    // виртуальное время попадания в исходную позицию
    circle_context.tx_prev = t_start_x;
    circle_context.ty_prev = t_start_y;
    
    // если начинаем движение из стартовой точки alpha=0, то значения будут
    //circle_context.tx_prev = 0;
    //circle_context.ty_prev = 0;
    
    // берем за основу меньшее время и устанавливаем первоначальный сдвиг
    if(t_start_x < t_start_y) {
        // мотор_x на первой итерации пойдет сразу
        circle_context.tx_delay = 0;
        circle_context.ty_delay = t_start_y - t_start_x;
    } else {
        circle_context.tx_delay = t_start_x - t_start_y;
        circle_context.ty_delay = 0;
    }
        
    // колбэки для вычисления переменных промежутков между шагами
    int (*next_step_delay_sm1)(int, void*) = &next_step_delay_circle_x;
    int (*next_step_delay_sm2)(int, void*) = &next_step_delay_circle_y;
    
    // подготовим шаги с переменной скоростью
    prepare_curved_steps(sm1, steps_sm1, &circle_context, next_step_delay_sm1);
    prepare_curved_steps(sm2, steps_sm2, &circle_context, next_step_delay_sm2);
}

void prepare_spiral_arc2(stepper *sm1, stepper *sm2, stepper *sm3, double target_c1, double target_c2, double target_c3, double radius, double spd) {
}

