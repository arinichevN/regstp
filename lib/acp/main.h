#ifndef LIBPAS_ACP_MAIN_H
#define LIBPAS_ACP_MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>

#include "app.h"
#include "cmd.h"

#include "../app.h"
#include "../util.h"
#include "../crc.h"

typedef struct {
    char id[NAME_SIZE];
    int *fd;
    struct sockaddr_in addr;
    socklen_t addr_size;
} Peer;

typedef struct {
    Peer *item;
    size_t length;
} PeerList;

typedef struct {
    int *item;
    size_t length;
} I1List;

typedef struct {
    int p0;
    int p1;
} I2;

typedef struct {
    I2 *item;
    size_t length;
} I2List;

typedef struct {
    int p0;
    int p1;
    int p2;
} I3;

typedef struct {
    I3 *item;
    size_t length;
} I3List;

typedef struct {
    float *item;
    size_t length;
} F1List;

typedef struct {
    int p0;
    float p1;
} I1F1;

typedef struct {
    I1F1 *item;
    size_t length;
} I1F1List;

typedef struct {
    char *item;
    size_t length;
} S1List;

typedef struct {
    int p0;
    char p1[NAME_SIZE];
} I1S1;

typedef struct {
    I1S1 *item;
    size_t length;
} I1S1List;

typedef struct {
    int id;
    float value;
    struct timespec tm;
    int state;
} FTS;

typedef struct {
    FTS *item;
    size_t length;
} FTSList;

#define ACP_HEADER_LENGTH 3

#define ACP_RESP_BUF_SIZE_MIN 6UL

#define ACP_QUANTIFIER_BROADCAST '!'
#define ACP_QUANTIFIER_SPECIFIC '.'

#define ACP_RESP_REQUEST_FAILED ("F")
#define ACP_RESP_REQUEST_SUCCEEDED ("T")
#define ACP_RESP_REQUEST_SUCCEEDED_PARTIAL ("P")
#define ACP_RESP_RESULT_UNKNOWN ("R")
#define ACP_RESP_COMMAND_UNKNOWN ("U")
#define ACP_RESP_QUANTIFIER_UNKNOWN ("Q")
#define ACP_RESP_CRC_ERROR ("C")
#define ACP_RESP_BUF_OVERFLOW ("O")



extern int acp_initBuf(char *buf, size_t buf_size) ;

extern void acp_parsePackI1(const char *buf, I1List *list, size_t list_max_size) ;

extern void acp_parsePackI2(const char *buf, I2List *list, size_t list_max_size) ;

extern void acp_parsePackI3(const char *buf, I3List *list, size_t list_max_size) ;

extern void acp_parsePackF1(const char *buf, F1List *list, size_t list_max_size) ;

extern void acp_parsePackI1F1(const char *buf, I1F1List *list, size_t list_max_size) ;

extern void acp_parsePackS1(const char *buf, S1List *list, size_t list_max_size) ;

extern void acp_parsePackI1S1(const char *buf, I1S1List *list, size_t list_max_size) ;

extern void acp_parsePackFTS(const char *buf, FTSList *list, size_t list_max_size) ;

extern size_t acp_packlen(char *buf, size_t buf_size) ;

extern int acp_bufAddHeader(char *buf, char qnf, char *cmd_str, size_t buf_size) ;

extern int acp_bufAddFooter(char *buf, size_t buf_size) ;

extern int acp_sendBuf(char *buf, size_t buf_size, const Peer *peer) ;

extern int acp_sendBufArrPackI1List(char cmd, size_t buf_size, const I1List *data, const Peer *peer) ;

extern int acp_sendBufArrPackI2List(char cmd, size_t buf_size, const I2List *data, const Peer *peer) ;

extern void acp_sendStr(const char *s, uint8_t *crc, const Peer *peer) ;

extern void acp_sendFooter(int8_t crc, Peer *peer) ;

extern int acp_sendBufPack(char *buf, char qnf, const char *cmd_str, size_t buf_size, const Peer *peer) ;

extern int acp_sendStrPack(char qnf, char *cmd, size_t buf_size, const Peer *peer) ;

extern int acp_recvOK(Peer *peer, size_t buf_size) ;

extern int acp_recvFTS(FTSList *list, char qnf, char *cmd, size_t buf_size, size_t list_max_size, int fd) ;

extern void freePeer(PeerList *list);

extern void acp_printI1(I1List *list) ;

extern void acp_printI2(I2List *list) ;

extern void acp_printI3(I3List *list) ;

#endif /* ACP_H */

