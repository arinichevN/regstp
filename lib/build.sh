#!/bin/bash

MODE_DEBUG=-DMODE_DEBUG
PLATFORM_ALL=-DP_ALL
PLATFORM_A20=-DP_A20
PSQL_I_DIR_A20=-I/usr/include/postgresql
PSQL_I_DIR_ALL=-I/opt/PostgreSQL/9.5/include 
PSQL_L_DIR_A20=-L/opt/PostgreSQL/9.5/lib
PSQL_L_DIR_ALL=-L/opt/PostgreSQL/9.5/lib
NONE=""

#    1         2        3     4
#platform debug_mode psql_I psql_L
function build {
	gcc $1 $2 -c 1w.c && \
	gcc $1 $2 -c app.c -D_REENTRANT  -lpthread && \
	gcc $1 $2 -c crc.c && \
	gcc $1 $2 -c db.c $3 $4 -lpq && \
	gcc $1 $2 -c i2c.c && \
	gcc $1 $2 -c gpio.c && \
	gcc $1 $2 -c pid.c && \
	gcc $1 $2 -c timef.c && \
	gcc $1 $2 -c udp.c && \
	gcc $1 $2 -c util.c && \
	gcc $1 $2 -c pm.c && \
	gcc $1 $2 -c pwm.c && \
	gcc $1 $2 -c ds18b20.c && \
	gcc $1 $2 -c config.c $3 $4 -lpq && \
	cd acp && \
	gcc $1 $2 -c main.c && \
	#gcc $1 $2 -c ds18b20.c && \
	cd ../ && \
	echo "library: making archive..." && \
	rm -f libpac.a
	ar -crv libpac.a 1w.o app.o crc.o db.o gpio.o i2c.o pid.o timef.o udp.o util.o pm.o pwm.o ds18b20.o config.o acp/main.o && \
	echo "library: done"
	rm -f *.o acp/*.o
}

function for_all_debug {
    build $PLATFORM_ALL $MODE_DEBUG $PSQL_I_DIR_ALL $PSQL_L_DIR_ALL
}

function for_a20_debug {
    build $PLATFORM_A20 $MODE_DEBUG $PSQL_I_DIR_A20 $PSQL_L_DIR_A20
}

function for_all {
    build $PLATFORM_ALL $NONE $PSQL_I_DIR_ALL $PSQL_L_DIR_ALL
}

function for_a20 {
    build $PLATFORM_A20 $NONE $PSQL_I_DIR_A20 $PSQL_L_DIR_A20
}

f=$1
${f}
