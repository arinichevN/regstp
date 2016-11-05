
#include "main.h"

int acp_initBuf(char *buf, size_t buf_size) {
    if (buf_size < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
        fputs("acp_initBuf: not enough space buf\n", stderr);
#endif
        return 0;
    }
    memset(buf, 0, buf_size);
    buf[0] = '_';
    buf[1] = '_';
    buf[2] = '\n';
    return 1;
}

void acp_parsePackI1(const char *buf, I1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        int p0;
        if (sscanf(buff, "%d", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackI2(const char *buf, I2List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        int p0, p1;
        if (sscanf(buff, "%d_%d", &p0, &p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackI3(const char *buf, I3List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        int p0, p1, p2;
        if (sscanf(buff, "%d_%d_%d", &p0, &p1, &p2) != 3) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->item[list->length].p2 = p2;
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackF1(const char *buf, F1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        float p0;
        if (sscanf(buff, "%f", &p0) != 1) {
            break;
        }
        list->item[list->length] = p0;
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackI1F1(const char *buf, I1F1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        int p0;
        float p1;
        if (sscanf(buff, "%d_%f", &p0, &p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        list->item[list->length].p1 = p1;
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackS1(const char *buf, S1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    char format[LINE_SIZE];
    int n = sprintf(format, "%%%lus", NAME_SIZE);
    if (n <= 0) {
        return;
    }
    while (list->length < list_max_size) {
        char p0[NAME_SIZE];
        if (sscanf(buff, format, p0) != 1) {
            break;
        }
        strcpy(&list->item[list->length*NAME_SIZE], p0);
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackI1S1(const char *buf, I1S1List *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    char format[LINE_SIZE];
    int n = sprintf(format, "%%d_%%%lus", NAME_SIZE);
    if (n <= 0) {
        return;
    }
    while (list->length < list_max_size) {
        int p0; char p1[NAME_SIZE];
        if (sscanf(buff, format, &p0, p1) != 2) {
            break;
        }
        list->item[list->length].p0 = p0;
        strcpy(list->item[list->length].p1, p1);
        list->length++;
        strnline(&buff);
    }
}

void acp_parsePackFTS(const char *buf, FTSList *list, size_t list_max_size) {
    char *buff = buf;
    list->length = 0;
    if (strlen(buff) < ACP_RESP_BUF_SIZE_MIN) {
        return;
    }
    strnline(&buff);
    while (list->length < list_max_size) {
        int id, state;
        float temp;
        struct timespec tm;
        if (sscanf(buff, "%d_%f_%ld_%ld_%d", &id, &temp, &tm.tv_sec, &tm.tv_nsec, &state) != 5) {
            break;
        }
        list->item[list->length].id = id;
        list->item[list->length].value = temp;
        list->item[list->length].tm.tv_sec = tm.tv_sec;
        list->item[list->length].tm.tv_nsec = tm.tv_nsec;
        list->item[list->length].state = state;
        list->length++;
        strnline(&buff);
    }
}

size_t acp_packlen(char *buf, size_t buf_size) {
    int i, state = 0;
    size_t n = 0;
    for (i = 0; i < buf_size; i++) {
        switch (state) {
            case 0:
                if (buf[i] == '\n') {
                    state = 1;
                }
                n++;
                break;
            case 1:
                if (buf[i] == '\n') {
                    state = 2;
                } else {
                    state = 0;
                }
                n++;
                break;
            case 2:
                n++;
                return n;

        }

    }
    return 0;
}

//requires buf after acp_initBuf()

int acp_bufAddHeader(char *buf, char qnf, char *cmd_str, size_t buf_size) {
    buf[0] = qnf;
    buf[1] = cmd_str[0];
    buf[2] = '\n';
    return 1;
}

int acp_bufAddFooter(char *buf, size_t buf_size) {
    uint8_t crc = 0;
    //no more then 2 '\n''s in the end
    int state;
    size_t i, p;
    for (i = 0, p = -1, state = 0; i < buf_size && buf[i] != '\0' && state != 2; i++) {
        switch (state) {
            case 0:
                if (buf[i] == '\n') {
                    state = 1;
                }
                break;
            case 1:
                if (buf[i] == '\n' && p == -1) {
                    p = i;
                    state = 2;
                } else {
                    p = -1;
                    state = 0;
                }
                break;
        }
    }
    size_t pack_len;
    if (p == -1 && state == 0) {//we will add two '\n''s
        pack_len = strlen(buf) + 3;
        if (pack_len >= buf_size) {
#ifdef MODE_DEBUG
            fputs("acp_bufAddFooter: not enough space for three char in buf\n", stderr);
#endif
            return 0;
        }
        buf[pack_len - 3] = '\n';
        buf[pack_len - 2] = '\n';
    } else if (p == -1 && state == 1) {//we will add one '\n'
        pack_len = strlen(buf) + 2;
        if (pack_len >= buf_size) {
#ifdef MODE_DEBUG
            fputs("acp_bufAddFooter: not enough space for three char in buf\n", stderr);
#endif
            return 0;
        }
        buf[pack_len - 2] = '\n';
    } else if (p != -1 && state == 2) {//no '\n''s required
        puts("no ns req");
        if (p + 1 < strlen(buf)) {//we have too many '\n''s, lets cut them
            puts("too many ns");
            buf[p + 1] = '\0';
        }
        pack_len = p + 2;
        if (pack_len >= buf_size) {
#ifdef MODE_DEBUG
            fputs("acp_bufAddFooter: not enough space for three char in buf\n", stderr);
#endif
            return 0;
        }
    }
    crc_update_by_str(&crc, buf);
    buf[pack_len - 1] = crc;
    return 1;
}

int acp_sendBuf(char *buf, size_t buf_size, const Peer *peer) {
    if (sendto(*(peer->fd), buf, acp_packlen(buf, buf_size), 0, (struct sockaddr*) (&peer->addr), peer->addr_size) < 0) {
#ifdef MODE_DEBUG
        perror("acp_sendBuf: error writing to socket");
#endif
        return 0;
    }
    return 1;
}
//requires buf after acp_initBuf()

int acp_sendBufArrPackI1List(char cmd, size_t buf_size, const I1List *data, const Peer *peer) {
    char q[LINE_SIZE], buf[buf_size];
    int i;
    if (!acp_initBuf(buf, buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%d\n", data->item[i]);
        if (bufCat(buf, q, buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, buf_size, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendBufArrPackI2List(char cmd, size_t buf_size, const I2List *data, const Peer *peer) {
    char q[LINE_SIZE], buf[buf_size];
    int i;
    if (!acp_initBuf(buf, buf_size)) {
        return 0;
    }
    char cmd_str[] = {cmd, '\0'};
    if (!acp_bufAddHeader(buf, ACP_QUANTIFIER_SPECIFIC, cmd_str, buf_size)) {
        return 0;
    }
    for (i = 0; i < data->length; i++) {
        snprintf(q, sizeof q, "%d_%d\n", data->item[i].p0, data->item[i].p1);
        if (bufCat(buf, q, buf_size) == NULL) {
            return 0;
        }
    }
    if (!acp_bufAddFooter(buf, buf_size)) {
        return 0;
    }
    if (!acp_sendBuf(buf, buf_size, peer)) {
        return 0;
    }
    return 1;
}

void acp_sendStr(const char *s, uint8_t *crc, const Peer *peer) {
    if (sendto(*(peer->fd), s, strlen(s), 0, (struct sockaddr*) (&peer->addr), peer->addr_size) < 0) {
#ifdef MODE_DEBUG

        perror("sendStr: error writing to socket");
#endif
    }
    crc_update_by_str(crc, s);
}

void acp_sendFooter(int8_t crc, Peer *peer) {
    char s[] = "\n\n \n";
    s[2] = crc;
    if (sendto(*(peer->fd), s, strlen(s), 0, (struct sockaddr*) (&peer->addr), peer->addr_size) < 0) {
#ifdef MODE_DEBUG

        perror("sendFooter: error writing to socket");
#endif
    }
}

int acp_sendBufPack(char *buf, char qnf, const char *cmd_str, size_t buf_size, const Peer *peer) {
    if (!acp_bufAddHeader(buf, qnf, cmd_str, buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendBufPack: acp_bufAddHeader() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_bufAddFooter(buf, buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendBufPack: acp_bufAddFooter() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_sendBuf(buf, buf_size, peer)) {
        return 0;
    }
    return 1;
}

int acp_sendStrPack(char qnf, char *cmd, size_t buf_size, const Peer *peer) {
    char buf[buf_size];
    memset(buf, 0, sizeof buf);
    if (!acp_bufAddHeader(buf, qnf, cmd, buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendStrPack: acp_bufAddHeader() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_bufAddFooter(buf, buf_size)) {
#ifdef MODE_DEBUG
        fputs("acp_sendStrPack: acp_bufAddFooter() failed\n", stderr);
#endif
        return 0;
    }
    if (!acp_sendBuf(buf, buf_size, peer)) {
        return 0;
    }
    return 1;
}

int acp_recvOK(Peer *peer, size_t buf_size) {
    char buf[buf_size];
    if (recvfrom(*(peer->fd), buf, sizeof buf, 0, NULL, NULL) < 0) {
#ifdef MODE_DEBUG
        perror("acp_recvOK: recvfrom() error");
#endif
        return 0;
    }
    if (!crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvOK: crc_check() failed\n", stderr);
#endif
        return 0;
    }
    if (strncmp(buf + 1, ACP_RESP_REQUEST_SUCCEEDED, sizeof ACP_RESP_REQUEST_SUCCEEDED - 1) != 0) {
#ifdef MODE_DEBUG

        fprintf(stderr, "acp_recvOK: response is not OK but is: %c\n", buf[0]);
#endif
        return 0;
    }
    return 1;
}

int acp_recvFTS(FTSList *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd) {
    char buf[buf_size];
    if (recvfrom(fd, buf, sizeof buf, 0, NULL, NULL) < 0) {
#ifdef MODE_DEBUG
        perror("acp_recvFTS: recvfrom() error");
#endif
        return 0;
    }
    if (strlen(buf) < ACP_RESP_BUF_SIZE_MIN) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: not enough data\n", stderr);
#endif
        return 0;
    }
    if (buf[0] != qnf) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: bad quantifier\n", stderr);
#endif
        return 0;
    }
    if (strncmp(&buf[1], cmd, 1) != 0) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: bad command\n", stderr);
#endif
        return 0;
    }
    if (!crc_check(buf, sizeof buf)) {
#ifdef MODE_DEBUG
        fputs("acp_recvFTS: crc_check() failed\n", stderr);
#endif
        return 0;
    }
    acp_parsePackFTS(buf, list, list_max_size);
    return 1;
}

void freePeer(PeerList *list) {
    free(list->item);
    list->item = NULL;
    list->length = 0;
}

void acp_printI1(I1List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I1List dump");
    puts("+-----------+");
    puts("|   item    |");
    puts("+-----------+");
    for (i = 0; i < list->length; i++) {

        snprintf(q, sizeof q, "|%11d|", list->item[i]);
        puts(q);
    }
    puts("+-----------+");
}

void acp_printI2(I2List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I2List dump");
    puts("+-----------+-----------+");
    puts("|     p0    |     p1    |");
    puts("+-----------+-----------+");
    for (i = 0; i < list->length; i++) {

        snprintf(q, sizeof q, "|%11d|%11d|", list->item[i].p0, list->item[i].p1);
        puts(q);
    }
    puts("+-----------+-----------+");
}

void acp_printI3(I3List *list) {
    size_t i;
    char q[LINE_SIZE];
    puts("I3List dump");
    puts("+-----------+-----------+-----------+");
    puts("|     p0    |     p1    |     p2    |");
    puts("+-----------+-----------+-----------+");
    for (i = 0; i < list->length; i++) {
        snprintf(q, sizeof q, "|%11d|%11d|%11d|", list->item[i].p0, list->item[i].p1, list->item[i].p2);
        puts(q);
    }
    puts("+-----------+-----------+-----------+");
}