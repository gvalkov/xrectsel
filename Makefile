CC     ?= gcc
CFLAGS ?= -Wall -lX11 -Wno-unused-variable
PROG   ?= xrect

$(PROG): xrect.c strtonum.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(PROG)

run: $(PROG)
	$(PROG)

all: $(PROG)

.DEFAULT_GOAL: all
.PHONE: all run clean
