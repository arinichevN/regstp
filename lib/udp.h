
#ifndef LIBPAS_UDP_H
#define LIBPAS_UDP_H
#include <stdio.h>
#include <string.h>
#include <unistd.h>
extern int initUDPServer(int *fd, int port);

extern void freeSocketFd(int *udp_fd);

extern int initUDPClient(int *fd, __time_t tmo) ;

extern int makeUDPClientAddr(struct sockaddr_in *addr, const char *addr_str, int port);

#endif

