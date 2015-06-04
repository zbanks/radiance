CC = gcc

# Files to include
C_SRC = $(wildcard *.c)
C_SRC += ../core/parameter.c
C_SRC += ../patterns/pattern.c
#C_SRC  += $(wildcard ../core/*.c)
#C_SRC += $(wildcard dynamic/*.c)
#C_SRC += $(wildcard filters/*.c)
#C_SRC += $(wildcard hits/*.c)
#C_SRC += $(wildcard lib/*.c)
#C_SRC += $(wildcard midi/*.c)
#C_SRC += $(wildcard output/*.c)
#C_SRC += $(wildcard patterns/*.c)
#C_SRC += $(wildcard signals/*.c)
#C_SRC += $(wildcard timebase/*.c)
#C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard ../util/*.c)
#C_SRC += $(wildcard waveform/*.c)
#C_SRC += $(wildcard lib/lux/src/*.c)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))
DEPS = $(OBJECTS:.o=.d)

INC  = -I.. -L/usr/local/lib -L/usr/lib 
LIB  = -lm -lSDL

# Assembler, compiler, and linker flags
CFLAGS  = -g -O3 $(INC) -Wall -Wextra -Werror -std=c99 -fPIC -DSHAREDLIB

-include $(DEPS)
%.d : %.c
	@$(CC) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

# Targets
.PHONY: all
all: live.so

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(DEPS) live.so

live.so: $(OBJECTS)
	$(CC) -shared $(LFLAGS) -g -o $@ $^ $(LIB)


#%.o: %.c $(C_INC)
#	gcc $(CFLAGS) -std=c99 -c -o $@ $<
#%.o: %.cpp
#	g++ $(CFLAGS) -c -o $@ $<

.DEFAULT_GOAL := all
