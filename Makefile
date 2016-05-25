PROJECT = radiance
CC = gcc

C_SRC  = $(wildcard *.c)
C_SRC += $(wildcard pattern/*.c)
C_SRC += $(wildcard time/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard util/*.c)

OBJECTS = $(C_SRC:.c=.o)

INC = -I. -I/usr/include -I/usr/include/SDL2

LIBRARIES = -lSDL2 -lGL -lGLU -lm

CFLAGS = -std=c99 -ggdb3 -Og $(INC)
CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -D_POSIX_C_SOURCE=20160524
LFLAGS = $(CFLAGS)

all: $(PROJECT)
clean:
	-rm -f $(OBJECTS) $(PROJECT) tags
$(PROJECT): $(OBJECTS)
	$(CC) $(LFLAGS) -o $(PROJECT) $(OBJECTS) $(LIBRARIES)
%.o: %.c
	$(CC) $(CFLAGS) $(APP_INC) -c -o $@ $<

tags: $(C_SRC)
	ctags --exclude='beat-off/*' -R .
