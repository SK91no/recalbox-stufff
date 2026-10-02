################################################################################
#
# DICE
#
################################################################################

# Commit of 2025/04/20
LIBRETRO_DICE_VERSION = beb422a68dbb7f78d73b1fd6cd0807288a6613c6
LIBRETRO_DICE_SITE = $(call github,mittonk,dice-libretro,$(LIBRETRO_DICE_VERSION))
LIBRETRO_DICE_LICENSE = GPL-3.0

LIBRETRO_DICE_DEPENDENCIES = arcade-dats

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI3)$(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2LEGACY)$(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2),y)
LIBRETRO_DICE_PLATFORM=rpi3
else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI1),y)
LIBRETRO_DICE_PLATFORM=rpi1
else
LIBRETRO_DICE_PLATFORM=$(RETROARCH_LIBRETRO_BOARD)
endif

define LIBRETRO_DICE_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="$(LIBRETRO_DICE_PLATFORM)"
endef

define LIBRETRO_DICE_INSTALL_TARGET_CMDS
	$(call InstallArcadeFiles,libretro,dice,$(LIBRETRO_DICE_VERSION))
	$(INSTALL) -D $(@D)/dice_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/dice_libretro.so
endef

$(eval $(generic-package))
