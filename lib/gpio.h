#ifndef LIBPAS_GPIO_H
#define LIBPAS_GPIO_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>



#define INPUT    0
#define OUTPUT    1

#define LOW    0
#define HIGH    1


extern void digitalWrite(int pin, int value);
extern int digitalRead(int pin);
extern void pinLow(int pin);
extern void pinHigh(int pin);
extern void pinModeIn(int pin);
extern void pinModeOut(int pin);
//extern int validatePin(int pin);
extern int checkPin(int pin) ;
extern int gpioSetup();


#endif 

