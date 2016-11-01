
#ifndef LIBPAS_DS18B20_H
#define LIBPAS_DS18B20_H

#include <stdio.h>
#include <stdint.h>
#include "timef.h"
#include "1w.h"

#define DS18B20_SCRATCHPAD_BYTE_NUM 8 
#define DS18B20_EEPROM_BYTE_NUM 3
#define DS18B20_SCRATCHPAD_CONFIG_REG 4 
#define DS18B20_RES_9BIT 0x1f 
#define DS18B20_RES_10BIT 0x3f 
#define DS18B20_RES_11BIT 0x5f
#define DS18B20_RES_12BIT 0x7f 
#define DS18B20_CMD_CONVERTT 0x44
#define DS18B20_CMD_WRITE_SCRATCHPAD 0x4E
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE
#define DS18B20_CMD_COPY_SCRATCHPAD 0x48
#define DS18B20_CMD_RECALL 0xB8

extern float dsToFloat(uint16_t v) ;

extern int ds18b20_read_scratchpad(int pin, const uint8_t *addr, uint8_t *sp) ;

extern int ds18b20_write_scratchpad(int pin, const uint8_t *addr, const uint8_t *data) ;

extern int ds18b20_copy_scratchpad(int pin, const uint8_t *addr) ;

extern int ds18b20_recall(int pin, const uint8_t *addr) ;

extern int ds18b20_set_resolution(int pin, const uint8_t *addr, uint8_t res) ;

extern int ds18b20_get_resolution(int pin, const uint8_t *addr, int *res) ;

extern int ds18b20_convert_t(int pin, const uint8_t *addr) ;

extern int ds18b20_get_temp(int pin, const uint8_t *addr, float * temp) ;

extern int ds18b20_parse_resolution(int r) ;

#endif /* DS18B20_H */

