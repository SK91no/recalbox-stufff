################################################################################
#
# GEOLITH
#
################################################################################

# Commit of 03/03/2026
LIBRETRO_GEOLITH_VERSION = b15decb86d1c52ecedefc3fb019510c11647889a
LIBRETRO_GEOLITH_SITE = $(call github,libretro,geolith-libretro,$(LIBRETRO_GEOLITH_VERSION))
LIBRETRO_GEOLITH_LICENSE = BSD 3
LIBRETRO_GEOLITH_LICENSE_FILES = LICENSE.txt

LIBRETRO_GEOLITH_DEPENDENCIES = zlib

define LIBRETRO_GEOLITH_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/libretro -f Makefile platform="$(RETROARCH_LIBRETRO_BOARD_SHORT)"
endef

define LIBRETRO_GEOLITH_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/libretro/geolith_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/geolith_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/geolith
endef

$(eval $(generic-package))
