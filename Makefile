CC     ?= gcc
CFLAGS ?= -Wall -lX11
PROG   ?= xrect

$(PROG): xrect.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(PROG) xrect.o

run: $(PROG)
	$(PROG)

all: $(PROG)

.DEFAULT_GOAL: all
.PHONE: all run clean
