CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L
LDLIBS = -pthread -lrt

.PHONY: all clean

all: core ui logger

core: core.c sync.c ipc_common.h sim_interface.h sync.h
	$(CC) $(CFLAGS) -o $@ core.c sync.c $(LDLIBS)

ui: ui.c sync.c ipc_common.h sim_interface.h sync.h
	$(CC) $(CFLAGS) -o $@ ui.c sync.c $(LDLIBS)

logger: logger.c ipc_common.h
	$(CC) $(CFLAGS) -o $@ logger.c -lrt

clean:
	rm -f core ui logger simulator.log

