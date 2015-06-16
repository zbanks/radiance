AUTOMAKE_OPTIONS = foreign subdir-objects
bin_PROGRAMS = beat-off
dist_doc_DATA = README.md util/inih_LICENSE.txt

CXXFLAGS = -Wall -Wextra -Wformat=2 -Wno-format-nonliteral -Wshadow -Wpointer-arith -Wcast-qual  -Wno-missing-braces -Wuninitialized -Werror
CXXFLAGS += -funsafe-loop-optimizations -Wunsafe-loop-optimizations -ffast-math
CFLAGS = $(CXXFLAGS) -std=c99 -D_POSIX_C_SOURCE

beat_off_SOURCES = audio/audio.c audio/input_pa.c \
				  core/config.c core/main.c core/parameter.c core/slot.c \
				  dynamic/object.c \
				  filters/filter.c filters/vamp.cpp \
				  midi/config.c midi/midi.c \
				  output/flux.c output/output.c output/slice.c \
				  patterns/bubble.c patterns/full.c patterns/rainbow.c \
				  patterns/fade.c patterns/pattern.c patterns/sparkle.c \
				  patterns/strobe.c patterns/swipe.c patterns/wave.c \
				  signals/signal.c \
				  state/state.c \
				  timebase/timebase.c \
				  ui/ui.c ui/text.c ui/layout.c \
				  ui/slider.c ui/graph.c ui/waveform.c \
				  util/color.c util/colormaps.c \
				  util/ini.c util/siggen.c util/signal.c \
				  waveform/waveform.c


