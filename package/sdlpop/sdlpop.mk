################################################################################
#
# SDLPoP
#
################################################################################

# Commit of 2024/05/11
SDLPOP_VERSION = be89e134e6f19ca5a9d4442b1ecfa2ccf2b3c6e2
SDLPOP_SITE = $(call github,NagyD,SDLPoP,$(SDLPOP_VERSION))
SDLPOP_SUBDIR = src
SDLPOP_LICENSE = GPL3.0
SDLPOP_LICENSE_FILES = COPYING
SDLPOP_DEPENDENCIES = sdl2 sdl2_image

define SDLPOP_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/configs/sdlpop
	mkdir -p "$(TARGET_DIR)/recalbox/share_init/roms/ports/Prince of Persia"
	$(INSTALL) -m 0755 $(@D)/prince -D $(TARGET_DIR)/usr/bin
	cp -r $(@D)/SDLPoP.ini $(TARGET_DIR)/recalbox/share_init/system/configs/sdlpop/sdlpop.ini
	cp -r $(@D)/data "$(TARGET_DIR)/recalbox/share_init/roms/ports/Prince of Persia"
endef

$(eval $(cmake-package))
