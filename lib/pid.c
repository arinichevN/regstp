#include <time.h>

#include "pid.h"
#include "timef.h"
#include "acp/main.h"

float pid(PID *p, float set_point, float input) {
    struct timespec now;
    double dt;
    static float error, derrivitive_error;
    clock_gettime(LIB_CLOCK, &now);
    if (!p->reset) {
        dt = (float) (now.tv_sec - p->previous_time.tv_sec)+(now.tv_nsec - p->previous_time.tv_nsec) * NANO_FACTOR;
        error = set_point - input;
        p->integral_error += error * dt;
        derrivitive_error = (error - p->previous_error) / dt;
        p->previous_error = error;
        p->previous_time = now;
        switch (p->mode) {
            case PID_MODE_COOLER:
                return p->kp * -1 * error + p->ki * -1 * p->integral_error + p->kd * -1 * derrivitive_error;
                break;
            case PID_MODE_HEATER:
            default:
                return p->kp * error + p->ki * p->integral_error + p->kd * derrivitive_error;
                break;
        }
    } else {
        p->integral_error = 0;
        p->previous_error = 0;
        p->previous_time=now;
        p->reset = 0;
        return 0.0f;
    }
}

float pidwt(PID *p, float set_point, float input, struct timespec tm) {
    double dt;
    static float error, derrivitive_error;
    if (!p->reset) {
        dt = (float) (tm.tv_sec - p->previous_time.tv_sec)+(tm.tv_nsec - p->previous_time.tv_nsec) * NANO_FACTOR;
        error = set_point - input;
        p->integral_error += error * dt;
        derrivitive_error = (error - p->previous_error) / dt;
        p->previous_error = error;
        p->previous_time = tm;
        switch (p->mode) {
            case PID_MODE_COOLER:
                return p->kp * -1 * error + p->ki * -1 * p->integral_error + p->kd * -1 * derrivitive_error;
                break;
            case PID_MODE_HEATER:
            default:
                return p->kp * error + p->ki * p->integral_error + p->kd * derrivitive_error;
                break;
        }
    } else {
        p->integral_error = 0;
        p->previous_error = 0;
       p->previous_time=tm;
        p->reset = 0;
        return 0.0f;
    }
}

void stopPid(PID *p) {
    p->reset = 1;
}

/* algorithm from Arduino PID AutoTune Library - Version 0.0.1 by Brett Beauregard <br3ttb@gmail.com> brettbeauregard.com */
int pidAutoTune(PID_AT *at, PID *p, float input, float *output) {
    struct timespec now, dif;
    at->justevaled = 0;
    if (at->peakCount > 9 && at->running) {
        *output = at->outputStart;
        //we can generate tuning parameters!
        at->Ku = 4 * (2 * at->oStep) / ((at->absMax - at->absMin)*3.14159);
        at->Pu = (float) ((at->peak1.tv_sec - at->peak2.tv_sec)+(at->peak1.tv_nsec - at->peak2.tv_nsec) * NANO_FACTOR) / 1000;
        p->kp = 0.6 * at->Ku;
        p->ki = 1.2 * at->Ku / at->Pu;
        p->kd = 0.075 * at->Ku * at->Pu;
        at->running = 0;
        return 1;
    }
    clock_gettime(LIB_CLOCK, &now);
    timespecsub(&now, &at->lastTime, &dif);
    if (timespeccmp(&dif, &at->sampleTime, <)) return 0;
    at->lastTime = now;
    at->justevaled = 1;
    if (!at->running) { //initialize working variables the first time around
        *output = PID_TUNE_START_VALUE;
        at->noiseBand = PID_TUNE_NOISE;
        at->oStep = PID_TUNE_STEP;
        int value = PID_TUNE_LOOP_BACK;
        if (value < 1) value = 1;
        if (value < 25) {
            at->nLookBack = value * 4;
            at->sampleTime = (struct timespec){250, 0};
        } else {
            at->nLookBack = 100;
            at->sampleTime = (struct timespec){value * 10, 0};
        }
        at->peakType = 0;
        at->peakCount = 0;
        at->justchanged = 0;
        at->absMax = input;
        at->absMin = input;
        at->setpoint = input;
        at->running = 1;
        at->outputStart = *output;
        *output = at->outputStart + at->oStep;
    } else {
        if (input > at->absMax)at->absMax = input;
        if (input < at->absMin)at->absMin = input;
    }

    //oscillate the output base on the input's relation to the setpoint

    if (input > at->setpoint + at->noiseBand) {
        *output = at->outputStart - at->oStep;
    } else if (input < at->setpoint - at->noiseBand) {
        *output = at->outputStart + at->oStep;
    }
    at->isMax = 1;
    at->isMin = 1;
    //id peaks
    int i;
    for (i = at->nLookBack - 1; i >= 0; i--) {
        float val = at->lastInputs[i];
        if (at->isMax) at->isMax = input > val;
        if (at->isMin) at->isMin = input < val;
        at->lastInputs[i + 1] = at->lastInputs[i];
    }
    at->lastInputs[0] = input;
    if (at->nLookBack < 9) { //we don't want to trust the maxes or mins until the inputs array has been filled
        return 0;
    }

    if (at->isMax) {
        if (at->peakType == 0)at->peakType = 1;
        if (at->peakType == -1) {
            at->peakType = 1;
            at->justchanged = 1;
            at->peak2 = at->peak1;
        }
        at->peak1 = now;
        at->peaks[at->peakCount] = input;

    } else if (at->isMin) {
        if (at->peakType == 0) {
            at->peakType = -1;
        }
        if (at->peakType == 1) {
            at->peakType = -1;
            at->peakCount++;
            at->justchanged = 1;
        }
        if (at->peakCount < 10) {
            at->peaks[at->peakCount] = input;
        }
    }

    if (at->justchanged && at->peakCount > 2) { //we've transitioned.  check if we can autotune based on the last peaks
        float avgSeparation = (abs(at->peaks[at->peakCount - 1] - at->peaks[at->peakCount - 2]) + abs(at->peaks[at->peakCount - 2] - at->peaks[at->peakCount - 3])) / 2;
        if (avgSeparation < 0.05 * (at->absMax - at->absMin)) {
            *output = at->outputStart;
            //we can generate tuning parameters!
            at->Ku = 4 * (2 * at->oStep) / ((at->absMax - at->absMin)*3.14159);
            at->Pu = (float) ((at->peak1.tv_sec - at->peak2.tv_sec)+(at->peak1.tv_nsec - at->peak2.tv_nsec)) / 1000;
            p->kp = 0.6 * at->Ku;
            p->ki = 1.2 * at->Ku / at->Pu;
            p->kd = 0.075 * at->Ku * at->Pu;
            at->running = 0;
            return 1;

        }
    }
    at->justchanged = 0;
    return 0;
}