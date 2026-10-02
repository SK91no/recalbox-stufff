################################################################################
#
# RECALBOX_INPUT_LAG_TOOLS
#
################################################################################

RECALBOX_INPUT_LAG_TOOLS_VERSION = custom
RECALBOX_INPUT_LAG_TOOLS_SITE = $(TOPDIR)/../projects/recalbox-input-lag-tools
RECALBOX_INPUT_LAG_TOOLS_SITE_METHOD = local
RECALBOX_INPUT_LAG_TOOLS_LICENSE = GPL
RECALBOX_INPUT_LAG_TOOLS_DEPENDENCIES = libgpiod2

$(eval $(cmake-package))
