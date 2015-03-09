# Files to include
C_SRC  = $(wildcard core/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard model/*.c)

C_INC  = $(wildcard core/*.h)
C_INC += $(wildcard ui/*.h)
C_INC += $(wildcard model/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))

INC  = -Icore -Iui -Imodel
LIB  = -lSDL2 -lm

# Assembler, compiler, and linker flags
CFLAGS  = -g -O3 $(INC) -std=c99 -Wall
LFLAGS  = $(CFLAGS)

# Targets
all: beat-off
clean:
	-rm -f $(OBJECTS) beat-off
beat-off: $(OBJECTS)
	gcc $(LFLAGS) -o beat-off $(OBJECTS) $(LIB)
%.o: %.c $(C_INC)
	gcc $(CFLAGS) -c -o $@ $<
