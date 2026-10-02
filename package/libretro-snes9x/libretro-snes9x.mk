################################################################################
#
# SNES9X
#
################################################################################

# Commit of 2024/08/20
LIBRETRO_SNES9X_VERSION = 229933ea5aa6b4d2a6f00ed55b34d7b8bf5f63e3
LIBRETRO_SNES9X_SITE = $(call github,libretro,snes9x,$(LIBRETRO_SNES9X_VERSION))
LIBRETRO_SNES9X_LICENSE = COPYRIGHT
LIBRETRO_SNES9X_LICENSE_FILES = LICENSE
LIBRETRO_SNES9X_NON_COMMERCIAL = y
LIBRETRO_SNES9X_DEPENDENCIES = zlib

define LIBRETRO_SNES9X_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) -std=c++11" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/libretro -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_SNES9X_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/libretro/snes9x_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/snes9x_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/satellaview
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/sufami
endef

$(eval $(generic-package))
