
#!/bin/bash
APP=regstp
INST_DIR=/usr/sbin
CONF_DIR=/etc/controller

PSQL_I_DIR_LUBUNTU=-I/usr/include/postgresql
PSQL_I_DIR_XUBUNTU=-I/opt/PostgreSQL/9.5/include 
PSQL_L_DIR_LUBUNTU=-L/opt/PostgreSQL/9.5/lib
PSQL_L_DIR_XUBUNTU=-L/opt/PostgreSQL/9.5/lib


function move_bin {
	([ -d $INST_DIR ] || mkdir $INST_DIR) && \
	cp bin $INST_DIR/$APP && \
	chmod a+x $INST_DIR/$APP && \
	chmod og-w $INST_DIR/$APP && \
	echo "Your $APP executable file: $INST_DIR/$APP";
}

function move_conf {
	([ -d $CONF_DIR ] || mkdir $CONF_DIR) && \
	cp  main.conf $CONF_DIR/$APP.conf && \
	echo "Your $APP configuration file: $CONF_DIR/$APP.conf";
}

#your application will run on OS startup
function conf_autostart {
	cp -v starter_init /etc/init.d/$APP && \
	chmod 755 /etc/init.d/$APP && \
	update-rc.d -f $APP remove && \
	update-rc.d $APP defaults 30 && \
	echo "Autostart configured";
}

function for_all {
	cd lib && \
	./build.sh for_all && \
	cd ../ && \
	gcc -D_REENTRANT -DP_ALL main.c -o bin $PSQL_I_DIR_XUBUNTU $PSQL_L_DIR_XUBUNTU -L./lib -lpq -lpthread -lpac && \
	move_bin && \
	move_conf && \
	conf_autostart && \
	echo "Application $APP successfully installed"
}

function for_all_debug {
	cd lib && \
	./build.sh for_all_debug && \
	cd ../ && \
	gcc -D_REENTRANT -DMODE_DEBUG -DP_ALL main.c -o bin $PSQL_I_DIR_XUBUNTU $PSQL_L_DIR_XUBUNTU -L./lib -lpq -lpthread -lpac && \
	echo "Application $APP successfully installed. Launch command: sudo ./bin"
}

function for_a20 {
	cd lib && \
	./build.sh for_all && \
	cd ../ && \
	gcc -D_REENTRANT -DP_ALL main.c -o bin $PSQL_I_DIR_LUBUNTU $PSQL_L_DIR_LUBUNTU -L./lib -lpq -lpthread -lpac && \
	move_bin && \
	move_conf && \
	conf_autostart && \
	echo "Application $APP successfully installed"
}

function for_a20_debug {
	cd lib && \
	./build.sh for_a20_debug && \
	cd ../ && \
	gcc -D_REENTRANT -DMODE_DEBUG -DP_A20 main.c -o bin $PSQL_I_DIR_LUBUNTU $PSQL_L_DIR_LUBUNTU -L./lib -lpq -lpthread -lpac && \
	echo "Application $APP successfully installed. Launch command: sudo ./bin"
}

f=$1
${f}