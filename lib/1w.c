#include <stdint.h>

#include "gpio.h"
#include "1w.h"
#include "timef.h"

int onewire_reset(int pin) {
    int i;
    pinLow(pin);
    delayUsBusy(640); 
    pinHigh(pin);
    pinModeIn(pin);
    for (i = 80; i; i--) {
        if (!digitalRead(pin)) {
            while (!digitalRead(pin)) {
            }
            pinModeOut(pin);
            return 1;
        }
        delayUsBusy(1);
    }
    pinModeOut(pin);
    delayUsBusy(1);
    return 0;
}


/*
 *  works, but many crc mistakes
void onewire_send_bit1(int pin,int bit) {
    pinLow(pin);
    if (bit) {
        delayUsBusy(5); 
        pinHigh(pin);
        delayUsBusy(90); 
    } else {
        delayUsBusy(90); 
        pinHigh(pin);
        delayUsBusy(5); 
    }
}
*/

void onewire_send_bit(int pin,int bit) {
    pinLow(pin);
    if (bit) {
        delayUsBusy(1); 
        pinHigh(pin);
        delayUsBusy(60); 
    } else {
        delayUsBusy(60); 
        pinHigh(pin);
        delayUsBusy(1); 
    }
    delayUsBusy(60);
}


void onewire_send_byte(int pin,int b) {
    int p;
    for (p = 8; p; p--) {
        onewire_send_bit(pin, b & 1);
        b >>= 1;
    }
}


/*
 * works, but many crc mistakes
int onewire_read_bit(int pin) {
    pinLow(pin);
    delayUsBusy(2); 
    pinHigh(pin);
    delayUsBusy(8); 
    pinModeIn(pin);
    int r = digitalRead(pin);
    pinModeOut(pin);
    delayUsBusy(80); 
    return r;
}
*/

int onewire_read_bit(int pin) {
    pinLow(pin);
    delayUsBusy(2); 
    pinHigh(pin);
    delayUsBusy(2); 
    pinModeIn(pin);
    int r = digitalRead(pin);
    pinModeOut(pin);
    delayUsBusy(60); 
    return r;
}


uint8_t onewire_read_byte(int pin) {
    uint8_t r = 0;
    int p;
    for (p = 8; p; p--) {
        r >>= 1;
        if (onewire_read_bit(pin)) {
            r |= 0x80;
        }
    }
    return r;
}


uint8_t onewire_crc_update(uint8_t crc, uint8_t b) {
    int i;
    for (i = 8; i; i--) {
        crc = ((crc ^ b) & 1) ? (crc >> 1) ^ 0b10001100 : (crc >> 1);
        b >>= 1;
    }
    return crc;
}


int onewire_skip(int pin) {
    if (!onewire_reset(pin)) {
        return 0;
    }
    onewire_send_byte(pin, OW_CMD_SKIP_ROM);
    return 1;
}


int onewire_read_rom(int pin, uint8_t * buf) {
    if (!onewire_reset(pin)) {
        return 0;
    }
    onewire_send_byte(pin, OW_CMD_READ_ROM);
    int i;
    for (i = 0; i < 8; i++) {
        *(buf++) = onewire_read_byte(pin);
    }
    return 1;
}


int onewire_match(int pin, const uint8_t * addr) {
    if (!onewire_reset(pin)) {
        return 0;
    }
    onewire_send_byte(pin, OW_CMD_MATCH_ROM);
    int p;
    for (p = 0; p < 8; p++) {
        onewire_send_byte(pin, *(addr++));
    }
    return 1;
}