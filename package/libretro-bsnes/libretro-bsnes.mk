################################################################################
#
# BSNES
#
################################################################################

# Commit of 2023/10/13
LIBRETRO_BSNES_VERSION = 6e73d33127a1e2e4b5e49f4aa4f0619364e473c2
LIBRETRO_BSNES_SITE = $(call github,libretro,bsnes-libretro,$(LIBRETRO_BSNES_VERSION))
LIBRETRO_BSNES_LICENSE = COPYRIGHT
LIBRETRO_BSNES_LICENSE_FILES = LICENSE
LIBRETRO_BSNES_NON_COMMERCIAL = y

define LIBRETRO_BSNES_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_BSNES_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/bsnes_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/bsnes_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/satellaview
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/sgb
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/sufami
endef

$(eval $(generic-package))
