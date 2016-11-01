
#ifndef REGSTP_H
#define REGSTP_H


#include "lib/db.h"
#include "lib/util.h"
#include "lib/crc.h"
#include "lib/gpio.h"
#include "lib/pid.h"
#include "lib/app.h"
#include "lib/config.h"
#include "lib/timef.h"
#include "lib/udp.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/regstp.h"

#define APP_NAME regstp
#define APP_NAME_STR_P(APP) (""#APP"")
#define APP_NAME_STR APP_NAME_STR_P(regstp)


#ifndef MODE_DEBUG
#define CONF_FILE_P(APP) "/etc/controller/"#APP".conf"
#define CONF_FILE CONF_FILE_P(regstp) 
#endif
#ifdef MODE_DEBUG
#define CONF_FILE "main.conf"
#endif

#define WAIT_RESP_TIMEOUT 1

#define MODE_STR_HEATER "h"
#define MODE_STR_COOLER "c"
#define STATE_STR_INIT "i"
#define STATE_STR_REG "r"
#define STATE_STR_TUNE "t"
#define UNKNOWN_STR "u"

#define STOP_KIND_TIME 't'
#define STOP_KIND_GOAL 'g'
#define STOP_KIND_STR_TIME "t"
#define STOP_KIND_STR_GOAL "g"

#define PROG_LIST_LOOP_ST  {Prog *curr = prog_list.top; while (curr != NULL) {
#define PROG_LIST_LOOP_SP curr = curr->next; }} 

#define INIT_PROG_FIELDS item->pid_list = getProgPIDListFdb(item->id); \
        item->sem_list = getProgSEMListFdb(item->id, slist, elist); \
        item->step_curr.state=OFF; \
        item->state = INIT; \
        item->next = NULL;


#define SELECT_PROG_PID_P(APP) "select "#APP".pid_mapping.prog_pid_id, "#APP".pid.mode, "#APP".pid.kp, "#APP".pid.ki, "#APP".pid.kd  from "#APP".pid_mapping, "#APP".pid where "#APP".pid_mapping.pid_id="#APP".pid.id and "#APP".pid_mapping.prog_id=%d"
#define SELECT_PROG_PID SELECT_PROG_PID_P(regstp)
#define SELECT_PROG_SEM_P(APP) "select "#APP".sem_mapping.prog_sem_id, "#APP".sem.sensor_id, "#APP".sem.em_id from "#APP".sem_mapping, "#APP".sem where "#APP".sem_mapping.sem_id="#APP".sem.id and "#APP".sem_mapping.prog_id=%d"
#define SELECT_PROG_SEM SELECT_PROG_SEM_P(regstp)

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    RUN,
    CREP,
    NSTEP,
    FSTEP,
    BYTIME,
    BYGOAL,
    STOP
} StateAPP;

typedef struct {
    int id;
    int remote_id;
    Peer *source;
    float last_output; //we will keep last output value in order not to repeat the same queries to peers
} EM; //executive mechanism

typedef struct {
    EM *item;
    size_t length;
} EMList;

typedef struct {
    int id;
    int remote_id;
    Peer *source;
    FTS value;
} Sensor;

typedef struct {
    Sensor *item;
    size_t length;
} SensorList;

typedef struct {
    int id;
    PID pid;
} ProgPID;

typedef struct {
    ProgPID *item;
    size_t length;
} ProgPIDList;

typedef struct {
    int id;
    Sensor *sensor;
    EM *em;
} ProgSEM;

typedef struct {
    ProgSEM *item;
    size_t length;
} ProgSEMList;

typedef struct {
    int id;
    Sensor *sensor;
    EM *em;
    float goal;
    struct timespec duration;
    int even_change;
    char stop_kind; //by time or by goal
    PID *pid;
    int next_id;

    char state;
    float output;
    float cgoal;
    Ton_ts tmr;
    char state_ch;
    float goal_correction;
    float temp_start;

    char state_sp;
    int wait_above;
} Step;

struct prog_st {
    int id;
    int first_step;
    Step step_curr;
    int repeat;
    int repeat_infinite;
    ProgPIDList pid_list;
    ProgSEMList sem_list;
    int crepeat;
    char state;
    Mutex mutex;
    Mutex mutex_all;
    struct prog_st *next;
};

typedef struct prog_st Prog;

typedef struct {
    Prog *top;
    Prog *last;
    size_t length;
} ProgList;

typedef struct {
    pthread_attr_t thread_attr;
    pthread_t thread;
    char cmd;
    char qfr;
    int on;
    struct timespec cycle_duration; //one cycle minimum duration
    int created;
    int attr_initialized;
} ThreadData;

extern int readSettings();

extern void initApp();

extern void initData();

extern int initSensor(SensorList *list, const PeerList *pl);

extern int initEM(EMList *list, const PeerList *pl);

extern int addProg(Prog *item, ProgList *list);

extern int addProgById(int id, ProgList *list);

extern int deleteProgById(int id, ProgList *list);

extern int switchProgById(int id, ProgList *list);

extern void loadAllProg(ProgList *list, const SensorList *slist, const EMList *elist);

extern int sensorRead(Sensor *s);

extern int controlEM(EM *em, float output);

extern void serverRun(int *state, int init_state);

extern void secure();

extern void progControl(Prog *item);

extern int stepControl(Step *item);

extern void progNextStep(Prog *item);

extern void progToStep(int step_id, Prog *item);

extern void *threadFunction_ctl(void *arg);

extern int createThread_ctl(ThreadData * td);

extern void freeProg(ProgList *list);

extern void freeThread_ctl();

extern void freeData();

extern void freeApp();

extern void exit_nicely();

extern void exit_nicely_e(char *s);
#endif 

