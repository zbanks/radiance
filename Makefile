PROJECT = radiance

C_SRC  = $(wildcard *.c)
C_SRC += $(wildcard util/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard deck/*.c)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))

INC = -I. -I/usr/include/SDL2

LIBRARIES = -lSDL2 -lGL -lGLU

CFLAGS = -std=c99 -Wall -g -O1 $(INC)
LFLAGS = $(CFLAGS)

all: $(PROJECT) tags
clean:
	-rm -f *.o util/*.o ui/*.o $(PROJECT) tags
$(PROJECT): $(OBJECTS)
	$(CC) $(LFLAGS) -o $(PROJECT) $(OBJECTS) $(LIBRARIES)
%.o: %.c
	$(CC) $(CFLAGS) $(APP_INC) -c -o $@ $<

tags: $(C_SRC)
	ctags --exclude='beat-off/*' -R .
