################################################################################
#
# libzedmd
#
################################################################################

# Commit of 2026/05/08
LIBZEDMD_VERSION = 24044222a83ffed33f93d9f0a0baa68fe9fa5a47
LIBZEDMD_SITE = $(call github,PPUC,libzedmd,$(LIBZEDMD_VERSION))
LIBZEDMD_LICENSE = GPLv3
LIBZEDMD_LICENSE_FILES = LICENSE
LIBZEDMD_DEPENDENCIES = libserialport sockpp libgpiod cargs libframeutil
LIBZEDMD_INSTALL_STAGING = YES
LIBZEDMD_SUPPORTS_IN_SOURCE_BUILD = NO

LIBZEDMD_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBZEDMD_CONF_OPTS += -DBUILD_STATIC=OFF
LIBZEDMD_CONF_OPTS += -DPLATFORM=linux
LIBZEDMD_CONF_OPTS += -DARCH=$(BUILD_ARCH)
LIBZEDMD_CONF_OPTS += -DPOST_BUILD_COPY_EXT_LIBS=OFF

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
    BUILD_ARCH = x64
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
    BUILD_ARCH = aarch64
endif

$(eval $(cmake-package))
