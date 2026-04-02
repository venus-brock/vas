bin/vas: src/vas.c src/vas_gui.c src/vas.h src/preset.c
	mkdir bin -p
	cc -std=c99 -D_XOPEN_SOURCE=500 -o bin/vas -I /usr/include/jack -ljack -I /usr/include/X11 \
		-lX11 -lm src/vas.c src/vas_gui.c src/preset.c

install: bin/vas ${SUDO_HOME}/.config/vas/presets/
	chown -R ${SUDO_USER} bin
	chown -R ${SUDO_USER} ${SUDO_HOME}/.config/vas
	install -m 755 bin/vas /usr/bin

${SUDO_HOME}/.config/vas/presets/: presets
	mkdir ${SUDO_HOME}/.config/vas/presets/ -p
	install -m 644 presets/* ${SUDO_HOME}/.config/vas/presets/
