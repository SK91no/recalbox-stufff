################################################################################
#
# MAME2003
#
################################################################################

# Commit of 2023/06/22
LIBRETRO_MAME2003_VERSION = 105ca02fb85e92b9dd5d6ee43f7152d1199eb149
LIBRETRO_MAME2003_SITE = $(call github,libretro,mame2003-libretro,$(LIBRETRO_MAME2003_VERSION))
LIBRETRO_MAME2003_LICENSE = MAME
LIBRETRO_MAME2003_NON_COMMERCIAL = y

LIBRETRO_MAME2003_DEPENDENCIES = arcade-dats

define LIBRETRO_MAME2003_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	mkdir -p $(@D)/obj/mame/cpu/ccpu
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="$(RETROARCH_LIBRETRO_BOARD)"
endef

define LIBRETRO_MAME2003_INSTALL_TARGET_CMDS
	$(call InstallArcadeFiles,libretro,mame2003,$(LIBRETRO_MAME2003_VERSION))
	$(INSTALL) -D $(@D)/mame2003_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mame2003_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/mame2003/samples
	cp -R $(@D)/metadata/* $(TARGET_DIR)/recalbox/share_upgrade/bios/mame2003
	rm $(TARGET_DIR)/recalbox/share_upgrade/bios/mame2003/mame2003.xml
endef

$(eval $(generic-package))
