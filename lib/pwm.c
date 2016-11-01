/*
 *  pulse-width modulation 
*/
#include "pwm.h"

int pwmctl(PWM *d, int duty_cycle) {
    struct timespec now, dif, time_req;
    int time_req_sec;
    if (d->period.tv_sec < 0) {
        d->period.tv_sec = 0;
        d->period.tv_nsec = 0;
        return (PWM_IDLE);
    } else if (d->period.tv_sec == 0) {
        return (PWM_IDLE);
    }
    //duty_cycle normalization
    if (duty_cycle < 0) {
        duty_cycle = 0;
    } else if (duty_cycle > PWM_RSL) {
        duty_cycle = PWM_RSL;
    }
    
    clock_gettime(CLOCK_REALTIME_COARSE, &now);
    if (!d->initialized) {
        d->state = PWM_BUSY;
        d->start_time = now;
        d->initialized = 1;
    }
    switch (d->state) {
        case PWM_BUSY:
            timespecsub(&now, &d->start_time, &dif);
            time_req_sec = (duty_cycle * d->period.tv_sec) / PWM_RSL;
            time_req.tv_sec = time_req_sec;
            time_req.tv_nsec = 0;
            if (timespeccmp(&time_req, &dif, <=)) {
                if (duty_cycle != PWM_RSL) {
                    d->state = PWM_IDLE;
                    d->start_time = now;
                }
            }
            break;
        case PWM_IDLE:
            timespecsub(&now, &d->start_time, &dif);
            time_req_sec = ((PWM_RSL - duty_cycle) * d->period.tv_sec) / PWM_RSL;
            time_req.tv_sec = time_req_sec;
            time_req.tv_nsec = 0;
            if (timespeccmp(&time_req, &dif, <=)) {
                if (duty_cycle != 0) {
                    d->state = PWM_BUSY;
                    d->start_time = now;
                }
            }
            break;
        default:
            d->state = PWM_BUSY;
            break;
    }
    return d->state;
}

