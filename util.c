
#include "main.h"

Peer *getPeerById(char *id, const PeerList *list) {
    LIST_GET_BY_IDSTR
}

Sensor * getSensorById(int id, const SensorList *list) {
    LIST_GET_BY_ID
}

Prog * getProgById(int id, const ProgList *list) {
    LLIST_GET_BY_ID(Prog)
}

EM * getEMById(int id, const EMList *list) {
    LIST_GET_BY_ID
}

int checkSensor(const SensorList *list) {
    size_t i, j;
    for (i = 0; i < list->length; i++) {
        if (list->item[i].source == NULL) {
            fprintf(stderr, "ERROR: checkSensorList: no data source where id = %d\n", list->item[i].id);
            return 0;
        }
    }
    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "ERROR: checkSensorList: id is not unique where id = %d\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

int checkEM(const EMList *list) {
    size_t i, j;
    for (i = 0; i < list->length; i++) {
        if (list->item[i].source == NULL) {
            fprintf(stderr, "ERROR: checkEm: no data source where id = %d\n", list->item[i].id);
            return 0;
        }
    }
    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "ERROR: checkEm: id is not unique where id = %d\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

int checkProg(const Prog *item, const ProgList *list) {
    if (item->repeat < 0) {
        fprintf(stderr, "ERROR: checkProg: negative repeat where prog id = %d\n", item->id);
        return 0;
    }
    //unique id
    if (getProgById(item->id, list) != NULL) {
        fprintf(stderr, "ERROR: checkProg: prog with id = %d is already running\n", item->id);
        return 0;
    }
    return 1;
}

int checkStep(Step *item) {
    if (item->em == NULL) {
        fprintf(stderr, "ERROR: checkStep: no EM attached to step with id = %d\n", item->id);
        return 0;
    }
    if (item->sensor == NULL) {
        fprintf(stderr, "ERROR: checkStep: no sensor attached to step with id = %d\n", item->id);
        return 0;
    }
    switch (item->pid->mode) {
        case PID_MODE_HEATER:
        case PID_MODE_COOLER:
            break;
        default:
            fprintf(stderr, "ERROR: checkStep: bad PID mode for step with id = %d\n", item->id);
            return 0;
    }
    if (item->duration.tv_sec < 0 && item->duration.tv_nsec < 0) {
        fprintf(stderr, "ERROR: checkStep: negative duration where step id = %d\n", item->id);
        return 0;
    }
    switch (item->stop_kind) {
        case STOP_KIND_TIME:
        case STOP_KIND_GOAL:
            break;
        default:
            fprintf(stderr, "ERROR: checkStep: bad stop_kind for step with id = %d ('t' or 'g' expected)\n", item->id);
            return 0;
    }
    return 1;
}

PID *getProgPIDById(int id, const Prog *prog) {
    size_t i;
    for (i = 0; i < prog->pid_list.length; i++) {
        if (prog->pid_list.item[i].id == id) {
            return &prog->pid_list.item[i].pid;
        }
    }
    return NULL;
}

Sensor *getProgSensorById(int id, const Prog *prog) {
    size_t i;
    for (i = 0; i < prog->sem_list.length; i++) {
        if (prog->sem_list.item[i].id == id) {
            return prog->sem_list.item[i].sensor;
        }
    }
#ifdef MODE_DEBUG
    fprintf(stderr, "ERROR: getProgSensorById: sensor not found for prog with id=%d and SEM with id=%d\n", prog->id, id);
#endif
    return NULL;
}

EM *getProgEMById(int id, const Prog *prog) {
    size_t i;
    for (i = 0; i < prog->sem_list.length; i++) {
        if (prog->sem_list.item[i].id == id) {
            return prog->sem_list.item[i].em;
        }
    }
#ifdef MODE_DEBUG
    fprintf(stderr, "ERROR: getProgEMById: EM not found for prog with id=%d and SEM with id=%d\n", prog->id, id);
#endif
    return NULL;
}

int getStepByIdFdb(int id, const Prog *prog, Step *item) {
    PGresult *r;
    char q[LINE_SIZE];
    item->state = OFF;
    snprintf(q, sizeof q, "select next_id, prog_pid_id, prog_sem_id, goal, extract(epoch from duration), even_change, stop_kind from %s.step where id=%d", APP_NAME_STR, id);
    if ((r = dbGetDataT(*db_connp_data, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "ERROR: getStepByIdFdb: only one tuple expected where id=%d\n", id);
#endif
        PQclear(r);
        return 0;
    }
    item->id = id;
    item->next_id = atoi(PQgetvalue(r, 0, 0));
    item->pid = getProgPIDById(atoi(PQgetvalue(r, 0, 1)), prog);
    item->sensor = getProgSensorById(atoi(PQgetvalue(r, 0, 2)), prog);
    item->em = getProgEMById(atoi(PQgetvalue(r, 0, 2)), prog);
    item->goal = atof(PQgetvalue(r, 0, 3));
    item->duration.tv_sec = atoi(PQgetvalue(r, 0, 4));
    item->even_change = atoi(PQgetvalue(r, 0, 5));
    memcpy(&item->stop_kind, PQgetvalue(r, 0, 6), sizeof item->stop_kind);
    PQclear(r);

    if (!checkStep(item)) {
        return 0;
    }
    item->state = INIT;

    return 1;
}

int checkProgPID(const ProgPIDList *list) {
    size_t i, j;
    for (i = 0; i < list->length; i++) {
        switch (list->item[i].pid.mode) {
            case PID_MODE_HEATER:
            case PID_MODE_COOLER:
                break;
            default:
                fprintf(stderr, "ERROR: checkProgPID: pid.mode is bad where id = %d\n", list->item[i].id);
                return 0;
        }
    }

    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "ERROR: checkProgPID: id is not unique where id = %d\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

ProgPIDList getProgPIDListFdb(int prog_id) {
    ProgPIDList list = {.item = NULL, .length = 0};
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, SELECT_PROG_PID, prog_id);
    if ((r = dbGetDataT(*db_connp_data, q, q)) == NULL) {
        return list;
    }
    list.length = PQntuples(r);
    if (list.length > 0) {
        list.item = (ProgPID *) malloc(list.length * sizeof *(list.item));
        if (list.item == NULL) {
            list.length = 0;
            fputs("ERROR: getProgPIDListFdb: failed to allocate memory\n", stderr);
            PQclear(r);
            list.length = 0;
            return list;
        }
        size_t i;
        for (i = 0; i < list.length; i++) {
            list.item[i].id = atoi(PQgetvalue(r, i, 0));
            memcpy(&list.item[i].pid.mode, PQgetvalue(r, i, 1), sizeof list.item[i].pid.mode);
            list.item[i].pid.kp = atof(PQgetvalue(r, i, 2));
            list.item[i].pid.ki = atof(PQgetvalue(r, i, 3));
            list.item[i].pid.kd = atof(PQgetvalue(r, i, 4));
        }
    }
    PQclear(r);
    if (!checkProgPID(&list)) {
        FREE_LIST(&list);
        return list;
    }
    return list;
}

int checkProgSEM(const ProgSEMList *list) {
    size_t i, j;
    for (i = 0; i < list->length; i++) {
        if (list->item[i].em == NULL) {
            fprintf(stderr, "ERROR: checkProgSEM: no EM where id = %d\n", list->item[i].id);
            return 0;
        }
        if (list->item[i].sensor == NULL) {
            fprintf(stderr, "ERROR: checkProgSEM: no sensor where id = %d\n", list->item[i].id);
            return 0;
        }
    }
    //unique id
    for (i = 0; i < list->length; i++) {
        for (j = i + 1; j < list->length; j++) {
            if (list->item[i].id == list->item[j].id) {
                fprintf(stderr, "ERROR: checkProgSEM: id is not unique where id = %d\n", list->item[i].id);
                return 0;
            }
        }
    }
    return 1;
}

ProgSEMList getProgSEMListFdb(int prog_id, const SensorList *slist, const EMList *elist) {
    ProgSEMList list = {NULL, 0};
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, SELECT_PROG_SEM, prog_id);
    if ((r = dbGetDataT(*db_connp_data, q, q)) == NULL) {
        return list;
    }
    list.length = PQntuples(r);
    if (list.length > 0) {
        list.item = (ProgSEM *) malloc(list.length * sizeof *(list.item));
        if (list.item == NULL) {
            list.length = 0;
#ifdef MODE_DEBUG
            fputs("ERROR: getProgSEMListFdb: failed to allocate memory\n", stderr);
#endif
            PQclear(r);
            list.length = 0;
            return list;
        }
        size_t i;
        for (i = 0; i < list.length; i++) {
            list.item[i].id = atoi(PQgetvalue(r, i, 0));
            list.item[i].sensor = getSensorById(atoi(PQgetvalue(r, i, 1)), &sensor_list);
            list.item[i].em = getEMById(atoi(PQgetvalue(r, i, 2)), &em_list);
        }
    }
    PQclear(r);
    if (!checkProgSEM(&list)) {
        free(list.item);
        list.item = NULL;
        list.length = 0;
        return list;
    }
    return list;
}

Prog * getProgByIdFdb(int id, const SensorList *slist, const EMList *elist) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select first_step, repeat, repeat_infinite from %s.prog where id=%d", APP_NAME_STR, id);
    if ((r = dbGetDataT(*db_connp_data, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) != 1) {
#ifdef MODE_DEBUG
        fputs("ERROR: getProgByIdFdb: only one tuple expected\n", stderr);
#endif
        PQclear(r);
        return NULL;
    }

    Prog *item = (Prog *) malloc(sizeof *(item));
    if (item == NULL) {
#ifdef MODE_DEBUG
        fputs("ERROR: getProgByIdFdb: failed to allocate memory\n", stderr);
#endif
        PQclear(r);
        return NULL;
    }
    item->id = id;
    item->first_step = atoi(PQgetvalue(r, 0, 0));
    item->repeat = atoi(PQgetvalue(r, 0, 1));
    item->repeat_infinite = atoi(PQgetvalue(r, 0, 2));
    PQclear(r);
    INIT_PROG_FIELDS
    if (!initMutex(&item->mutex)) {
        FREE_LIST(&item->pid_list);
        FREE_LIST(&item->sem_list);
        free(item);
        return NULL;
    }
    if (!initMutex(&item->mutex_all)) {
        FREE_LIST(&item->pid_list);
        FREE_LIST(&item->sem_list);
        free(item);
        return NULL;
    }
    if (!checkProg(item, &prog_list)) {
        FREE_LIST(&item->pid_list);
        FREE_LIST(&item->sem_list);
        free(item);
        return NULL;
    }
    return item;
}

int lockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_lock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG

        perror("ERROR: lockProgList: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_trylock(&(progl_mutex.self)) != 0) {

        return 0;
    }
    return 1;
}

int unlockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_unlock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG

        perror("ERROR: unlockProgList: error unlocking mutex");
#endif 
        return 0;
    }
    return 1;
}

