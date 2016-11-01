
#ifndef LIBPAS_I2C_H
#define LIBPAS_I2C_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>



// I2C definitions

#define I2C_SLAVE 0x0703
#define I2C_SMBUS 0x0720 /* SMBus-level access */

#define I2C_SMBUS_READ 1
#define I2C_SMBUS_WRITE 0

// SMBus transaction types

#define I2C_SMBUS_QUICK      0
#define I2C_SMBUS_BYTE      1
#define I2C_SMBUS_BYTE_DATA     2
#define I2C_SMBUS_WORD_DATA     3
#define I2C_SMBUS_PROC_CALL     4
#define I2C_SMBUS_BLOCK_DATA     5
#define I2C_SMBUS_I2C_BLOCK_BROKEN  6
#define I2C_SMBUS_BLOCK_PROC_CALL   7  /* SMBus 2.0 */
#define I2C_SMBUS_I2C_BLOCK_DATA    8

// SMBus messages

#define I2C_SMBUS_BLOCK_MAX 32 /* As specified in SMBus standard */
#define I2C_SMBUS_I2C_BLOCK_MAX 32 /* Not specified but we use same structure */

// Structures used in the ioctl() calls

union i2c_smbus_data {
    uint8_t byte;
    uint16_t word;
    uint8_t block [I2C_SMBUS_BLOCK_MAX + 2]; // block [0] is used for length + one more for PEC
};

struct i2c_smbus_ioctl_data {
    char read_write;
    uint8_t command;
    int size;
    union i2c_smbus_data *data;
};

extern int I2CRead(int fd);

extern int I2CReadReg8(int fd, int reg);

extern int I2CReadReg16(int fd, int reg);

extern int I2CWrite(int fd, int data);

extern int I2CWriteReg8(int fd, int reg, int value);

extern int I2CWriteReg16(int fd, int reg, int value);

extern int I2COpen(const char *path, int addr);

#endif 

