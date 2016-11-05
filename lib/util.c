#include "util.h"

void dumpStr(const char *buf) {
    int i;
    for (i = 0; i < strlen(buf); i++) {
        printf("%hhu.", buf[i]);
    }
    putchar('\n');
}

void dumpBuf(const char *buf, size_t buf_len) {
    int i, state = 0;
    for (i = 0; i < buf_len; i++) {
        switch (state) {
            case 0:
                if (buf[i] == '\n') {
                    state = 1;
                }
                break;
            case 1:
                if (buf[i] == '\n') {
                    state = 2;
                }else{
                    state=0;
                }
                break;
            case 2:
                state = 3;
                break;
            case 3:
                putchar('\n');
                return;

        }
        printf("%hhu.", buf[i]);
    }
}

void strnline(char **v) {
    char *c;
    c = strchr(*v, '\n');
    if (c == NULL) {
        *v = strchr(*v, '\0');
    } else {
        *v = c + 1;
        if(**v=='\n'){
            *v = strchr(*v, '\0');
        }
    }
}

char * bufCat(char * buf, const char *str, size_t size) {
    if (strlen(buf) + strlen(str) + 1 >= size) {
#ifdef MODE_DEBUG
        fputs("bufCat: buffer overflow\n", stderr);
#endif
        return NULL;
    }
    return strcat(buf, str);
}