int lockProg(Prog *item) {
    if (pthread_mutex_lock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("ERROR: lockProg: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProg(Prog *item) {
    if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProg(Prog *item) {
    if (pthread_mutex_unlock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("ERROR: unlockProg: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int lockProgA(Prog *item) {
    if (pthread_mutex_lock(&(item->mutex_all.self)) != 0) {
#ifdef MODE_DEBUG
        perror("ERROR: lockProgA: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProgA(Prog *item) {
    if (pthread_mutex_trylock(&(item->mutex_all.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProgA(Prog *item) {
    if (pthread_mutex_unlock(&(item->mutex_all.self)) != 0) {
#ifdef MODE_DEBUG
        perror("ERROR: unlockProgA: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int lockProgAll() {
    int f = 0;
    PROG_LIST_LOOP_ST
    if (!lockProgA(curr)) {
        f = 1;
    }
    PROG_LIST_LOOP_SP
    if (f) {
        PROG_LIST_LOOP_ST
        unlockProgA(curr);
        PROG_LIST_LOOP_SP
        return 0;
    }
    return 1;
}

int unlockProgAll() {
    PROG_LIST_LOOP_ST
    unlockProgA(curr);
    PROG_LIST_LOOP_SP
    return 1;
}

int bufCatProg(const Prog *item, char *buf, size_t buf_size) {
    char q[LINE_SIZE];
    struct timespec dif;
    if (item->step_curr.stop_kind == STOP_KIND_TIME) {
        dif = getTimePassed_ts(item->step_curr.tmr.start);
    } else {
        dif.tv_sec = -1;
        dif.tv_nsec = -1;
    }
    snprintf(q, sizeof q, "%d_%hhd_%d_%d_%hhd_%hhd_%hhd_%f_%ld\n",
            item->id,
            item->state,
            item->crepeat,
            item->step_curr.id,
            item->step_curr.state,
            item->step_curr.state_ch,
            item->step_curr.state_sp,
            item->step_curr.cgoal,
            dif.tv_sec
            );
    if (bufCat(buf, q, buf_size) == NULL) {
        return 0;
    }
    return 1;
}

int sendStrPack(char qnf, char *cmd) {
    extern size_t udp_buf_size;
    extern Peer peer_client;

    return acp_sendStrPack(qnf, cmd, udp_buf_size, &peer_client);
}

int sendBufPack(char *buf, char qnf, const char *cmd_str) {
    extern size_t udp_buf_size;
    extern Peer peer_client;

    return acp_sendBufPack(buf, qnf, cmd_str, udp_buf_size, &peer_client);
}

void waitThread_ctl(char *cmd) {
    if (thread_data_ctl.on) {
        waitThreadCmd(&thread_data_ctl.cmd, &thread_data_ctl.qfr, cmd);
    }
}

void sendStr(const char *s, uint8_t *crc) {

    acp_sendStr(s, crc, &peer_client);
}

void sendFooter(int8_t crc) {

    acp_sendFooter(crc, &peer_client);
}

char * getProgState(char in) {
    static char *str;
    switch (in) {
        case INIT:
            str = "INIT";
            break;
        case CREP:
            str = "CREP";
            break;
        case FSTEP:
            str = "FSTEP";
            break;
        case RUN:
            str = "RUN";
            break;
        case NSTEP:
            str = "NSTEP";
            break;
        case OFF:
            str = "OFF";
            break;
        default:
            str = "?";
            break;
    }
    return str;
}

char * getStepState(char in) {
    static char *str;
    switch (in) {
        case INIT:
            str = "INIT";
            break;
        case RUN:
            str = "RUN";
            break;
        case OFF:
            str = "OFF";
            break;
        default:
            str = "?";
            break;
    }
    return str;
}

char * getStepStateCh(char in) {
    static char *str;
    switch (in) {
        case INIT:
            str = "INIT";
            break;
        case RUN:
            str = "RUN";
            break;
        case OFF:
            str = "OFF";
            break;
        default:
            str = "?";
            break;
    }
    return str;
}

char * getStepStateSp(char in) {
    static char *str;
    switch (in) {
        case INIT:
            str = "INIT";
            break;
        case BYTIME:
            str = "BYTIME";
            break;
        case BYGOAL:
            str = "BYGOAL";
            break;
        default:
            str = "?";
            break;
    }
    return str;
}

char * getStepStopKind(char in) {
    static char *str;
    switch (in) {
        case STOP_KIND_TIME:
            str = "by time";
            break;
        case STOP_KIND_GOAL:
            str = "by goal";
            break;
        default:
            str = "?";
            break;
    }
    return str;
}

void printAll(ProgList *list, PeerList *pl, EMList *el, SensorList *sl) {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    size_t i;

    sendStr("+-------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                  Peer                                   |\n", &crc);
    sendStr("+--------------------------------+-----------+----------------+-----------+\n", &crc);
    sendStr("|               id               |  udp_port |      addr      |     fd    |\n", &crc);
    sendStr("+--------------------------------+-----------+----------------+-----------+\n", &crc);
    for (i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%32s|%11u|%16u|%11d|\n",
                pl->item[i].id,
                pl->item[i].addr.sin_port,
                pl->item[i].addr.sin_addr.s_addr,
                *pl->item[i].fd
                );
        sendStr(q, &crc);
    }
    sendStr("+--------------------------------+-----------+----------------+-----------+\n", &crc);

    sendStr("+--------------------------------------------------------+\n", &crc);
    sendStr("|                          EM                            |\n", &crc);
    sendStr("+-----------+-----------+--------------------------------+\n", &crc);
    sendStr("|     id    | remote_id |             peer_id            |\n", &crc);
    sendStr("+-----------+-----------+--------------------------------+\n", &crc);
    for (i = 0; i < el->length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%32s|\n",
                el->item[i].id,
                el->item[i].remote_id,
                el->item[i].source->id
                );
        sendStr(q, &crc);
    }
    sendStr("+-----------+-----------+--------------------------------+\n", &crc);

    sendStr("+---------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                     Sensor                                        |\n", &crc);
    sendStr("+-----------+-----------+--------------------------------+------------------------------------------+\n", &crc);
    sendStr("|           |           |                                |                   value                  |\n", &crc);
    sendStr("|           |           |                                |-----------+-----------+-----------+------+\n", &crc);
    sendStr("|     id    | remote_id |             peer_id            |   temp    |   sec     |   nsec    | state|\n", &crc);
    sendStr("+-----------+-----------+--------------------------------+-----------+-----------+-----------+------+\n", &crc);
    for (i = 0; i < sl->length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%32s|%11f|%11ld|%11ld|%6d|\n",
                sl->item[i].id,
                sl->item[i].remote_id,
                sl->item[i].source->id,
                sl->item[i].value.value,
                sl->item[i].value.tm.tv_sec,
                sl->item[i].value.tm.tv_nsec,
                sl->item[i].value.state
                );
        sendStr(q, &crc);
    }
    sendStr("+-----------+-----------+--------------------------------+-----------+-----------+-----------+------+\n", &crc);


    sendStr("+--------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                        Program                                             |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+--------------------------------+\n", &crc);
    sendStr("|    id     | f_step_id |   repeat  |  rep_inf  |   crepeat |              state             |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+--------------------------------+\n", &crc);
    PROG_LIST_LOOP_ST
            char *state = getProgState(curr->state);
    snprintf(q, sizeof q, "|%11d|%11d|%11d|%11d|%11d|%32s|\n",
            curr->id,
            curr->first_step,
            curr->repeat,
            curr->repeat_infinite,
            curr->crepeat,
            state
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+--------------------------------+\n", &crc);

    sendStr("+-----------------------------------------------------------------------+\n", &crc);
    sendStr("|                             Program PID                               |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  prog_id  |prog_pid_id|    mode   |     kp    |     ki    |     kd    |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_ST
    for (i = 0; i < curr->pid_list.length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%11c|%11f|%11f|%11f|\n",
                curr->id,
                curr->pid_list.item[i].id,
                curr->pid_list.item[i].pid.mode,
                curr->pid_list.item[i].pid.kp,
                curr->pid_list.item[i].pid.ki,
                curr->pid_list.item[i].pid.kd
                );
        sendStr(q, &crc);
    }
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-----------------------------------------------+\n", &crc);
    sendStr("|                   Program SEM                 |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  prog_id  |prog_sem_id|    em     |  sensor   |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_ST
    for (i = 0; i < curr->sem_list.length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%11d|%11d|\n",
                curr->id,
                curr->sem_list.item[i].id,
                curr->sem_list.item[i].em->id,
                curr->sem_list.item[i].sensor->id
                );
        sendStr(q, &crc);
    }
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+\n", &crc);

    sendStr("+-----------------------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                                  Program steps init data                                  |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  prog_id  |   kind    |     id    |  sensor   |    em     |   goal    |   dur sec |even_change| stop_kind |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_ST

            char *stop_kind = getStepStopKind(curr->step_curr.stop_kind);
    snprintf(q, sizeof q, "|%11d|%11s|%11d|%11p|%11p|%11f|%11ld|%11d|%11s|\n",
            curr->id,
            "CURR",
            curr->step_curr.id,
            curr->step_curr.sensor,
            curr->step_curr.em,
            curr->step_curr.goal,
            curr->step_curr.duration.tv_sec,
            curr->step_curr.even_change,
            stop_kind
            );
    sendStr(q, &crc);
    PROG_LIST_LOOP_SP
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("+-----------------------------------------------------------------------------------------------+\n", &crc);
    sendStr("|                                      Program steps runtime data                               |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    sendStr("|  prog_id  | step_kind |   state   | state_ch  | state_sp  |  cgoal    | goal_corr |wait_above |\n", &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_ST
            char *state = getStepState(curr->step_curr.state);
    char *state_ch = getStepStateCh(curr->step_curr.state_ch);
    char *state_sp = getStepStateSp(curr->step_curr.state_sp);
    snprintf(q, sizeof q, "|%11d|%11s|%11s|%11s|%11s|%11f|%11f|%11d|\n",
            curr->id,
            "CURR",
            state,
            state_ch,
            state_sp,
            curr->step_curr.cgoal,
            curr->step_curr.goal_correction,
            curr->step_curr.wait_above
            );
    sendStr(q, &crc);
    sendStr("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n", &crc);
    PROG_LIST_LOOP_SP

    sendFooter(crc);
}

void printProgList(ProgList *list) {
    puts("Prog list");
    puts("+-----------+-----------+-----------+-----------+");
    puts("|    id     | first_step|  repeat   |  rep_inf  |");
    puts("+-----------+-----------+-----------+-----------+");
    PROG_LIST_LOOP_ST
    printf("|%11d|%11d|%11d|%11d|\n",
            curr->id,
            curr->first_step,
            curr->repeat,
            curr->repeat_infinite
            );
    PROG_LIST_LOOP_SP
    puts("+-----------+-----------+-----------+-----------+");
}

void printProgPIDList(ProgPIDList *list) {
    size_t i;
    printf("|%11s|%11s|%11s|%11s|\n", "id", "kp", "ki", "kd");
    for (i = 0; i < list->length; i++) {
        printf("|%11d|%11f|%11f|%11f|\n",
                list->item[i].id,
                list->item[i].pid.kp,
                list->item[i].pid.ki,
                list->item[i].pid.kd
                );
    }
}

void printProgSEMList(ProgSEMList *list) {
    size_t i;
    puts("SEM list");
    puts("+-----------+-----------+-----------+-----------+-----------+");
    puts("|   id      | snr_id    | snr_rid   |  em_id    |  em_rid   |");
    puts("+-----------+-----------+-----------+-----------+-----------+");
    for (i = 0; i < list->length; i++) {
        printf("|%11d|%11d|%11d|%11d|%11d|\n",
                list->item[i].id,
                list->item[i].sensor->id,
                list->item[i].sensor->remote_id,
                list->item[i].em->id,
                list->item[i].em->remote_id
                );
    }
    puts("+-----------+-----------+-----------+-----------+-----------+");
}

void printHelp() {
    char q[LINE_SIZE];
    uint8_t crc = 0;
    sendStr("COMMAND LIST\n", &crc);
    snprintf(q, sizeof q, "%c\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tterminate process\n", ACP_CMD_APP_EXIT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    sendStr(q, &crc);

    snprintf(q, sizeof q, "%c\tload prog into RAM and start its execution; program id expected if '.' quantifier is used\n", ACP_CMD_START);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tunload program from RAM; program id expected if '.' quantifier is used\n", ACP_CMD_STOP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\texecute program from the beginning; no data reloading; program id expected if '.' quantifier is used\n", ACP_CMD_RESET);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tgo to specified step; stateId expected; program id expected if '.' quantifier is used\n", ACP_CMD_REGSTP_PROG_TO_STEP);
    sendStr(q, &crc);
    snprintf(q, sizeof q, "%c\tget program info; response format: progId_progState_progCurrRepeat_stepId_stepState_stepStateCh_stepStateSp_stepCurrGoal_stepTimePassed; program id expected if '.' quantifier is used\n", ACP_CMD_REGSTP_PROG_GET_DATA);
    sendStr(q, &crc);
    sendFooter(crc);
}
