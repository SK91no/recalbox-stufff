################################################################################
#
# libframeutil
#
################################################################################

# Commit of 2026/01/22
LIBFRAMEUTIL_VERSION = 03d2483d5cded0bdef84bec24c9ddfdede324b5c
LIBFRAMEUTIL_SITE = $(call github,PPUC,libframeutil,$(LIBFRAMEUTIL_VERSION))
LIBFRAMEUTIL_LICENSE = GPL-3.0
LIBFRAMEUTIL_LICENSE_FILES = LICENSE
LIBFRAMEUTIL_INSTALL_STAGING = YES

define LIBFRAMEUTIL_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/include/FrameUtil.h \
		$(STAGING_DIR)/usr/include/FrameUtil.h
endef

$(eval $(generic-package))
