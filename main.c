/*
 * regstp
 */


#include "main.h"

char pid_path[LINE_SIZE];
char app_class[NAME_SIZE];
char db_conninfo_settings[LINE_SIZE];

int app_state = APP_INIT;
PGconn *db_conn_settings = NULL;
PGconn *db_conn_public = NULL;
PGconn **db_connp_public = NULL;

PGconn *db_conn_data = NULL;
PGconn **db_connp_data = NULL; //pointer to db_conn_settings or to db_conn_data

int pid_file = -1;
int proc_id;
int udp_port = -1;
size_t udp_buf_size = 0;
int udp_fd = -1;
int udp_fd_tf = -1;
Peer peer_client = {.fd = &udp_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
int data_initialized = 0;
ThreadData thread_data_ctl = {.cmd = ACP_CMD_APP_NO, .on = 0, .created = 0, .attr_initialized = 0};
I1List i1l = {NULL, 0};
I2List i2l = {NULL, 0};
Mutex progl_mutex = {.created = 0, .attr_initialized = 0};
PeerList peer_list = {NULL, 0};
EMList em_list = {NULL, 0};
SensorList sensor_list = {NULL, 0};
ProgList prog_list = {NULL, NULL, 0};

#include "util.c"

int readSettings() {
    PGresult *r;
    char q[LINE_SIZE];
    char db_conninfo_data[LINE_SIZE];
    char db_conninfo_public[LINE_SIZE];
    memset(pid_path, 0, sizeof pid_path);
    memset(db_conninfo_data, 0, sizeof db_conninfo_data);
    if (!initDB(&db_conn_settings, db_conninfo_settings)) {
        return 0;
    }
    snprintf(q, sizeof q, "select db_public from %s.config where app_class='%s'", APP_NAME_STR, app_class);
    if ((r = dbGetDataT(db_conn_settings, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) != 1) {
        PQclear(r);
        fputs("ERROR: readSettings: need only one tuple (1)\n", stderr);
        return 0;
    }


    memcpy(db_conninfo_public, PQgetvalue(r, 0, 0), LINE_SIZE);
    PQclear(r);
    if (dbConninfoEq(db_conninfo_public, db_conninfo_settings)) {
        db_connp_public = &db_conn_settings;
    } else {
        if (!initDB(&db_conn_public, db_conninfo_public)) {
            return 0;
        }
        db_connp_public = &db_conn_public;
    }

    udp_buf_size = 0;
    udp_port = -1;
    snprintf(q, sizeof q, "select udp_port, udp_buf_size, pid_path, db_data, cycle_duration_us from %s.config where app_class='%s'", APP_NAME_STR, app_class);
    if ((r = dbGetDataT(db_conn_settings, q, q)) == NULL) {
        return 0;
    }
    if (PQntuples(r) == 1) {
        int done = 1;
        done = done && config_getUDPPort(*db_connp_public, PQgetvalue(r, 0, 0), &udp_port);
        done = done && config_getBufSize(*db_connp_public, PQgetvalue(r, 0, 1), &udp_buf_size);
        done = done && config_getPidPath(*db_connp_public, PQgetvalue(r, 0, 2), pid_path, LINE_SIZE);
        done = done && config_getDbConninfo(*db_connp_public, PQgetvalue(r, 0, 3), db_conninfo_data, LINE_SIZE);
        done = done && config_getCycleDurationUs(*db_connp_public, PQgetvalue(r, 0, 4), &cycle_duration);
        if (!done) {
            PQclear(r);
            freeDB(&db_conn_public);
            fputs("ERROR: readSettings: failed to read some fields\n", stderr);
            return 0;
        }
    } else {
        PQclear(r);
        freeDB(&db_conn_public);
        fputs("ERROR: readSettings: one tuple expected\n", stderr);
        return 0;
    }
    PQclear(r);


    if (dbConninfoEq(db_conninfo_data, db_conninfo_settings)) {
        db_connp_data = &db_conn_settings;
    } else if (dbConninfoEq(db_conninfo_data, db_conninfo_public)) {
        db_connp_data = &db_conn_public;
    } else {
        if (!initDB(&db_conn_data, db_conninfo_data)) {
            return 0;
        }
        db_connp_data = &db_conn_data;
    }

    if (!(dbConninfoEq(db_conninfo_data, db_conninfo_settings) || dbConninfoEq(db_conninfo_public, db_conninfo_settings))) {
        freeDB(&db_conn_settings);
    }
    return 1;
}

void initApp() {
    if (!readConf(CONF_FILE, db_conninfo_settings, app_class)) {
        exit_nicely_e("initApp: failed to read configuration file\n");
    }

    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }

    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }

    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize mutex\n");
    }

    if (!initUDPServer(&udp_fd, udp_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }

    if (!initUDPClient(&udp_fd_tf, WAIT_RESP_TIMEOUT)) {
        exit_nicely_e("initApp: failed to initialize udp client\n");
    }
}

