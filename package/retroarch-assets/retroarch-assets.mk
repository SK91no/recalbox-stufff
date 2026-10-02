################################################################################
#
# RETROARCH ASSETS
#
################################################################################

RETROARCH_ASSETS_VERSION = ded629fd9ba2d9da5e379c9c526be79801824c26
RETROARCH_ASSETS_SITE = $(call github,libretro,retroarch-assets,$(RETROARCH_ASSETS_VERSION))
RETROARCH_ASSETS_LICENSE = GPL
RETROARCH_ASSETS_DEPENDENCIES = retroarch

define RETROARCH_ASSETS_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/share/retroarch/assets/xmb
	cp -r $(@D)/ozone $(TARGET_DIR)/usr/share/retroarch/assets
	cp -r $(@D)/rgui $(TARGET_DIR)/usr/share/retroarch/assets
	cp -r $(@D)/glui $(TARGET_DIR)/usr/share/retroarch/assets
	cp -r $(@D)/xmb/monochrome $(TARGET_DIR)/usr/share/retroarch/assets/xmb
	cp -r $(@D)/menu_widgets $(TARGET_DIR)/usr/share/retroarch/assets
	cp -r $(@D)/sounds $(TARGET_DIR)/usr/share/retroarch/assets
endef

$(eval $(generic-package))
