
#ifndef LIBPAS_PID_H
#define LIBPAS_PID_H

#include <stdlib.h>
#define PID_MODE_HEATER 'h'
#define PID_MODE_COOLER 'c'
#define PID_TUNE_START_VALUE 100.0f
#define PID_TUNE_NOISE 1.0f
#define PID_TUNE_STEP 50.0f
#define PID_TUNE_LOOP_BACK 20
#define NANO_FACTOR 0.000000001



typedef struct {
    float integral_error;
    float previous_error;
    float kp, ki, kd;
    struct timespec previous_time;
    int reset;
    char mode;// PID_MODE_HEATER or PID_MODE_COOLER
} PID;

typedef struct {
    int isMax, isMin;
    float setpoint;
    float noiseBand;
    int controlType;
    int running;
    struct timespec peak1, peak2, lastTime;
    struct timespec sampleTime;
    int nLookBack;
    int peakType;
    float lastInputs[101];
    float peaks[10];
    int peakCount;
    int justchanged;
    int justevaled;
    float absMax, absMin;
    float oStep;
    float outputStart;
    float Ku, Pu;
}PID_AT;


extern float pid(PID *p, float set_point, float input);

extern float pidwt(PID *p, float set_point, float input, struct timespec tm);

extern void stopPid(PID *p);

extern int pidAutoTune(PID_AT *at, PID *p, float input, float *output) ;
#endif /* PID_H */

