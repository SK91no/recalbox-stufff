################################################################################
#
# kodi freebox plugin
#
################################################################################

KODI_PLUGIN_VIDEO_FREEBOX_VERSION = 21.0.0-Omega
KODI_PLUGIN_VIDEO_FREEBOX_SITE = $(call github,aassif,pvr.freebox,$(KODI_PLUGIN_VIDEO_FREEBOX_VERSION))
KODI_PLUGIN_VIDEO_FREEBOX_DEPENDENCIES = kodi json-for-modern-cpp
KODI_PLUGIN_VIDEO_FREEBOX_LICENSE = MIT
KODI_PLUGIN_VIDEO_FREEBOX_LICENSE_FILES = $(KODI_PLUGIN_VIDEO_FREEBOX_PLUGINNAME)/LICENSE

$(eval $(cmake-package))
