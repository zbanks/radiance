PROJECT = radiance
#CC = gcc
#CC = clang

# Source files
C_SRC  = main.c
C_SRC += $(wildcard audio/*.c)
C_SRC += $(wildcard midi/*.c)
C_SRC += $(wildcard output/*.c)
C_SRC += $(wildcard pattern/*.c)
C_SRC += $(wildcard time/*.c)
C_SRC += $(wildcard ui/*.c)
C_SRC += $(wildcard util/*.c)

C_SRC += liblux/lux.c liblux/crc.c
C_SRC += $(wildcard BTrack/src/*.c)

OBJDIR = build
$(shell mkdir -p $(OBJDIR) >/dev/null)
OBJECTS = $(C_SRC:%.c=$(OBJDIR)/%.o)

# Compiler flags
INC = -I.

LIBRARIES = -lSDL2 -lSDL2_ttf -lGL -lGLU -lm -lportaudio -lportmidi -lfftw3 -lsamplerate

CFLAGS = -std=c99 -ggdb3 -O3 $(INC)
CFLAGS += -Wall -Wextra -Werror -Wno-unused-parameter
CFLAGS += -D_POSIX_C_SOURCE=20160524
LFLAGS = $(CFLAGS)

# File dependency generation
DEPDIR = .deps
$(shell mkdir -p $(DEPDIR) >/dev/null)
DEPS = $(OBJECTS:$(OBJDIR)/%.o=$(DEPDIR)/%.d)
-include $(DEPS)
$(DEPDIR)/%.d : %.c .deps
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $< -MM -MT $(@:$(DEPDIR)/%.d=%.o) >$@

# Targets
$(PROJECT): $(OBJECTS)
	$(CC) $(LFLAGS) -o $(PROJECT) $(OBJECTS) $(LIBRARIES)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(APP_INC) -c -o $@ $<

# luxctl utility
luxctl: $(OBJDIR)/luxctl.o $(OBJDIR)/liblux/lux.o $(OBJDIR)/liblux/crc.o
	$(CC) $(LFLAGS) -o $@ $^ 

.PHONY: all
all: $(PROJECT) luxctl

.PHONY: clean
clean:
	-rm -f $(PROJECT) tags luxctl
	-rm -rf $(OBJDIR) $(DEPDIR)

tags: $(C_SRC)
	ctags --exclude='beat-off/*' -R .

.DEFAULT_GOAL := all
