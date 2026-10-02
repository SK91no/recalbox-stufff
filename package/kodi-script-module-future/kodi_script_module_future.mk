################################################################################
#
# kodi future script
#
################################################################################

KODI_SCRIPT_MODULE_FUTURE_RELEASE = omega
KODI_SCRIPT_MODULE_FUTURE_VERSION = 1.0.0+matrix.1
KODI_SCRIPT_MODULE_FUTURE_PLUGIN_NAME = script.module.future
KODI_SCRIPT_MODULE_FUTURE_SOURCE = $(KODI_SCRIPT_MODULE_FUTURE_PLUGIN_NAME)-$(KODI_SCRIPT_MODULE_FUTURE_VERSION).zip
KODI_SCRIPT_MODULE_FUTURE_SITE = https://mirrors.kodi.tv/addons/$(KODI_SCRIPT_MODULE_FUTURE_RELEASE)/$(KODI_SCRIPT_MODULE_FUTURE_PLUGIN_NAME)
KODI_SCRIPT_MODULE_FUTURE_LICENSE = COPYRIGHT

KODI_SCRIPT_MODULE_FUTURE_TARGET_DIR=$(TARGET_DIR)/usr/share/kodi/addons

define KODI_SCRIPT_MODULE_FUTURE_EXTRACT_CMDS
	@unzip -q -o $(DL_DIR)/kodi-script-module-future/$(KODI_SCRIPT_MODULE_FUTURE_SOURCE) -d $(@D)
endef

define KODI_SCRIPT_MODULE_FUTURE_INSTALL_TARGET_CMDS
	@mkdir -p $(KODI_SCRIPT_MODULE_FUTURE_TARGET_DIR)
	@cp -r $(@D)/$(KODI_SCRIPT_MODULE_FUTURE_PLUGIN_NAME) $(KODI_SCRIPT_MODULE_FUTURE_TARGET_DIR)/
endef

$(eval $(generic-package))