void initData() {
    data_initialized = 0;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select peer_id from %s.em_mapping where app_class='%s' union distinct select peer_id from %s.sensor_mapping where app_class='%s'", APP_NAME_STR, app_class, APP_NAME_STR, app_class);
    if (!config_getPeerList(*db_connp_data, *db_connp_public, q, &peer_list, &udp_fd_tf)) {
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    if (!initSensor(&sensor_list, &peer_list)) {
        FREE_LIST(&sensor_list);
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    if (!initEM(&em_list, &peer_list)) {
        FREE_LIST(&em_list);
        FREE_LIST(&sensor_list);
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    i1l.item = (int *) malloc(udp_buf_size * sizeof *(i1l.item));
    if (i1l.item == NULL) {
        FREE_LIST(&em_list);
        FREE_LIST(&sensor_list);
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    i2l.item = (I2 *) malloc(udp_buf_size * sizeof *(i2l.item));
    if (i2l.item == NULL) {
        FREE_LIST(&i1l);
        FREE_LIST(&em_list);
        FREE_LIST(&sensor_list);
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    if (!createThread_ctl(&thread_data_ctl)) {
        FREE_LIST(&i2l);
        FREE_LIST(&i1l);
        FREE_LIST(&em_list);
        FREE_LIST(&sensor_list);
        FREE_LIST(&peer_list);
        freeDB(&db_conn_data);
        freeDB(&db_conn_public);
        return;
    }
    data_initialized = 1;
}

int initSensor(SensorList *list, const PeerList *pl) {
    PGresult *r;
    char q[LINE_SIZE];
    size_t i;
    list->length = 0;
    list->item = NULL;
    snprintf(q, sizeof q, "select sensor_id, remote_id, peer_id from %s.sensor_mapping where app_class='%s'", APP_NAME_STR, app_class);
    if ((r = dbGetDataT(*db_connp_data, q, "initSensor: select pin: ")) == NULL) {
        return 0;
    }
    list->length = PQntuples(r);
    if (list->length > 0) {
        list->item = (Sensor *) malloc(list->length * sizeof *(list->item));
        if (list->item == NULL) {
            list->length = 0;
            fputs("ERROR: initSensor: failed to allocate memory\n", stderr);
            PQclear(r);
            return 0;
        }
        for (i = 0; i < list->length; i++) {
            list->item[i].id = atoi(PQgetvalue(r, i, 0));
            list->item[i].remote_id = atoi(PQgetvalue(r, i, 1));
            list->item[i].source = getPeerById(PQgetvalue(r, i, 2), pl);
        }
    }
    PQclear(r);
    if (!checkSensor(list)) {
        return 0;
    }
    return 1;
}

int initEM(EMList *list, const PeerList *pl) {
    PGresult *r;
    char q[LINE_SIZE];
    size_t i;
    list->length = 0;
    list->item = NULL;
    snprintf(q, sizeof q, "select em_id, remote_id, peer_id from %s.em_mapping where app_class='%s'", APP_NAME_STR, app_class);
    if ((r = dbGetDataT(*db_connp_data, q, "initEm: select pin: ")) == NULL) {
        return 0;
    }
    list->length = PQntuples(r);
    if (list->length > 0) {
        list->item = (EM *) malloc(list->length * sizeof *(list->item));
        if (list->item == NULL) {
            list->length = 0;
            fputs("ERROR: initEm: failed to allocate memory\n", stderr);
            PQclear(r);
            return 0;
        }
        for (i = 0; i < list->length; i++) {
            list->item[i].id = atoi(PQgetvalue(r, i, 0));
            list->item[i].remote_id = atoi(PQgetvalue(r, i, 1));
            list->item[i].source = getPeerById(PQgetvalue(r, i, 2), &peer_list);
            list->item[i].last_output = 0.0f;
        }
    }
    PQclear(r);
    if (!checkEM(list)) {
        return 0;
    }
    return 1;
}

int addProg(Prog *item, ProgList *list) {
    if (list->length >= INT_MAX) {
        return 0;
    }
    if (list->top == NULL) {
        list->top = item;
    } else {
        list->last->next = item;
    }
    list->last = item;
    list->length++;
#ifdef MODE_DEBUG
    printf("prog with id=%d loaded\n", item->id);
#endif
    return 1;
}

int addProgById(int id, ProgList *list) {
    Prog *p = getProgByIdFdb(id, &sensor_list, &em_list);
    if (p == NULL) {
        return 0;
    }
    if (!addProg(p, list)) {
        free(p);
        return 0;
    }
    return 1;
}

int deleteProgById(int id, ProgList *list) {
    Prog *prev = NULL, *curr;
    int done = 0;
    curr = list->top;
    while (curr != NULL) {
        if (curr->id == id) {
            if (prev != NULL) {
                prev->next = curr->next;
            } else {//curr=top
                list->top = curr->next;
            }
            if (curr == list->last) {
                list->last = prev;
            }
            free(curr);
            list->length--;
#ifdef MODE_DEBUG
            printf("prog with id: %d deleted from prog_list\n", id);
#endif
            done = 1;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    return done;
}

int switchProgById(int id, ProgList *list) {
    if (deleteProgById(id, list)) {
        return 1;
    }
    return addProgById(id, list);
}

void loadAllProg(ProgList *list, const SensorList *slist, const EMList *elist) {
    PGresult *r;
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select id, first_step, repeat, repeat_infinite from %s.prog order by id", APP_NAME_STR);
    if ((r = dbGetDataT(*db_connp_data, q, "loadAllProg: select pin: ")) == NULL) {
        return;
    }
    int n = PQntuples(r);
    int i;
    for (i = 0; i < n; i++) {
        Prog *item = (Prog *) malloc(sizeof *(item));
        if (item == NULL) {
#ifdef MODE_DEBUG
            fputs("ERROR: loadAllProg: failed to allocate memory\n", stderr);
#endif
            break;
        }
        item->id = atoi(PQgetvalue(r, i, 0));
        item->first_step = atoi(PQgetvalue(r, i, 1));
        item->repeat = atoi(PQgetvalue(r, i, 2));
        item->repeat_infinite = atoi(PQgetvalue(r, i, 3));
        INIT_PROG_FIELDS
        if (!initMutex(&item->mutex)) {
            FREE_LIST(&item->pid_list);
            FREE_LIST(&item->sem_list);
            free(item);
            continue;
        }
        if (!initMutex(&item->mutex_all)) {
            FREE_LIST(&item->pid_list);
            FREE_LIST(&item->sem_list);
            free(item);
            continue;
        }
        if (!checkProg(item, list)) {
            FREE_LIST(&item->pid_list);
            FREE_LIST(&item->sem_list);
            free(item);
            continue;
        }
        if (!addProg(item, list)) {
            FREE_LIST(&item->pid_list);
            FREE_LIST(&item->sem_list);
            free(item);
            continue;
        }
    }
    PQclear(r);
    printProgList(&prog_list);
}

int sensorRead(Sensor *s) {
    if (s == NULL) {
        return 0;
    }
    int di[1];
    di[0] = s->remote_id;
    I1List data = {di, 1};

    if (!acp_sendBufArrPackI1List(ACP_CMD_GET_FTS, udp_buf_size, &data, s->source)) {
#ifdef MODE_DEBUG
        fputs("ERROR: sensorRead: acp_sendBufArrPackI1List failed\n", stderr);
#endif
        s->value.state = 0;
        return 0;
    }
    //waiting for response...
    FTS td[1];
    FTSList tl = {td, 0};

    if (!acp_recvFTS(&tl, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED, udp_buf_size, 1, *(s->source->fd))) {
#ifdef MODE_DEBUG
        fputs("ERROR: sensorRead: acp_recvFTS() error\n", stderr);
#endif
        s->value.state = 0;
        return 0;
    }
    if (tl.item[0].state == 1 && tl.item[0].id == s->id) {
        s->value = tl.item[0];
        return 1;
    } else {
#ifdef MODE_DEBUG
        fputs("ERROR: sensorRead: response: temperature sensor state is bad or id is not requested one\n", stderr);
#endif
        s->value.state = 0;

        return 0;
    }
    return 1;
}

int controlEM(EM *em, float output) {
    if (em == NULL) {
        return 0;
    }
    if (output == em->last_output) {
        return 0;
    }
    I2 di[1];
    di[0].p0 = em->remote_id;
    di[0].p1 = (int) output;
    I2List data = {di, 1};
    if (!acp_sendBufArrPackI2List(ACP_CMD_SET_DUTY_CYCLE_PWM, udp_buf_size, &data, em->source)) {
#ifdef MODE_DEBUG
        fputs("ERROR: controlEM: failed to send request\n", stderr);
#endif
        return 0;
    }
    em->last_output = output;
    return 1;
}

void serverRun(int *state, int init_state) {
    char buf_in[udp_buf_size];
    char buf_out[udp_buf_size];
    uint8_t crc;
    int i, j;
    char q[LINE_SIZE];
    crc = 0;
    memset(buf_in, 0, sizeof buf_in);
    acp_initBuf(buf_out, sizeof buf_out);
    if (recvfrom(udp_fd, buf_in, sizeof buf_in, 0, (struct sockaddr*) (&(peer_client.addr)), &(peer_client.addr_size)) < 0) {
#ifdef MODE_DEBUG
        perror("serverRun: recvfrom() error");
#endif
    }
#ifdef MODE_DEBUG
    dumpBuf(buf_in, sizeof buf_in);
#endif    
    if (!crc_check(buf_in, sizeof buf_in)) {
#ifdef MODE_DEBUG
        fputs("ERROR: serverRun: crc check failed\n", stderr);
#endif
        return;
    }
    switch (buf_in[1]) {
        case ACP_CMD_APP_START:
            if (!init_state) {
                *state = APP_INIT_DATA;
            }
            return;
        case ACP_CMD_APP_STOP:
            if (init_state) {
                waitThread_ctl(buf_in);
                *state = APP_STOP;
            }
            return;
        case ACP_CMD_APP_RESET:
            waitThread_ctl(buf_in);
            *state = APP_RESET;
            return;
        case ACP_CMD_APP_EXIT:
            waitThread_ctl(buf_in);
            *state = APP_EXIT;
            return;
        case ACP_CMD_APP_PING:
            if (init_state) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_BUSY);
            } else {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_APP_IDLE);
            }
            return;
        case ACP_CMD_APP_PRINT:
            printAll(&prog_list, &peer_list, &em_list, &sensor_list);
            return;
        case ACP_CMD_APP_HELP:
            printHelp();
            return;
        default:
            if (!init_state) {
                return;
            }
            break;
    }

    switch (buf_in[0]) {
        case ACP_QUANTIFIER_BROADCAST:
        case ACP_QUANTIFIER_SPECIFIC:
            break;
        default:
            return;
    }

    switch (buf_in[1]) {
        case ACP_CMD_STOP:
        case ACP_CMD_START:
        case ACP_CMD_RESET:
        case ACP_CMD_REGSTP_PROG_GET_DATA:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI1(buf_in, &i1l, udp_buf_size);
                    if (i1l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        case ACP_CMD_REGSTP_PROG_TO_STEP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    acp_parsePackI1(buf_in, &i1l, udp_buf_size);
                    if (i1l.length <= 0) {
                        return;
                    }
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    acp_parsePackI2(buf_in, &i2l, udp_buf_size);
                    if (i2l.length <= 0) {
                        return;
                    }
                    break;
            }
            break;
        default:
            return;

    }

    switch (buf_in[1]) {
        case ACP_CMD_STOP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    if (lockProgAll()) {
                        if (lockProgList()) {
                            PROG_LIST_LOOP_ST
                            controlEM(curr->step_curr.em, 0.0f);
                            PROG_LIST_LOOP_SP
                            freeProg(&prog_list);
                            unlockProgList();
                        }
                        unlockProgAll();
                    }
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    if (lockProgList()) {
                        for (i = 0; i < i1l.length; i++) {
                            Prog *p = getProgById(i1l.item[i], &prog_list);
                            if (p != NULL) {
                                controlEM(p->step_curr.em, 0.0f);
                            }
                            deleteProgById(i1l.item[i], &prog_list);
                        }
                        unlockProgList();
                    }
                    break;
            }
            return;
        case ACP_CMD_START:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    if (lockProgList()) {
                        loadAllProg(&prog_list, &sensor_list, &em_list);
                        unlockProgList();
                    }
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    if (lockProgList()) {
                        for (i = 0; i < i1l.length; i++) {
                            addProgById(i1l.item[i], &prog_list);
                        }
                        unlockProgList();
                    }
                    break;
            }
            return;
        case ACP_CMD_RESET:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        curr->state = INIT;
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                curr->state = INIT;
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGSTP_PROG_TO_STEP:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        progToStep(i1l.item[0], curr);
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i2l.length; i++) {
                        Prog *curr = getProgById(i2l.item[i].p0, &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                progToStep(i2l.item[i].p1, curr);
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            return;
        case ACP_CMD_REGSTP_PROG_GET_DATA:
            switch (buf_in[0]) {
                case ACP_QUANTIFIER_BROADCAST:
                {
                    PROG_LIST_LOOP_ST
                    if (lockProg(curr)) {
                        if (!bufCatProg(curr, buf_out, udp_buf_size)) {
                            sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                            return;
                        }
                        unlockProg(curr);
                    }
                    PROG_LIST_LOOP_SP
                    break;
                }
                case ACP_QUANTIFIER_SPECIFIC:
                    for (i = 0; i < i1l.length; i++) {
                        Prog *curr = getProgById(i1l.item[i], &prog_list);
                        if (curr != NULL) {
                            if (lockProg(curr)) {
                                if (!bufCatProg(curr, buf_out, udp_buf_size)) {
                                    sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                                    return;
                                }
                                unlockProg(curr);
                            }
                        }
                    }
                    break;
            }
            if (!sendBufPack(buf_out, ACP_QUANTIFIER_SPECIFIC, ACP_RESP_REQUEST_SUCCEEDED)) {
                sendStrPack(ACP_QUANTIFIER_BROADCAST, ACP_RESP_BUF_OVERFLOW);
                return;
            }
            return;
    }

}

void secure() {
    size_t i;
    for (i = 0; i < em_list.length; i++) {
        controlEM(&em_list.item[i], 0.0f);
    }
}

void progControl(Prog *item) {
    switch (item->state) {
        case INIT:
            item->crepeat = 0;
            item->state = CREP;
            break;
        case CREP:
            if ((item->crepeat < INT_MAX && item->crepeat < item->repeat) || item->repeat_infinite) {
                item->state = FSTEP;
            } else {
                item->state = OFF;
            }
            break;
        case FSTEP:
            if (getStepByIdFdb(item->first_step, item, &item->step_curr)) {
                item->state = RUN;
            } else {
                item->state = OFF;
            }
            break;
        case RUN:
            printf("prog:%d ", item->id);
            if (stepControl(&item->step_curr)) {
                item->state = NSTEP;
            }
            break;
        case NSTEP:
            if (getStepByIdFdb(item->step_curr.next_id, item, &item->step_curr)) {
                item->state = RUN;
            } else {
                if (!item->repeat_infinite) {
                    item->crepeat++;
                }
                item->state = CREP;
            }
            break;
        default:
            item->state = OFF;
            break;
    }
}

int stepControl(Step *item) {

    char *state = getStepState(item->state);
    char *state_ch = getStepStateCh(item->state_ch);
    char *state_sp = getStepStateSp(item->state_sp);
    printf("step:%d state:%s state_ch:%s state_sp:%s  ", item->id, state, state_ch, state_sp);

    switch (item->state) {
        case INIT:
            item->pid->reset = 1;
            item->tmr.ready = 0;
            item->cgoal = item->goal;
            item->state_ch = INIT;
            item->state_sp = INIT;
            item->state = RUN;
            break;
        case RUN:
            if (sensorRead(item->sensor)) {
                item->output = pid(item->pid, item->cgoal, item->sensor->value.temp);
                //  item->output = pidwt(item->pid, item->cgoal, item->sensor->value.temp, item->sensor->value.tm);
#ifdef MODE_DEBUG

                printf("goal=%.1f real=%.1f out=%.1f\n", item->cgoal, item->sensor->value.temp, item->output);

                //     printf("pid_mode=%c pid_ie=%.1f\n", item->pid->mode, item->pid->integral_error);
#endif
                controlEM(item->em, item->output);
            }

            //  {item->sensor->value.temp = 20; item->sensor->value.state = 1;}

            //end of step control
            switch (item->state_sp) {
                case INIT:
                    if (item->stop_kind == STOP_KIND_TIME) {
                        item->state_sp = BYTIME;
                    } else if (item->stop_kind == STOP_KIND_GOAL) {
                        item->wait_above = (item->goal > item->sensor->value.temp);
                        item->state_sp = BYGOAL;
                    }
                    break;
                case BYTIME:
                    if (ton_ts(item->duration, &item->tmr)) {
                        item->state = OFF;
                        //  puts("step done by time");
                        controlEM(item->em, 0.0f);
                        return 1;
                    }
                    break;
                case BYGOAL:
                    if (item->wait_above) {
                        if (item->goal <= item->sensor->value.temp) {
                            item->state = OFF;
                            //  puts("step done by goal <");
                            controlEM(item->em, 0.0f);
                            return 1;
                        }
                    } else {
                        if (item->goal >= item->sensor->value.temp) {
                            item->state = OFF;
                            //  puts("step done by goal >");
                            controlEM(item->em, 0.0f);
                            return 1;
                        }
                    }
                    break;
                default:
                    item->state_sp = OFF;
                    break;
            }

            //goal correction
            switch (item->state_ch) {
                case INIT:
                    if (item->even_change && item->stop_kind == STOP_KIND_TIME) {
                        if (item->sensor->value.state) {
                            item->goal_correction = 0;
                            item->temp_start = item->sensor->value.temp;
                            if (item->duration.tv_sec > 0) {
                                item->goal_correction = (item->goal - item->sensor->value.temp) / item->duration.tv_sec;
                            }
                            if (item->duration.tv_nsec > 0) {
                                item->goal_correction += (item->goal - item->sensor->value.temp) / item->duration.tv_nsec * NANO_FACTOR;
                            }
                            item->state_ch = RUN;
                            //   printf("goal_correction=%f\n", item->goal_correction);
                        }
                    } else {
                        item->state_ch = OFF;
                    }
                    break;
                case RUN:
                {
                    struct timespec dif = getTimePassed_ts(item->tmr.start);
                    item->cgoal = item->temp_start + dif.tv_sec * item->goal_correction + dif.tv_nsec * item->goal_correction * NANO_FACTOR;
                    break;
                }
                default:
                    item->state_ch = OFF;
                    break;
            }
            break;

        default:
            item->state = INIT;
            break;
    }


    return 0;
}

void progToStep(int step_id, Prog *item) {
    Step step;
    if (getStepByIdFdb(step_id, item, &step)) {
        item->step_curr = step;
        item->state = RUN;
    }
}

void *threadFunction_ctl(void *arg) {
    ThreadData *data = (ThreadData *) arg;
    data->on = 1;
    int r;
    if (pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &r) != 0) {
        perror("threadFunction_ctl: pthread_setcancelstate");
    }
    if (pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &r) != 0) {
        perror("threadFunction_ctl: pthread_setcanceltype");
    }

    while (1) {
        size_t i;
        struct timespec t1;
        clock_gettime(LIB_CLOCK, &t1);
        if (tryLockProgList()) {
            PROG_LIST_LOOP_ST
            if (tryLockProgA(curr)) {
                if (tryLockProg(curr)) {
                    progControl(curr);
                    unlockProg(curr);
                }
                unlockProgA(curr);
            } else {
                break;
            }

            PROG_LIST_LOOP_SP
            unlockProgList();
        }
        switch (data->cmd) {
            case ACP_CMD_APP_STOP:
            case ACP_CMD_APP_RESET:
            case ACP_CMD_APP_EXIT:
                secure();
                data->cmd = ACP_CMD_APP_NO;
                data->on = 0;
                return (EXIT_SUCCESS);
        }
        data->cmd = ACP_CMD_APP_NO; //notify main thread that command has been executed
        sleepRest(data->cycle_duration, t1);
    }
}

int createThread_ctl(ThreadData * td) {
    //set attributes for each thread
    if (pthread_attr_init(&td->thread_attr) != 0) {
        perror("createThreads: pthread_attr_init");
        return 0;
    }
    td->attr_initialized = 1;
    td->cycle_duration = cycle_duration;
    if (pthread_attr_setdetachstate(&td->thread_attr, PTHREAD_CREATE_DETACHED) != 0) {
        perror("createThreads: pthread_attr_setdetachstate");
        return 0;
    }

    //create a thread
    if (pthread_create(&td->thread, &td->thread_attr, threadFunction_ctl, (void *) td) != 0) {
        perror("createThreads: pthread_create");
        return 0;
    }
    td->created = 1;

    return 1;
}

void freeProg(ProgList *list) {
    Prog *curr = list->top, *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        FREE_LIST(&temp->pid_list);
        FREE_LIST(&temp->sem_list);
        free(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

void freeThread_ctl() {
    if (thread_data_ctl.created) {
        if (thread_data_ctl.on) {
            char cmd[2] = {ACP_QUANTIFIER_BROADCAST, ACP_CMD_APP_EXIT};
            waitThreadCmd(&thread_data_ctl.cmd, &thread_data_ctl.qfr, cmd);
        }
        if (pthread_attr_destroy(&thread_data_ctl.thread_attr) != 0) {
            perror("freeThread: pthread_attr_destroy");
        }
    } else {
        if (thread_data_ctl.attr_initialized) {
            if (pthread_attr_destroy(&thread_data_ctl.thread_attr) != 0) {

                perror("freeThread: pthread_attr_destroy");
            }
        }
    }
    thread_data_ctl.on = 0;
    thread_data_ctl.cmd = ACP_CMD_APP_NO;
}

void freeData() {
    freeThread_ctl();
    freeProg(&prog_list);
    FREE_LIST(&i2l);
    FREE_LIST(&i1l);
    FREE_LIST(&em_list);
    FREE_LIST(&sensor_list);
    FREE_LIST(&peer_list);
    freeDB(&db_conn_data);
    freeDB(&db_conn_public);
    data_initialized = 0;
}

void freeApp() {
    freeData();
    freeSocketFd(&udp_fd);
    freeSocketFd(&udp_fd_tf);
    freeMutex(&progl_mutex);
    freePid(&pid_file, &proc_id, pid_path);
    freeDB(&db_conn_settings);
}

void exit_nicely() {

    freeApp();
    puts("\nBye...");
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {

    fprintf(stderr, "%s", s);
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    while (1) {
        switch (app_state) {
            case APP_INIT:
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
                initData();
                app_state = APP_RUN;
                break;
            case APP_RUN:puts("main: run");
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
                freeData();
                app_state = APP_RUN;
                break;
            case APP_RESET:
                freeApp();
                app_state = APP_INIT;
                break;
            case APP_EXIT:
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}