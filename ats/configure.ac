# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.57)

# initialize everything
AC_INIT(ats, 1.0)
AC_CONFIG_SRCDIR(src/atsa/atsa.c)
AM_INIT_AUTOMAKE(foreign)

#AC_CONFIG_AUX_DIR(src)
AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB(m, sin)
AC_CHECK_LIB(gslcblas,main)
AC_CHECK_LIB(gsl,main)

GTK_CONFIG=gtk-config
AC_MSG_CHECKING(for GTK 1.x)
if $GTK_CONFIG --version > /dev/null 2>&1; then 
  dnl We need the "%d" in order not to get e-notation on hpux.
  vers=`$GTK_CONFIG --version | awk 'BEGIN { FS = "."; } { printf "%d", ($[1] * 1000 + $[2]) * 1000 + $[3];}'`
  if test "$vers" -le 2000000; then
    AC_MSG_RESULT(found)
  else
    AC_MSG_ERROR(You need GTK+ 1.x to use atsh)
  fi
else
  AC_MSG_ERROR(Did not find GTK+ installed)
fi
GTK_CFLAGS=`$GTK_CONFIG --cflags`
GTK_LIBS=`$GTK_CONFIG --libs`
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h unistd.h])   #fcntl.h limits.h unistd.h)

# Set the gsl flag to no by default
#gsl=no
#AC_ARG_ENABLE(gsl, [  --enable-gsl  Enable linking with gsl and gslcblas if you need them], [gsl=yes])
#if test "x$gsl" = "xyes" ; then
#  GSL_LIBS="-lgsl -lgslcblas"
#else
#  GSL_LIBS=
#fi
#AC_SUBST(GSL_LIBS)

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
#AC_TYPE_MODE_T
#AC_TYPE_OFF_T
#AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
AC_FUNC_MALLOC
AC_FUNC_REALLOC
#AC_FUNC_STRFTIME
#AC_FUNC_VPRINTF
AC_CHECK_FUNCS([floor pow sqrt strerror memset])  #[getcwd getwd strdup strerror])

AC_CANONICAL_HOST
case $host in
  *-*-msdos* | *-*-go32* | *-*-mingw32* | *-*-cygwin* | *-*-windows*)
        AC_DEFINE(WINDOWS, 1, [system type])
    ;;
  *)
        AC_DEFINE(UNIX, 1, [system type])
    ;;
esac

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
