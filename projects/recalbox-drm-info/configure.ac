AC_INIT([recalbox-drm-info], [0.1.0], [davidb@230ruedubac.fr])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_CHECK_HEADERS([fcntl.h unistd.h stdlib.h stdio.h X11/Xlib.h X11/Xatom.h X11/extensions/Xrandr.h xf86drm.h xf86drmMode.h])
PKG_CHECK_MODULES([LIBDRM], [libdrm])
PKG_CHECK_MODULES([X11], [x11])
PKG_CHECK_MODULES([XRANDR], [xrandr])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
