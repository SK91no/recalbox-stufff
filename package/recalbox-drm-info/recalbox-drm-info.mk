################################################################################
#
# recalbox-drm-info
#
################################################################################

RECALBOX_DRM_INFO_VERSION = custom
RECALBOX_DRM_INFO_SITE = $(TOPDIR)/../projects/recalbox-drm-info
RECALBOX_DRM_INFO_SITE_METHOD = local
RECALBOX_DRM_INFO_DEPENDENCIES += libdrm xlib_libX11 xlib_libXrandr
RECALBOX_DRM_INFO_LICENSE = GPL-3.0
RECALBOX_DRM_INFO_LICENSE_FILES = COPYING
RECALBOX_DRM_INFO_AUTORECONF += YES

$(eval $(autotools-package))
