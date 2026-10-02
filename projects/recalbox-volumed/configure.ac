AC_PREREQ([2.72])
AC_INIT([volumed], [2.0], [davidb@230ruedubac.fr])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AC_CONFIG_SRCDIR([src/])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(pulse, pa_mainloop_new)
AC_CHECK_LIB(m, rintf)

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdint.h sys/ioctl.h syslog.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([alarm memset select strdup strerror sigabbrev_np])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

