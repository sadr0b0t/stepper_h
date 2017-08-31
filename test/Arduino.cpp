// сохраненные значение пинов
// для digitalWrite
int dbg_pin_values[64];

unsigned long micros() {
    return 0;
}

void pinMode(int pin, int mode) {
}

/**
 * Сохранить значение пина
 */
void digitalWrite(int pin, int val) {
    dbg_pin_values[pin] = val;
}

/**
 * Сохраненное значение пина после digitalWrite
 * (для отладки)
 */
int digitalRead(int pin) {
    return dbg_pin_values[pin];
}

