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
C_SRC += $(wildcard text/*.c)

C_SRC += liblux/lux.c liblux/crc.c
C_SRC += $(wildcard submodules/BTrack/src/*.c)

CXX_SRC = $(wildcard *.cpp)
CXX_SRC += $(wildcard util/*.cpp)
CXX_SRC += $(wildcard ui/*.cpp)
CXX_SRC += $(wildcard pattern/*.cpp)
CXX_SRC += $(wildcard output/*.cpp)
CXX_SRC += $(wildcard audio/*.cpp)
CXX_SRC += $(wildcard midi/*.cpp)
CXX_SRC += $(wildcard time/*.cpp)
ifeq ($(FONTS),ftgl)
	CXX_SRC += text/ftgl.cpp
else
	CXX_SRC += text/embedded.cpp
endif


C_SRC += submodules/gl3w/src/gl3w.c

OBJDIR = build
$(shell mkdir -p $(OBJDIR) >/dev/null)
OBJECTS = $(C_SRC:%.c=$(OBJDIR)/%.o) $(CXX_SRC:%.cpp=$(OBJDIR)/%.o)

# Compiler flags
INC = -I. -I./submodules -I./submodules/gl3w/include -I./submodules/freetype-gl -I./submodules/BTrack

LIBRARIES = -lrt -lfreetype -lSDL2 -lGL -lGLU -lrt -ldl -lm -lportaudio -lportmidi -lsamplerate $(shell pkg-config --libs sdl2 gl glu fftw3 fftw3f)

CFLAGS = -std=gnu11 -ggdb3 -O3 $(INC) $(shell pkg-config --cflags sdl2 gl glu fftw3)
CXXFLAGS = -std=gnu++14 -ggdb3 -O3 $(INC) $(shell pkg-config --cflags sdl2 gl glu fftw3)

CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-error=cpp
CXXFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter -Wno-narrowing -Wno-missing-field-initializers -Wno-error=cpp
CFLAGS += -D_DEFAULT_SOURCE
CXXFLAGS += -D_DEFAULT_SOURCE
ifeq ($(FONTS),ftgl)
	CXXFLAGS+=-DUSE_FREETYPE_GL=1
	CFLAGS+=-DUSE_FREETYPE_GL=1
	LIBRARIES+= -lfreetype-gl
	CFLAGS+= -L./submodules/freetype-gl/build -lfreetype-gl
else
	CXXFLAGS+=-UUSE_FREETYPE_GL
	CFLAGS+=-UUSE_FREETYPE_GL
endif
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

submodules/gl3w/src/gl3w.c: submodules/gl3w/gl3w_gen.py
	cd submodules/gl3w/ && ./gl3w_gen.py && cd ../../

# Targets
$(PROJECT): $(OBJECTS)
	$(CXX) -pthread -lpthread -lrt -lm -ldl $(LFLAGS) -o $(PROJECT) $(OBJECTS) $(LIBRARIES)

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
