################################################################################
#
# XPI_GAMECON_RPI Recalbox Team 2020
#
################################################################################

XPI_GAMECON_RPI_VERSION = v1.2
XPI_GAMECON_RPI_SITE = https://gitlab.com/recalbox/packages/hardware/xpi_gamecon_rpi.git
XPI_GAMECON_RPI_SITE_METHOD = git
XPI_GAMECON_RPI_DEPENDENCIES = linux
XPI_GAMECON_RPI_LICENSE = GPL-3.0

XPI_GAMECON_RPI_USER_EXTRA_CFLAGS = -Wno-int-conversion
XPI_GAMECON_RPI_MODULE_MAKE_OPTS = EXTRA_CFLAGS="$(XPI_GAMECON_RPI_USER_EXTRA_CFLAGS)"

$(eval $(kernel-module))
$(eval $(generic-package))
