include config.mk

bin/vas: ${SRC}
	mkdir bin -p
	${CC} ${CFLAGS} ${LDFLAGS} -o bin/vas ${SRC}

install: bin/vas ${SUDO_HOME}/.config/vas/presets/
	chown -R ${SUDO_USER} bin
	chown -R ${SUDO_USER} ${SUDO_HOME}/.config/vas
	mkdir -p ${DESTDIR}${PREFIX}/bin
	install -m 755 bin/vas ${DESTDIR}${PREFIX}/bin

${SUDO_HOME}/.config/vas/presets/: presets/*
	mkdir -p ${SUDO_HOME}/.config/vas/presets/
	install -m 644 presets/* ${SUDO_HOME}/.config/vas/presets/
