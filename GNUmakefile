CC     ?= gcc
PROG   ?= xrect
CFLAGS ?= -Wall -lX11 \
          -Wno-unused-variable \
          -Wno-format-security \
          -I/usr/local/include \
          -L/usr/local/lib

$(PROG): xrect.c strtonum.c
	$(CC) -g $(CFLAGS) $^ -o $@

clean:
	rm -f $(PROG)

run: $(PROG)
	$(PROG)

all: $(PROG)

.DEFAULT_GOAL: all
.PHONY: all run clean
