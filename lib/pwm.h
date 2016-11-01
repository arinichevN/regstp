
#ifndef LIBPAS_PWM_H
#define LIBPAS_PWM_H

#include <time.h>
#include "gpio.h"
#include "timef.h"

#define PWM_BUSY LOW
#define PWM_IDLE HIGH
#define PWM_RSL 100 //duty cycle should be between 0 and PWM_RSL
#define PWM_PERIOD_DEFAULT 10 //(sec)

typedef struct {
    int initialized;
    int state;
    struct timespec start_time;
    struct timespec period; 
} PWM;

extern int pwmctl(PWM *d, int duty_cycle) ;

#endif /* PWM_H */

