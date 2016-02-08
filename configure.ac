AC_INIT([radiance], [1.0.0], [zbanks@mit.edu], [https://github.com/ervanalb/radiance])
AC_CONFIG_HEADERS([src/confdefs.h])

AC_ARG_WITH([sdl], [
    AS_HELP_STRING([--without-sdl], [Disable SDL and user iterface])])

AC_ARG_WITH([vamp], [
    AS_HELP_STRING([--without-vamp], [Disable VAMP plugins])])

AC_ARG_WITH([flux], [
    AS_HELP_STRING([--without-flux], [Disable Flux output])])

AC_ARG_WITH([portmidi], [
    AS_HELP_STRING([--without-portmidi], [Disable PortMIDI input])])

AC_PROG_CXX
AC_PROG_CC

AC_SEARCH_LIBS([dlopen], [dl dld], [], [
    AC_MSG_ERROR([unable to find the dlopen() function])])

AC_SEARCH_LIBS([cos], [m], [], [
    AC_MSG_ERROR([unable to find the cos() function])])

AC_SEARCH_LIBS([pthread_create], [pthread], [], [
    AC_MSG_ERROR(["unable to find the pthread_create() function"])])
AC_SEARCH_LIBS([Pa_Initialize], [portaudio], [], [
    AC_MSG_ERROR(["unable to find the Pa_Initialize() function (PortAudio)"])])
#AC_SEARCH_LIBS([Pa_ReadStream], [portaudio], [], [
#    AC_MSG_ERROR([unable to find the Pa_ReadStream() function])])
AC_SEARCH_LIBS([src_simple], [samplerate], [], [
    AC_MSG_ERROR(["unable to find the src_simple() function (libresample)"])])
AC_SEARCH_LIBS([fftw_plan_dft_1d], [fftw3], [], [
    AC_MSG_ERROR(["unable to find the fftw_plan_dft_1d() function (libfftw3)"])])

AC_SUBST([SDL_ENABLED], ["no"])
AS_IF([test "x$with_sdl" != "xno"], [
    found_sdl=yes
    AC_SEARCH_LIBS([SDL_Init], [SDL], [], [found_sdl=no])
    AC_SEARCH_LIBS([TTF_Init], [SDL_ttf], [], [found_sdl=no])
    AC_SEARCH_LIBS([hlineRGBA], [SDL_gfx], [], [found_sdl=no])
    AC_SEARCH_LIBS([IMG_Load], [SDL_image], [], [found_sdl=no])
    AS_IF([test "x$found_sdl" == "xyes"], [
        AC_DEFINE([SDL_ENABLED], [1], [SDL UI is enabled])
        AC_SUBST([SDL_ENABLED], ["yes"])
    ], [
        AC_WARN(["Unable to find SDL libraries! Compiling without SDL support!"])
    ])
])

AC_SUBST([VAMP_ENABLED], ["no"])
AS_IF([test "x$with_vamp" != "xno"], [
    AC_CHECK_LIB([vamp-hostsdk], [main], [ 
        LIBS="$LIBS -lvamp-hostsdk"
        AC_DEFINE([VAMP_ENABLED], [1], [VAMP Plugin support is enabled])
        AC_SUBST([VAMP_ENABLED], ["yes"])
    ], [
        AC_WARN(["Unable to find VAMP host sdk library! Compiling without VAMP support!"])
    ])
])

AC_SUBST([FLUX_ENABLED], ["no"])
AS_IF([test "x$with_flux" != "xno"], [
    AC_SEARCH_LIBS([flux_cli_init], [flux], [
        AC_DEFINE([FLUX_ENABLED], [1], [Flux output is enabled])
        AC_SUBST([FLUX_ENABLED], ["yes"])
    ], [
        AC_WARN(["Unable to find flux library! Compiling without Flux support!"])
    ])
])

AC_SUBST([PORTMIDI_ENABLED], ["no"])
AS_IF([test "x$with_portmidi" != "xno"], [
    AC_SEARCH_LIBS([Pm_Initialize], [portmidi], [
        AC_DEFINE([PORTMIDI_ENABLED], [1], [PortMIDI input is enabled])
        AC_SUBST([PORTMIDI_ENABLED], ["yes"])
    ], [
        AC_WARN(["Unable to find PortMIDI library! Compiling without MIDI input support!"])
    ])
])

AC_OUTPUT([Makefile])
