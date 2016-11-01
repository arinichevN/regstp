

#include "app.h"

void setPriorityMax(int policy) {
    int max = sched_get_priority_max(policy);
    if (max == -1) {
        perror("sched_get_priority_max() failed");
        return;
    }
    struct sched_param sp;
    memset(&sp, 0, sizeof sp);
    sp.__sched_priority = max;
    int ret = sched_setscheduler(0, policy, &sp);
    if(ret==-1){
        perror("sched_setscheduler() failed");
    }
}

void conSig(void (*fn)()){
    signal(SIGINT, fn);
    signal(SIGTSTP, fn);
    signal(SIGTERM, fn);
    signal(SIGPIPE, SIG_IGN);
}

int readConf(const char *path, char conninfo[LINE_SIZE], char app_class[NAME_SIZE]) {
    FILE *fp;
    memset(conninfo, 0, LINE_SIZE);
    memset(app_class, 0, NAME_SIZE);
    fp = fopen(path, "r");
    if (fp == NULL) {
        fputs("readIni: failed to read file\n",stderr);
        return 0;
    }
    fgets(conninfo, LINE_SIZE, fp);
    fgets(app_class, NAME_SIZE, fp);
    fclose(fp);
    int i;
    for (i = 0; i < LINE_SIZE; i++) {
        if (conninfo[i] == '\n') {
            conninfo[i] = '\0';
            break;
        }
    }
    for (i = 0; i < NAME_SIZE; i++) {
        if (app_class[i] == '\n') {
            app_class[i] = '\0';
            break;
        }
    }
    return 1;
}
int readHostName(char *hostname) {
    memset(hostname, 0, sizeof hostname);
    if (gethostname(hostname, HOST_NAME_MAX)) {
        perror("readHostName: failed to read\n");
        return 0;
    }
    return 1;
}
/*
*call it if you want to prevent many instances of particular process from running at the same time
*/
int initPid(int *pid_file, int *pid, const char *pid_path) {
    *pid_file = creat(pid_path, O_RDWR);
    int rc;
    char pid_str[INT_STR_SIZE];
    int n_written;
    if (*pid_file == -1) {
        fputs("setPid: couldn't create pid file\n", stderr);
        return 0;
    } else {
        rc = flock(*pid_file, LOCK_EX | LOCK_NB);
        if (rc) {//lock failed
            if (errno == EWOULDBLOCK) {
                fputs("setPid: another instance of this process is running\n",stderr);
                return 0;
            } else {
                fputs("setPid: lock failed\n",stderr);
                return 0;
            }
        } else {//lock succeeded
            *pid = getpid();
            sprintf(pid_str, "%d\n", *pid);
            n_written = write(*pid_file, pid_str, sizeof pid_str);
            if (n_written != sizeof pid_str) {
                fputs("setPid: writing to pid file failed\n", stderr);
                return 0;
            }
        }
    }
    return 1;
}

void freePid(int *pid_file, int *pid, const char *pid_path) {
    if (*pid_file != -1) {
        close(*pid_file);
        *pid_file = -1;
    }
    if (*pid >= 0) {
        unlink(pid_path);
        *pid = -1;
    }
}

int initMutex(Mutex *m) {
    m->created = 0;
    m->attr_initialized = 0;
    if (pthread_mutexattr_init(&m->attr) != 0) {
        perror("initMutex: pthread_mutexattr_init");
        return 0;
    }
    m->attr_initialized = 1;
    if (pthread_mutexattr_settype(&m->attr, PTHREAD_MUTEX_ERRORCHECK) != 0) {
        perror("initMutex: pthread_mutexattr_settype");
        return 0;
    }

    if (pthread_mutex_init(&m->self, &m->attr) != 0) {
        perror("initMutex: pthread_mutex_init r");
        return 0;
    }
    m->created = 1;
    return 1;
}

void freeMutex(Mutex *m) {
    if (m->attr_initialized) {
        if (pthread_mutexattr_destroy(&m->attr) != 0) {
            perror("freeMutex: pthread_mutexattr_destroy");
        }else{
            m->attr_initialized = 0;
        }
    }
    if (m->created) {
        if (pthread_mutex_destroy(&m->self) != 0) {
            perror("freeMutex: pthread_mutex_destroy");
        }else{
            m->created = 0;
        }
    }
}

void waitThreadCmd(char *thread_cmd,char *thread_qfr, char *cmd) {
    struct timespec time_r, time_w = {0, 10000};
    *thread_qfr=cmd[0];
    *thread_cmd = cmd[1];
    while (*thread_cmd != ACP_CMD_APP_NO) {
        nanosleep(&time_w, &time_r);
    }
}