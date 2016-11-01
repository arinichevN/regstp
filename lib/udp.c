#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "udp.h"

int initUDPServer(int *fd, int port) {
    struct sockaddr_in addr;
    if ((*fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("initUDPServer: socket()\n");
        return 0;
    }
    memset((char *) &addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(*fd, (struct sockaddr*) (&addr), sizeof (addr)) == -1) {
        perror("initUDPServer: bind()");
        return 0;
    }
    return 1;
}

void freeSocketFd(int *udp_fd) {
    if (*udp_fd != -1) {
        if(close(*udp_fd)==-1){
            perror("freeSocketFd");
        }
        *udp_fd = -1;
    }
}

int initUDPClient(int *fd, __time_t tmo) {
    if ((*fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        perror("initUDPClient: socket()");
        return 0;
    }
    struct timeval tv = {tmo, 0};
    if (setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof (tv)) < 0) {
        perror("initUDPClient: timeout set failure");
        return 0;
    }
    return 1;
}

int makeUDPClientAddr(struct sockaddr_in *addr, const char *addr_str, int port) {
    memset(addr, 0, sizeof (*addr));
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    if (inet_aton(addr_str, &(addr->sin_addr)) == 0) {
        perror("makeUDPClientAddr: check address");
        return 0;
    }
    return 1;
}