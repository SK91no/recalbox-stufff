################################################################################
#
# PD777 (EPOCH CASSETTE VISION)
#
################################################################################

#Commit version of 04/08/2026
LIBRETRO_PD777_VERSION = 4c37ba011c9097cd76b0c1ce85cf1afcb2d377f6
LIBRETRO_PD777_SITE = $(call github,mittonk,pd777,$(LIBRETRO_PD777_VERSION))
LIBRETRO_PD777_LICENSE = MIT
LIBRETRO_PD777_LICENSE_FILES = licence.txt
LIBRETRO_PD777_DEPENDENCIES = zlib

define LIBRETRO_PD777_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/source/libretro/Makefile.libretro
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/source/libretro -f Makefile.libretro platform="$(RETROARCH_LIBRETRO_BOARD)"
		GIT_VERSION="-$(shell echo $(LIBRETRO_PD777_VERSION) | cut -c 1-8)"
endef

define LIBRETRO_PD777_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/source/libretro/pd777_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/pd777_libretro.so
endef

$(eval $(generic-package))
