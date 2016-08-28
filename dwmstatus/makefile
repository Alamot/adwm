# Compiler : gcc, g++, ...
CC=gcc
# Extension
EXT=.c
# Library flags : -lgl, -lpthread, ...
LDFLAGS=$(shell pkg-config --libs x11 libnl-genl-3.0)
# Debug flags
DEBUG_FLAGS=-DDEBUG=1 -std=c99 -Wall -Wextra -Wpedantic -g 
# Other flags
FLAGS=-std=c99 -Wall -Wpedantic -Wno-unused-parameter

include config.mk
#INCLUDE_DIR=include
SOURCE_DIR=src

PROG_NAME=dwmstatus
SRCS=\
dwmstatus.c

################################################################
#
################################################################

ifeq (${DEBUG},1)
	FLAGS+=${DEBUG_FLAGS}
endif
FLAGS += $(shell pkg-config --cflags x11 libnl-genl-3.0)
OBJS=${SRCS:${EXT}=.o}


${PROG_NAME}: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS} ${FLAGS}

%.o: ${SOURCE_DIR}/%${EXT}
	${CC} -o $@ -c $< ${FLAGS}

install: ${PROG_NAME}
	@echo installing executable file to ${PREFIX}/bin
	@mkdir -p ${PREFIX}/bin
	@cp -f ${PROG_NAME} ${PREFIX}/bin/${PROG_NAME}
	@chmod 755 ${PREFIX}/bin/${PROG_NAME}

mrproper:
	rm -rf *.o *~ \#*\# ${PROG_NAME}

clean:
	rm -rf *.o
