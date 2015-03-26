CC = gcc
CXX = g++

# Files to include
C_SRC  = $(wildcard core/*.c)
C_SRC += $(wildcard lib/*.c)
C_SRC += $(wildcard lib/lux/src/*.c)
CPP_SRC += $(wildcard core/*.cpp)

C_INC  = $(wildcard core/*.h)
C_INC += $(wildcard lib/*.h)
C_INC += $(wildcard lib/lux/inc/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))
OBJECTS += $(patsubst %.cpp,%.o,$(CPP_SRC))

INC  = -Icore -Ilib/lux/inc -Ilib -L/usr/local/lib
LIB  = -lSDL -lSDL_ttf -lSDL_gfx -lm -lpthread -lportaudio

# Assembler, compiler, and linker flags
CXXFLAGS  = -g -O0 $(INC) -Wall 
CFLAGS = $(CXXFLAGS) -std=c99
LFLAGS  = $(CXXFLAGS)

# Targets
all: beat-off
clean:
	-rm -f $(OBJECTS) beat-off
beat-off: $(OBJECTS)
	$(CXX) $(LFLAGS) -g -o beat-off $(OBJECTS) $(LIB)


#%.o: %.c $(C_INC)
#	gcc $(CFLAGS) -std=c99 -c -o $@ $<
#%.o: %.cpp
#	g++ $(CFLAGS) -c -o $@ $<
