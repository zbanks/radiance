# Files to include
C_SRC  = $(wildcard core/*.c)

C_INC  = $(wildcard core/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))

INC  = -Icore
LIB  = -lSDL -lSDL_ttf -lSDL_gfx -lm -lpthread

# Assembler, compiler, and linker flags
CFLAGS  = -g -O0 $(INC) -std=c99 -Wall
LFLAGS  = $(CFLAGS)

# Targets
all: beat-off
clean:
	-rm -f $(OBJECTS) beat-off
beat-off: $(OBJECTS)
	gcc $(LFLAGS) -o beat-off $(OBJECTS) $(LIB)
%.o: %.c $(C_INC)
	gcc $(CFLAGS) -c -o $@ $<
