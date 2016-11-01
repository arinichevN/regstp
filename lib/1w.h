#ifndef LIBPAS_1W_H
#define LIBPAS_1W_H


#define OW_CMD_READ_ROM 0x33
#define OW_CMD_MATCH_ROM 0x55
#define OW_CMD_SKIP_ROM 0xCC


extern int onewire_reset(int pin) ;


extern void onewire_send_bit(int pin,int bit) ;


extern void onewire_send_byte(int pin,int b) ;


extern int onewire_read_bit(int pin) ;


extern uint8_t onewire_read_byte(int pin) ;


extern uint8_t onewire_crc_update(uint8_t crc, uint8_t b) ;


extern int onewire_skip(int pin) ;


extern int onewire_read_rom(int pin, uint8_t * buf) ;


extern int onewire_match(int pin, const uint8_t * addr) ;

#endif /* 1W_H */

