#include "crc.h"

void crc_update(uint8_t *crc, uint8_t b) {
    int i;
    for (i = 8; i; i--) {
        *crc = ((*crc ^ b) & 1) ? (*crc >> 1) ^ 0b10001100 : (*crc >> 1);
        b >>= 1;
    }
}

int crc_check(const char * buf, size_t buflen) {
    uint8_t crc, crc_fact = 0;
    int state, i = 0, found = 0;
    for (i = 0, found = 0, state = 1; i < buflen; i++) {
        switch (state) {
            case 1:
                if (buf[i] == '\n') {
                    state = 2;
                }
                crc_update(&crc_fact, buf[i]);
                break;
            case 2:
                if (buf[i] == '\n') {
                    state = 3;
                } else {
                    state = 1;
                }
                crc_update(&crc_fact, buf[i]);
                break;
            case 3:
                crc = buf[i];
                found = 1;
                break;
            default:
#ifdef MODE_DEBUG
                fputs("crc_check: wrong state", stderr);
#endif
                break;
        }
        if (found) {
            break;
        }
    }
    if (found && crc == crc_fact) {
        return 1;
    } else {
#ifdef MODE_DEBUG
        fprintf(stderr, "crc_found: %d, crc: %hhd, crc_fact: %hhd\n", found, crc, crc_fact);
#endif
        return 0;
    }
}

void crc_update_by_str(uint8_t *crc, const char *str) {
    int i;
    for (i = 0; i < strlen(str); i++) {
        crc_update(crc, (uint8_t) str[i]);
    }
}