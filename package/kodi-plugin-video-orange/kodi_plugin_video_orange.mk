################################################################################
#
# kodi orange pvr
#
################################################################################

KODI_PLUGIN_VIDEO_ORANGE_RELEASE = omega
KODI_PLUGIN_VIDEO_ORANGE_VERSION = 2.3.6
KODI_PLUGIN_VIDEO_ORANGE_PLUGIN_NAME = plugin.video.orange.fr
KODI_PLUGIN_VIDEO_ORANGE_SOURCE = $(KODI_PLUGIN_VIDEO_ORANGE_PLUGIN_NAME)-$(KODI_PLUGIN_VIDEO_ORANGE_VERSION).zip
KODI_PLUGIN_VIDEO_ORANGE_SITE = https://mirrors.kodi.tv/addons/$(KODI_PLUGIN_VIDEO_ORANGE_RELEASE)/$(KODI_PLUGIN_VIDEO_ORANGE_PLUGIN_NAME)
KODI_PLUGIN_VIDEO_ORANGE_LICENSE = LICENSE

KODI_PLUGIN_VIDEO_ORANGE_TARGET_DIR=$(TARGET_DIR)/usr/share/kodi/addons

define KODI_PLUGIN_VIDEO_ORANGE_EXTRACT_CMDS
	@unzip -q -o $(DL_DIR)/kodi-plugin-video-orange/$(KODI_PLUGIN_VIDEO_ORANGE_SOURCE) -d $(@D)
endef

define KODI_PLUGIN_VIDEO_ORANGE_INSTALL_TARGET_CMDS
	@mkdir -p $(KODI_PLUGIN_VIDEO_ORANGE_TARGET_DIR)
	@cp -r $(@D)/$(KODI_PLUGIN_VIDEO_ORANGE_PLUGIN_NAME) $(KODI_PLUGIN_VIDEO_ORANGE_TARGET_DIR)/
endef

$(eval $(generic-package))
