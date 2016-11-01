
#ifndef LIBPAS_UTIL_H
#define LIBPAS_UTIL_H

#include <stdio.h>
#include <string.h>
extern void dumpStr(const char *buf) ;

extern void dumpBuf(const char *buf, size_t buf_len); 

extern void strnline(char **v) ;

extern char * bufCat(char * buf, const char * str, size_t size);

#endif 

