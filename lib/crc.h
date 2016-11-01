

#ifndef PASLIB_CRC_H
#define PASLIB_CRC_H

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>
#include "util.h"

extern void crc_update(uint8_t *crc, uint8_t b) ;

extern int crc_check(const char * buf, size_t buflen) ;

extern void crc_update_by_str(uint8_t *crc, const char *str);

#endif /* CRC_H */

