// сохраненные значение пинов
// для digitalWrite
int dbg_pin_values[64];


/**
 * Сохраненное значение пина после digitalWrite
 * (для отладки)
 */
int dbg_pin_val(int pin) {
    return dbg_pin_values[pin];
}

