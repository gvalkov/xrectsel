CC     ?= gcc
PROG   ?= xrectsel
CFLAGS ?= -Wall -lX11 --std=gnu99 \
          -Wno-unused-variable \
          -Wno-format-security \
          -I/usr/local/include \
          -L/usr/local/lib

$(PROG): xrectsel.c strtonum.c
	$(CC) -g $(CFLAGS) $^ -o $@

clean:
	rm -f $(PROG)

all: $(PROG)

.DEFAULT_GOAL: all
.PHONY: all run clean
