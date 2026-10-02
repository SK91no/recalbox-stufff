################################################################################
#
# XPAD_NOONE
#
################################################################################

# Commit of 2024/01/09
XPAD_NOONE_VERSION = 96d119aabacb84d39ed9c95ae48cc4891496ccb4
XPAD_NOONE_SITE = $(call github,medusalix,xpad-noone,$(XPAD_NOONE_VERSION))
XPAD_NOONE_LICENSE = GPL-2.0

$(eval $(kernel-module))
$(eval $(generic-package))
