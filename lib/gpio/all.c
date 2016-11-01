 void digitalWrite(int pin, int value) {
    int v;
    v = pin * value;
}

 int digitalRead(int pin) {
    int regval;
    srand(pin);
    regval = rand();
    regval &= 1;
    return regval;
}

 void pinLow(int pin) {
    int v = pin*pin;
}

 void pinHigh(int pin) {
    int v = pin*pin;
}

 void pinModeIn(int pin) {
    int v = pin*pin;
}

 void pinModeOut(int pin) {
    int v = pin*pin;
}

int checkPin(int pin) {
    return 1;
}

int gpioSetup() {
    return 1;
}


