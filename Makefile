CC = gcc
CXX = g++

# Files to include
C_SRC  = $(wildcard core/*.c)
C_SRC += $(wildcard filters/*.c)
C_SRC += $(wildcard hits/*.c)
C_SRC += $(wildcard lib/*.c)
C_SRC += $(wildcard midi/*.c)
C_SRC += $(wildcard output/*.c)
C_SRC += $(wildcard patterns/*.c)
C_SRC += $(wildcard signals/*.c)
C_SRC += $(wildcard timebase/*.c)
C_SRC += $(wildcard waveform/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard util/*.c)
C_SRC += $(wildcard lib/lux/src/*.c)
CPP_SRC += $(wildcard filters/*.cpp)

C_INC  = $(wildcard core/*.h)
C_INC += $(wildcard filters/*.h)
C_INC += $(wildcard lib/*.h)
C_INC += $(wildcard midi/*.h)
C_INC += $(wildcard output/*.h)
C_INC += $(wildcard patterns/*.h)
C_INC += $(wildcard signals/*.h)
C_INC += $(wildcard timebase/*.h)
C_INC += $(wildcard waveform/*.h)
C_INC += $(wildcard ui/*.h)
C_INC += $(wildcard util/*.h)
C_INC += $(wildcard hits/*.h)
C_INC += $(wildcard lib/lux/inc/*.h)

OBJECTS = $(patsubst %.c,%.o,$(C_SRC))
OBJECTS += $(patsubst %.cpp,%.o,$(CPP_SRC))
DEPS = $(OBJECTS:.o=.d)

INC  = -I. -Ilib/lux/inc -Ilib -L/usr/local/lib -L/usr/lib 
LIB  = -lm -lSDL -lSDL_ttf -lSDL_gfx -lpthread -lportaudio -lvamp-hostsdk -lportmidi #-lporttime

# Assembler, compiler, and linker flags
CXXFLAGS  = -g -O3 $(INC) -Wall -Wextra -DLUX_WRITE_ONLY
CFLAGS = $(CXXFLAGS) -std=c99
LFLAGS  = $(CXXFLAGS)

-include $(DEPS)
%.d : %.c
	@$(CXX) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@
%.d : %.cpp
	@$(CXX) $(CXXFLAGS) $< -MM -MT $(@:.d=.o) >$@

ui/layout.o: ui/layout.c
	$(CC) $(CFLAGS) -Wno-missing-field-initializers $(INC) $(LIB) -c -o $@ $<
midi/midi.o: midi/midi.c
	$(CC) -g -O0 $(INC) -Wall -Wextra -std=c99 $(INC) $(LIB) -c -o $@ $<

ui/ui.o: ui/ui.c
	$(CC) -g -O0 $(INC) -Wall -Wextra -std=c99 $(INC) $(LIB) -c -o $@ $<

timebase/timebase.o: timebase/timebase.c
	$(CC) -g -O0 $(INC) -Wall -Wextra -std=c99 $(INC) $(LIB) -c -o $@ $<

# Targets
.PHONY: all
all: beat-off

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(DEPS) beat-off

beat-off: $(OBJECTS)
	$(CXX) $(LFLAGS) -g -o beat-off $(OBJECTS) $(LIB)


#%.o: %.c $(C_INC)
#	gcc $(CFLAGS) -std=c99 -c -o $@ $<
#%.o: %.cpp
#	g++ $(CFLAGS) -c -o $@ $<

.DEFAULT_GOAL := beat-off
