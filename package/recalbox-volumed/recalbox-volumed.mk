################################################################################
#
# RECALBOX_VOLUMED
#
################################################################################

RECALBOX_VOLUMED_VERSION = custom
RECALBOX_VOLUMED_SITE = $(TOPDIR)/../projects/recalbox-volumed
RECALBOX_VOLUMED_SITE_METHOD = local
RECALBOX_VOLUMED_LICENSE = GPL-3.0
RECALBOX_VOLUMED_LICENSE_FILES = LICENSE
RECALBOX_VOLUMED_DEPENDENCIES = pulseaudio
RECALBOX_VOLUMED_AUTORECONF = YES

define RECALBOX_VOLUMED_INSTALL_INIT_CMDS
	$(INSTALL) -D -m 0755 $(RECALBOX_VOLUMED_PKGDIR)/S06volumed $(TARGET_DIR)/etc/init.d/S06volumed
	$(INSTALL) -D -m 0644 $(RECALBOX_VOLUMED_PKGDIR)/volumed.rules $(TARGET_DIR)/etc/udev/rules.d/99-volumed.rules
endef

RECALBOX_VOLUMED_POST_INSTALL_TARGET_HOOKS += RECALBOX_VOLUMED_INSTALL_INIT_CMDS

$(eval $(autotools-package))
