################################################################################
#
# kodi kodi-six script
#
################################################################################

KODI_SCRIPT_MODULE_KODI_SIX_RELEASE = omega
KODI_SCRIPT_MODULE_KODI_SIX_VERSION = 0.1.3.1
KODI_SCRIPT_MODULE_KODI_SIX_PLUGIN_NAME = script.module.kodi-six
KODI_SCRIPT_MODULE_KODI_SIX_SOURCE = $(KODI_SCRIPT_MODULE_KODI_SIX_PLUGIN_NAME)-$(KODI_SCRIPT_MODULE_KODI_SIX_VERSION).zip
KODI_SCRIPT_MODULE_KODI_SIX_SITE = https://mirrors.kodi.tv/addons/$(KODI_SCRIPT_MODULE_KODI_SIX_RELEASE)/$(KODI_SCRIPT_MODULE_KODI_SIX_PLUGIN_NAME)
KODI_SCRIPT_MODULE_KODI_SIX_LICENSE = LICENSE

KODI_SCRIPT_MODULE_KODI_SIX_TARGET_DIR=$(TARGET_DIR)/usr/share/kodi/addons

define KODI_SCRIPT_MODULE_KODI_SIX_EXTRACT_CMDS
	@unzip -q -o $(DL_DIR)/kodi-script-module-kodi-six/$(KODI_SCRIPT_MODULE_KODI_SIX_SOURCE) -d $(@D)
endef

define KODI_SCRIPT_MODULE_KODI_SIX_INSTALL_TARGET_CMDS
	@mkdir -p $(KODI_SCRIPT_MODULE_KODI_SIX_TARGET_DIR)
	@cp -r $(@D)/$(KODI_SCRIPT_MODULE_KODI_SIX_PLUGIN_NAME) $(KODI_SCRIPT_MODULE_KODI_SIX_TARGET_DIR)/
endef

$(eval $(generic-package))
