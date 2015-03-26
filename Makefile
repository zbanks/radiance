# Files to include
C_SRC  = $(wildcard core/*.c)
C_SRC += $(wildcard lib/*.c)
C_SRC += $(wildcard lib/lux/src/*.c)

C_INC  = $(wildcard core/*.h)
C_INC += $(wildcard lib/*.h)
C_INC += $(wildcard lib/lux/inc/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))

INC  = -Icore -Ilib/lux/inc -Ilib -L/usr/local/lib
LIB  = -lSDL -lSDL_ttf -lSDL_gfx -lm -lpthread -lportaudio

# Assembler, compiler, and linker flags
CFLAGS  = -g -O0 $(INC) -std=c99 -Wall
LFLAGS  = $(CFLAGS)

# Targets
all: beat-off
clean:
	-rm -f $(OBJECTS) beat-off
beat-off: $(OBJECTS)
	gcc $(LFLAGS) -g -o beat-off $(OBJECTS) $(LIB)
%.o: %.c $(C_INC)
	gcc $(CFLAGS) -c -o $@ $<
