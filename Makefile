# Makefile for hc2.

PROG = hc

CC = msp430-gcc
LD = msp430-gcc
OBJCOPY = msp430-objcopy

ELFPROGRAM = $(PROG).elf
SRCS = $(PROG).c bsp-449STK2.c \
	buttons.c \
	lcd-449STK2.c \
	morse.c \
	rtc.c \
	time.c \
	ui.c \
	qpn/source/qepn.c qpn/source/qfn.c \

ifeq ($(SERIAL),yes)
SRCS += serial-MSP430F449.c
endif
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
DEPDEPS = Makefile

CFLAGS = -std=gnu99 -mmcu=msp430f449 -Os -g -Wall -Werror -Iqpn/include -I.
ifeq ($(SERIAL),yes)
CFLAGS += -DSERIAL
endif
ifeq ($(LED),yes)
CFLAGS += -DLED
endif
LDFLAGS = -mmcu=msp430f449

%.d: %.c $(DEPDEPS)
	@echo DEP: $<
	@$(CC) -E -M $(CFLAGS) $< > $@

.PHONY: default
default: $(ELFPROGRAM)

$(ELFPROGRAM): $(OBJS)
	$(LD) $(LDFLAGS) -o $(ELFPROGRAM) $(OBJS)

# We require these two manual dependencies so that the DEP targets work
# properly.
ui.c: bsp-MCP9701A-temperature-scale.h
bsp-449STK2.c: bsp-MCP9701A-temperature-scale.inc

bsp-MCP9701A-temperature-scale.inc bsp-MCP9701A-temperature-scale.h: bsp-MCP9701A-temperature-scale-creator.py
	./$<

.PHONY: flash
flash: $(ELFPROGRAM)
	mspdebug -j olimex "prog $(ELFPROGRAM)"

.PHONY: clean
clean:
	rm -f $(ELFPROGRAM) $(OBJS) $(DEPS) \
		bsp-*-temperature-scale.inc \
		bsp-*-temperature-scale.h

# Put this late so the first .o target does not become the default.
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

