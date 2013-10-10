# Makefile for hc2.

PROG = hc

CC = msp430-gcc
LD = msp430-gcc
OBJCOPY = msp430-objcopy

ELFPROGRAM = $(PROG).elf
SRCS = $(PROG).c bsp-449STK2.c \
	lcd-449STK2.c lcd-449STK2-chars.c \
	morse.c \
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
LDFLAGS = -mmcu=msp430f449

%.d: %.c $(DEPDEPS)
	@echo DEP: $<
	@$(CC) -E -M $(CFLAGS) $< > $@

.PHONY: default
default: $(ELFPROGRAM)

$(ELFPROGRAM): $(OBJS)
	$(LD) $(LDFLAGS) -o $(ELFPROGRAM) $(OBJS)

.PHONY: flash
flash: $(ELFPROGRAM)
	mspdebug -j olimex "prog $(ELFPROGRAM)"

.PHONY: clean
clean:
	rm -f $(ELFPROGRAM) $(OBJS) $(DEPS)

# Put this late so the first .o target does not become the default.
ifneq ($(MAKECMDGOALS),clean)
-include $(DEPS)
endif

