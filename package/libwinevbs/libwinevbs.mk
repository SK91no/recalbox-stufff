################################################################################
#
# libwinevbs
#
################################################################################

# Commit from platforms/config.sh of vpinball 2026/05/22
LIBWINEVBS_VERSION = 02c353adadcba8158a2a1231f3c9c14f01370ea0
LIBWINEVBS_SITE = $(call github,vpinball,libwinevbs,$(LIBWINEVBS_VERSION))
LIBWINEVBS_LICENSE = LGPL-2.1
LIBWINEVBS_LICENSE_FILES = LICENSE
LIBWINEVBS_DEPENDENCIES =
LIBWINEVBS_SUPPORTS_IN_SOURCE_BUILD = NO
LIBWINEVBS_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
    LIBWINEVBS_BUILD_ARCH = x64
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
    LIBWINEVBS_BUILD_ARCH = aarch64
endif

LIBWINEVBS_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBWINEVBS_CONF_OPTS += -DPLATFORM=linux
LIBWINEVBS_CONF_OPTS += -DARCH=$(LIBWINEVBS_BUILD_ARCH)

$(eval $(cmake-package))
