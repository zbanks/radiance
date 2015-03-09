# Files to include
C_SRC  = $(wildcard core/*.c)

C_INC  = $(wildcard core/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))

INC  = -Icore
LIB  = -lSDL2 -lSDL2_ttf -lm

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
