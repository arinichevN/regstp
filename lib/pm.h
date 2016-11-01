
#ifndef LIBPAS_PM_H
#define LIBPAS_PM_H

#include "gpio.h"
#include "timef.h"

#define PM_BUSY LOW
#define PM_IDLE HIGH
#define PM_RSL 100 //duty cycle should be between 0 and PM_RSL

typedef struct {
    struct timespec busy_time_total;
    struct timespec state_time_curr;
    struct timespec state_time_start;
    int output;
} PMItem;

typedef struct {
    PMItem *item;
    int item_length;
    int initialized;
    int busy_pin_num;
    struct timespec swap_delay;
    struct timespec swap_time_start;
    struct timespec busy_min;
    struct timespec idle_min;
} PM; //parallel modulation

extern void pmctl(PM *d, int duty_cycle) ;

#endif /* PM_H */

