################################################################################
#
# BSNES_HD
#
################################################################################

# Commit of 2023/04/27
LIBRETRO_BSNES_HD_VERSION = f46b6d6368ea93943a30b5d4e79e8ed51c2da5e8
LIBRETRO_BSNES_HD_SITE = $(call github,DerKoun,bsnes-hd,$(LIBRETRO_BSNES_HD_VERSION))
LIBRETRO_BSNES_HD_LICENSE = COPYRIGHT
LIBRETRO_BSNES_HD_LICENSE_FILES = LICENSE
LIBRETRO_BSNES_HD_NON_COMMERCIAL = y

define LIBRETRO_BSNES_HD_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_BSNES_HD_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/bsnes_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/bsneshd_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/satellaview
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/sgb
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/sufami
endef

$(eval $(generic-package))
