################################################################################
#
# OPENLARA
#
################################################################################

# Commit of 2025/02/15
OPENLARA_VERSION = 46a753bbee487bfdda2e31b989fe9b979ce473fa
OPENLARA_SITE = $(call github,XProger,OpenLara,$(OPENLARA_VERSION))
OPENLARA_DEPENDENCIES = sdl2 sdl2_image sdl2_mixer sdl2_net
OPENLARA_LICENSE = BSD-2-Clause
OPENLARA_LICENSE_FILES = LICENSE

ifeq ($(BR2_x86_64),y)
OPENLARA_BACKEND = gl
else
OPENLARA_BACKEND = gles3
endif

define OPENLARA_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/src/platform/sdl2 -f \
		Makefile BACKEND=$(OPENLARA_BACKEND)
endef

define OPENLARA_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/configs/openlara
	mkdir -p "$(TARGET_DIR)/recalbox/share_init/roms/ports/Tomb Raider"
	mkdir -p $(TARGET_DIR)/recalbox/share_init/saves/tombraider
	$(INSTALL) -D -m 0755 $(@D)/src/platform/sdl2/openlara $(TARGET_DIR)/usr/bin/openlara
endef

$(eval $(generic-package))
