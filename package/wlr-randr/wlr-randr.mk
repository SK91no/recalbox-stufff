################################################################################
#
# wlr-randr
#
################################################################################

WLR_RANDR_VERSION = v0.2.0
WLR_RANDR_SITE = $(call github,emersion,wlr-randr,$(WLR_RANDR_VERSION))
WLR_RANDR_LICENSE = MIT
WLR_RANDR_LICENSE_FILES = LICENSE
WLR_RANDR_DEPENDENCIES = wayland wayland-protocols dwl

$(eval $(meson-package))
