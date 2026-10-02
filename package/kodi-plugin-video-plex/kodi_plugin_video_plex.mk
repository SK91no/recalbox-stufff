################################################################################
#
# kodi plex plugin
#
################################################################################

#https://github.com/pannal/plex-for-kodi/archive/refs/tags/v0.7.9-rev4.tar.gz
KODI_PLUGIN_VIDEO_PLEX_VERSION = 0.7.9-rev4
KODI_PLUGIN_VIDEO_PLEX_PLUGINNAME = plex-for-kodi
KODI_PLUGIN_VIDEO_PLEX_SOURCE = $(KODI_PLUGIN_VIDEO_PLEX_PLUGINNAME)-$(KODI_PLUGIN_VIDEO_PLEX_VERSION).zip
KODI_PLUGIN_VIDEO_PLEX_SITE = https://github.com/pannal/$(KODI_PLUGIN_VIDEO_PLEX_PLUGINNAME)/archive/refs/tags/v$(KODI_PLUGIN_VIDEO_PLEX_VERSION)
KODI_PLUGIN_VIDEO_PLEX_DEPENDENCIES = kodi-script-module-kodi-six
KODI_PLUGIN_VIDEO_PLEX_LICENSE = GPL-2.0
KODI_PLUGIN_VIDEO_PLEX_LICENSE_FILES = $(KODI_PLUGIN_VIDEO_PLEX_PLUGINNAME)/README.md

KODI_PLUGIN_VIDEO_PLEX_TARGET_DIR=$(TARGET_DIR)/usr/share/kodi/addons

define KODI_PLUGIN_VIDEO_PLEX_EXTRACT_CMDS
	@unzip -q -o $(DL_DIR)/kodi-plugin-video-plex/$(KODI_PLUGIN_VIDEO_PLEX_SOURCE) -d $(@D)
endef

define KODI_PLUGIN_VIDEO_PLEX_INSTALL_TARGET_CMDS
	@mkdir -p $(KODI_PLUGIN_VIDEO_PLEX_TARGET_DIR)
	@cp -r $(@D)/$(KODI_PLUGIN_VIDEO_PLEX_PLUGINNAME)-$(KODI_PLUGIN_VIDEO_PLEX_VERSION) $(KODI_PLUGIN_VIDEO_PLEX_TARGET_DIR)
	@cp $(KODI_PLUGIN_VIDEO_PLEX_PKGDIR)/templates/* $(KODI_PLUGIN_VIDEO_PLEX_TARGET_DIR)/plex-for-kodi-$(KODI_PLUGIN_VIDEO_PLEX_VERSION)/resources/skins/Main/1080i/
endef

$(eval $(generic-package))
