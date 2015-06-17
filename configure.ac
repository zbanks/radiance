AC_INIT([beat-off], [1.0.0], [zbanks@mit.edu], [https://github.com/ervanalb/beat-off])
AC_CONFIG_HEADERS([src/confdefs.h])

AC_ARG_WITH([vamp], [
    AS_HELP_STRING([--disable-vamp], [Disable VAMP plugins])])

AC_ARG_WITH([flux], [
    AS_HELP_STRING([--disable-flux], [Disable Flux output])])

AC_PROG_CXX
AC_PROG_CC

AC_SEARCH_LIBS([dlopen], [dl dld], [], [
    AC_MSG_ERROR([unable to find the dlopen() function])])

AC_SEARCH_LIBS([cos], [m], [], [
    AC_MSG_ERROR([unable to find the cos() function])])

AC_CHECK_LIB([SDL], [SDL_Init])
AC_CHECK_LIB([SDL_ttf], [TTF_Init])
AC_CHECK_LIB([SDL_gfx], [hlineRGBA])
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([portmidi], [Pm_Initialize])
AC_CHECK_LIB([portaudio], [Pa_Initialize])
#AC_SEARCH_LIBS([Pa_ReadStream], [portaudio], [], [
#    AC_MSG_ERROR([unable to find the Pa_ReadStream() function])])

AS_IF([test "x$enable_vamp" != "xno"], [
    AC_CHECK_LIB([vamp-hostsdk], [main])
    AC_DEFINE([VAMP_ENABLED], [], [VAMP Plugin support is enabled])
])

AS_IF([test "x$enable_flux" != "xno"], [
    AC_CHECK_LIB([flux], [flux_cli_init])
    AC_DEFINE([FLUX_ENABLED], [], [Flux output is enabled])
])

AC_OUTPUT([Makefile])
