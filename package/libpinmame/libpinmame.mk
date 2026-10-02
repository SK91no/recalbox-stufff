################################################################################
#
# libpinmame
#
################################################################################

# Commit of 2026/07/15. Must match PINMAME_SHA in vpinball's platforms/config.sh:
# vpinball needs PinmameSetMsgAPI() to let libpinmame publish its DMD on the
# plugin message bus, and that only lives in this fork -- vpinball/pinmame has
# never merged it, so a DMD is simply never rendered when building against it.
LIBPINMAME_VERSION = 842203ca52e78a61454dc1b678afc637d7ae04d6
LIBPINMAME_SITE = $(call github,vbousquet,pinmame,$(LIBPINMAME_VERSION))
LIBPINMAME_LICENSE = BSD-3-Clause
LIBPINMAME_LICENSE_FILES = LICENSE
LIBPINMAME_DEPENDENCIES = zlib
LIBPINMAME_SUPPORTS_IN_SOURCE_BUILD = NO
LIBPINMAME_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
LIBPINMAME_CONF_OPTS += -DARCH=aarch64
else
LIBPINMAME_CONF_OPTS += -DARCH=x64
endif

LIBPINMAME_CONF_OPTS += -DPLATFORM=linux
LIBPINMAME_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBPINMAME_CONF_OPTS += -DBUILD_STATIC=OFF
LIBPINMAME_CONF_OPTS += -DARCH=$(BUILD_ARCH)

define LIBPINMAME_RENAME_CMAKE
	cp $(@D)/cmake/libpinmame/CMakeLists.txt $(@D)/CMakeLists.txt
	rm $(@D)/makefile
endef

LIBPINMAME_PRE_CONFIGURE_HOOKS += LIBPINMAME_RENAME_CMAKE

$(eval $(cmake-package))
