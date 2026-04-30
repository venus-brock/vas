PREFIX = /usr
INCS = -I/usr/include/jack -I/usr/include/X11
LIBS = -ljack -lX11 -lm

CFLAGS = -std=c99 -D_XOPEN_SOURCE=500 -Wall -Wextra -pedantic -Wno-unused-function -Wno-unused-parameter ${INCS}
LDFLAGS = ${LIBS}

SRC = src/vas.c src/vas.h src/preset.c src/vas_gui.c

CC = cc
