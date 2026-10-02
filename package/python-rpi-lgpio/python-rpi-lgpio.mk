################################################################################
#
# python-rpi-lgpio
#
################################################################################

PYTHON_RPI_LGPIO_VERSION = 0.6
PYTHON_RPI_LGPIO_SOURCE = release-$(PYTHON_RPI_LGPIO_VERSION).tar.gz
PYTHON_RPI_LGPIO_SITE = https://github.com/waveform80/rpi-lgpio/archive/refs/tags
PYTHON_RPI_LGPIO_LICENSE = MIT
PYTHON_RPI_LGPIO_LICENSE_FILES = LICENCE.txt
PYTHON_RPI_LGPIO_SETUP_TYPE = setuptools
PYTHON_RPI_LGPIO_DEPENDENCIES = python-lgpio

$(eval $(python-package))
