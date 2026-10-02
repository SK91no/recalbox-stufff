################################################################################
#
# libvni
#
################################################################################

# Commit of 2026/05/09
LIBVNI_VERSION = 99a4ce0c844af75604546d8a4d0a2f98a485e2db
LIBVNI_SITE = $(call github,PPUC,libvni,$(LIBVNI_VERSION))
LIBVNI_LICENSE = GPL-2.0
LIBVNI_LICENSE_FILES = LICENSE.md
LIBVNI_DEPENDENCIES =
LIBVNI_SUPPORTS_IN_SOURCE_BUILD = NO
LIBVNI_INSTALL_STAGING = YES

LIBVNI_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBVNI_CONF_OPTS += -DBUILD_SHARED=ON
LIBVNI_CONF_OPTS += -DBUILD_STATIC=OFF
LIBVNI_CONF_OPTS += -DPLATFORM=linux
LIBVNI_CONF_OPTS += -DARCH=$(BUILD_ARCH)

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
    BUILD_ARCH = x64
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
    BUILD_ARCH = aarch64
endif

$(eval $(cmake-package))
