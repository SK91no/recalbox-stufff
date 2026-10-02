################################################################################
#
# BOOM3
#
################################################################################

# Commit of 2022/08/10
LIBRETRO_BOOM3_VERSION = 0bea79abf5ec8262dfe9af73cb8c54ea6e2aeb98
LIBRETRO_BOOM3_SITE = $(call github,libretro,boom3,$(LIBRETRO_BOOM3_VERSION))
LIBRETRO_BOOM3_LICENSE = GPL-3.0
LIBRETRO_BOOM3_LICENSE_FILES = COPYING.txt
LIBRETRO_BOOM3_DEPENDENCIES = zlib libgl retroarch

define LIBRETRO_BOOM3_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_NOLTO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_NOLTO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_NOLTO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/neo -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_BOOM3_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/neo/boom3_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/boom3_libretro.so
	mkdir -p "$(TARGET_DIR)/recalbox/share_upgrade/roms/ports/Doom 3"
	cp -R $(@D)/base "$(TARGET_DIR)/recalbox/share_upgrade/roms/ports/Doom 3"
endef

$(eval $(generic-package))
