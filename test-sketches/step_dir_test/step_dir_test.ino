
//
//#define STEP_PIN 1
//#define DIR_PIN 0

// CNC-shield
// http://blog.protoneer.co.nz/arduino-cnc-shield/
// X
#define STEP_PIN 2
#define DIR_PIN 5
#define EN_PIN 8

// Y
//#define STEP_PIN 3
//#define DIR_PIN 6
//#define EN_PIN 8

// Z
//#define STEP_PIN 4
//#define DIR_PIN 7
//#define EN_PIN 8

void setup() {
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);

  // EN=0 to enable
  digitalWrite(EN_PIN, 0);
}

//
// Задержка между шагами
// (M0 M1 M2) -> (|||)

//
// 1/1 (___)

// 1/1: пищит и не крутится (и не раскручивается)
//int step_delay_us = 500;

// 1/1: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 600;
//int step_delay_us = 900;

// 1/1: гудит и не крутится (можно раскрутить)
//int step_delay_us = 1000;
//int step_delay_us = 1300;

// 1/1: крутится ок, если притормозить, то может не продолжить
//int step_delay_us = 1400;

// 1/1: крутится ок, если притормозить, то продолжает полюбому
//int step_delay_us = 1500;


//
// 1/2 (|__)

// 1/2: пищит и не крутится (и не раскручивается)
//int step_delay_us = 300;

// 1/2: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 400;

// 1/2: гудит и не крутится (можно раскрутить)
//int step_delay_us = 500;
//int step_delay_us = 610;

// 1/2: крутится ок, если притормозить, то может не продолжить
//int step_delay_us = 620;
//int step_delay_us = 640;

// 1/2: крутится ок, если притормозить, то продолжает полюбому
//int step_delay_us = 650;


//
// 1/4 (_|_)

// 1/4: пищит и не крутится (и не раскручивается)
//int step_delay_us = 100;

// 1/4: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 200;

// 1/4: гудит и не крутится (можно раскрутить)
//int step_delay_us = 250;
//int step_delay_us = 310;

// 1/4: крутится ок, если притормозить, то может не продолжить
//int step_delay_us = 320;

// 1/4: крутится ок, если притормозить, то продолжает полюбому
//int step_delay_us = 330;


//
// 1/8 (||_)

// 1/8: пищит-гудит и не крутится (и не раскручивается)
//int step_delay_us = 50;

// 1/8: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 80;
//int step_delay_us = 110;

// 1/8: гудит и не крутится (можно раскрутить)
//int step_delay_us = 120;
//int step_delay_us = 150;

// 1/8: крутится ок, если притормозить, то может не продолжить
//int step_delay_us = 160;

// 1/8: крутится ок, если притормозить, то продолжает полюбому
// (оптимальный вариант)
//int step_delay_us = 180;

//
// 1/16 (__|)

// 1/16: пищит-гудит и не крутится (и не раскручивается)
//int step_delay_us = 25;

// 1/16: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 40;
//int step_delay_us = 50;

// 1/16: гудит и не крутится (можно раскрутить)
//int step_delay_us = 60;

// 1/16: крутится ок, если притормозить, то может не продолжить
//int step_delay_us = 70;

// 1/16: крутится ок, если притормозить, то продолжает полюбому
// (оптимальный вариант)
//int step_delay_us = 80;


//
// 1/32 (|||)

// 1/32: пищит и не крутится (и не раскручивается)
//int step_delay_us = 10;

// 1/32: гудит и не крутится (условно можно раскрутить - с нескольких проворотов)
//int step_delay_us = 20;

// 1/32: гудит и не крутится (можно раскрутить)
//int step_delay_us = 30;

// 1/32: крутится ок, если притормозить, то продолжает полюбому
// (оптимальный вариант)
int step_delay_us = 40;



// количество шагов в полном обороте
// (M0 M1 M2) -> (|||)

// 1/1 (___)
//int step_count = 200;
// 1/2 (|__)
//int step_count = 400;
// 1/4 (_|_)
//int step_count = 800;
// 1/8 (||_)
//int step_count = 1600;
// 1/16 (__|)
//int step_count = 3200;
// 1/32 (|||)
int step_count = 6400;


// количество полных оборотов
int turns = 1;

// всё за 10 секунд
//int step_delay_us = 10000000/(step_count*turns);

void loop() {
    loop_turns();
    //loop_whirl();
}

void loop_turns() {
    if(turns > 0) {
        for(int i = 0; i < step_count; i++) {
            digitalWrite(STEP_PIN, HIGH);
            delayMicroseconds(5);
            digitalWrite(STEP_PIN, LOW);
            delayMicroseconds(step_delay_us-5);
        }

        turns--;
    }
}

void loop_whirl() {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(step_delay_us-5);
}

