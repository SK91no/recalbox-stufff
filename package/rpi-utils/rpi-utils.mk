################################################################################
#
# rpi-utils
#
################################################################################

RPI_UTILS_VERSION = 193d1bec1c6db7e29b7ac4732e7bb396fbdd843d
RPI_UTILS_SITE = $(call github,raspberrypi,utils,$(RPI_UTILS_VERSION))
RPI_UTILS_LICENSE = BSD-3-Clause
RPI_UTILS_LICENSE_FILES = LICENCE
RPI_UTILS_INSTALL_STAGING = YES
RPI_UTILS_DEPENDENCIES = dtc

$(eval $(cmake-package))
