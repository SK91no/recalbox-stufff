################################################################################
#
# libaltsound
#
################################################################################

# Commit of 2026/05/17
LIBALTSOUND_VERSION = f4b790a19ae45a9f93ae0051df6933800c7a6446
LIBALTSOUND_SITE = $(call github,vpinball,libaltsound,$(LIBALTSOUND_VERSION))
LIBALTSOUND_LICENSE = BSD-3-Clause
LIBALTSOUND_LICENSE_FILES = LICENSE
LIBALTSOUND_DEPENDENCIES =
LIBALTSOUND_SUPPORTS_IN_SOURCE_BUILD = NO
LIBALTSOUND_INSTALL_STAGING = YES

LIBALTSOUND_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBALTSOUND_CONF_OPTS += -DBUILD_STATIC=OFF
LIBALTSOUND_CONF_OPTS += -DPLATFORM=linux
LIBALTSOUND_CONF_OPTS += -DARCH=$(BUILD_ARCH)

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
    BUILD_ARCH = x64
    BASS_ARCH = x86_64
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
    BUILD_ARCH = aarch64
    BASS_ARCH = aarch64
endif

$(eval $(cmake-package))
