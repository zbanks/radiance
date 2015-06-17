AC_INIT([beat-off], [1.0.0], [zbanks@mit.edu])
AC_CONFIG_HEADERS([src/confdefs.h])

AC_PROG_CXX
AC_PROG_CC

AC_SEARCH_LIBS([dlopen], [dl dld], [], [
    AC_MSG_ERROR([unable to find the dlopen() function])
])

AC_SEARCH_LIBS([cos], [m], [], [
    AC_MSG_ERROR([unable to find the cos() function])
])

AC_CHECK_LIB([SDL], [main])
AC_CHECK_LIB([SDL_ttf], [main])
AC_CHECK_LIB([SDL_gfx], [main])
AC_CHECK_LIB([flux], [main])
AC_CHECK_LIB([pthread], [main])
AC_CHECK_LIB([portaudio], [main])
AC_CHECK_LIB([vamp-hostsdk], [main])
AC_CHECK_LIB([portmidi], [main])

AC_OUTPUT([Makefile])
