################################################################################
#
# VVVVVV
#
################################################################################

#Commit of 28/09/2024
VVVVVV_VERSION = a0bd2f3da42da2f5249e2a4892e2b48b51b2fd86
VVVVVV_SITE = https://github.com/TerryCavanagh/VVVVVV.git
VVVVVV_LICENSE = VVVVVV Source Code License v1.0
VVVVVV_SUBDIR = desktop_version
VVVVVV_DEPENDENCIES = sdl2
VVVVVV_GIT_SUBMODULES = YES
VVVVVV_SITE_METHOD = git

define VVVVVV_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/bin/vvvvvv/
	cp -r $(@D)/desktop_version/fonts $(TARGET_DIR)/usr/bin/vvvvvv/
	cp -r $(@D)/desktop_version/lang $(TARGET_DIR)/usr/bin/vvvvvv/
	$(INSTALL) -D -m 0755 $(@D)/desktop_version/VVVVVV $(TARGET_DIR)/usr/bin/vvvvvv/VVVVVV
endef

$(eval $(cmake-package))
