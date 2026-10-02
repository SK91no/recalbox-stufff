################################################################################
#
# M2000 (PHILIPS P2000T)
#
################################################################################

#Commit version 03/31/2026
LIBRETRO_M2000_VERSION = 60e12fe9ee07f024b5a0d569ddf6ad8efbffcd4b
LIBRETRO_M2000_SITE = $(call github,p2000t,M2000,$(LIBRETRO_M2000_VERSION))
LIBRETRO_M2000_LICENSE = GPL-3.0
LIBRETRO_M2000_LICENSE_FILES = LICENSE

define LIBRETRO_M2000_BUILD_CMDS
		$(SED) "s|-O2|-O3|g" $(@D)/src/libretro/Makefile
				CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
				CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
				LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
				$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/src/libretro -f Makefile platform="unix"
				GIT_VERSION="-$(shell echo $(LIBRETRO_M2000_VERSION) | cut -c 1-8)"
endef

define LIBRETRO_M2000_INSTALL_TARGET_CMDS
		$(INSTALL) -D $(@D)/src/libretro/m2000_libretro.so \
				$(TARGET_DIR)/usr/lib/libretro/m2000_libretro.so
endef

$(eval $(generic-package))
