
#include "i2c.h"

static inline int i2c_smbus_access(int fd, char rw, uint8_t command, int size, union i2c_smbus_data *data) {
    struct i2c_smbus_ioctl_data args;
    args.read_write = rw;
    args.command = command;
    args.size = size;
    args.data = data;
    return ioctl(fd, I2C_SMBUS, &args);
}

int I2CRead(int fd) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(fd, I2C_SMBUS_READ, 0, I2C_SMBUS_BYTE, &data)) {
        return -1;
    } else {
        return data.byte & 0xFF;
    }
}

int I2CReadReg8(int fd, int reg) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(fd, I2C_SMBUS_READ, reg, I2C_SMBUS_BYTE_DATA, &data)) {
        return -1;
    } else {
        return data.byte & 0xFF;
    }
}

int I2CReadReg16(int fd, int reg) {
    union i2c_smbus_data data;
    if (i2c_smbus_access(fd, I2C_SMBUS_READ, reg, I2C_SMBUS_WORD_DATA, &data)) {
        return -1;
    } else {
        return data.word & 0xFFFF;
    }
}

int I2CWrite(int fd, int data) {
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, data, I2C_SMBUS_BYTE, NULL);
}


int I2CWriteReg8(int fd, int reg, int value) {
    union i2c_smbus_data data;

    data.byte = value;
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_BYTE_DATA, &data);
}

int I2CWriteReg16(int fd, int reg, int value) {
    union i2c_smbus_data data;

    data.word = value;
    return i2c_smbus_access(fd, I2C_SMBUS_WRITE, reg, I2C_SMBUS_WORD_DATA, &data);
}

int I2COpen(const char *path, int addr) {
    int fd;

    if ((fd = open(path, O_RDWR)) < 0) {
        perror("I2COpen: open");
        return -1;
    }
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        perror("I2COpen: ioctl");
        return -1;
    }
    return fd;
}

