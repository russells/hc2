# Makefile for hc2.

PROG = hc

CC = msp430-gcc
LD = msp430-gcc
OBJCOPY = msp430-objcopy

ELFPROGRAM = $(PROG).elf
SRCS = $(PROG).c
OBJS = $(SRCS:.c=.o)

CFLAGS = -mmcu=msp430f449 -Os -g
LDFLAGS = -mmcu=msp430f449

.PHONY: default
default: $(ELFPROGRAM)

$(ELFPROGRAM): $(OBJS)
	$(LD) $(LDFLAGS) -o $(ELFPROGRAM) $(OBJS)

.PHONY: flash
flash: $(ELFPROGRAM)
	mspdebug -j olimex "prog $(ELFPROGRAM)"

.PHONY: clean
clean:
	rm -f $(ELFPROGRAM) $(OBJS)

