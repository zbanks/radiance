PROJECT = radiance
#CC = gcc
#CC = clang

# Source files
C_SRC  = $(wildcard *.c)
C_SRC += $(wildcard audio/*.c)
C_SRC += $(wildcard midi/*.c)
C_SRC += $(wildcard output/*.c)
C_SRC += $(wildcard pattern/*.c)
C_SRC += $(wildcard time/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard util/*.c)

C_SRC += liblux/lux.c liblux/crc.c
C_SRC += $(wildcard BTrack/src/*.c)

CXX_SRC = $(wildcard *.cpp)
CXX_SRC += $(wildcard util/*.cpp)
CXX_SRC += $(wildcard ui/*.cpp)
CXX_SRC += $(wildcard pattern/*.cpp)
CXX_SRC += $(wildcard output/*.cpp)
CXX_SRC += $(wildcard audio/*.cpp)
CXX_SRC += $(wildcard midi/*.cpp)
CXX_SRC += $(wildcard time/*.cpp)

OBJDIR = build
$(shell mkdir -p $(OBJDIR) >/dev/null)
OBJECTS = $(C_SRC:%.c=$(OBJDIR)/%.o) $(CXX_SRC:%.cpp=$(OBJDIR)/%.o)


# Compiler flags
INC = -I.

LIBRARIES = -lSDL2 -lSDL2_ttf -lGL -lGLU -lm -lportaudio -lportmidi -lfftw3 -lsamplerate $(shell pkg-config --libs sdl2 gl glu fftw3)

CFLAGS = -std=gnu11 -ggdb3 -O3 $(INC) $(shell pkg-config --cflags sdl2 gl glu fftw3)
CXXFLAGS = -std=gnu++14 -ggdb3 -O3 $(INC) $(shell pkg-config --cflags sdl2 gl glu fftw3)

CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CXXFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -D_DEFAULT_SOURCE
CXXFLAGS += -D_DEFAULT_SOURCE
LFLAGS = $(CFLAGS)

# File dependency generation
DEPDIR = .deps
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPS = $(OBJECTS:$(OBJDIR)/%.o=$(DEPDIR)/%.d)
-include $(DEPS)
$(DEPDIR)/%.d : %.c .deps
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -MM -MT $(@:$(DEPDIR)/%.d=%.o) >$@
$(DEPDIR)/%.d : %.cpp .deps
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) $< -MM -MT $(@:$(DEPDIR)/%.d=%.o) >$@

# Targets
$(PROJECT): $(OBJECTS)
	$(CXX) $(LFLAGS) -o $(PROJECT) $(OBJECTS) $(LIBRARIES)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(APP_INC) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(APP_INC) -c -o $@ $<


.PHONY: all
all: $(PROJECT)

.PHONY: clean
clean:
	-rm -f $(PROJECT) tags
	-rm -rf $(OBJDIR) $(DEPDIR)

tags: $(C_SRC) $(CXX_SRC)
	ctags --exclude='beat-off/*' -R .

.DEFAULT_GOAL := all
