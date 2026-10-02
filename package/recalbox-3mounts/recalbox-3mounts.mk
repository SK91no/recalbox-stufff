################################################################################
#
# Recalbox 3mounts package
#
################################################################################

RECALBOX_3MOUNTS_VERSION = custom
RECALBOX_3MOUNTS_SITE = $(TOPDIR)/../projects/recalbox-3mounts
RECALBOX_3MOUNTS_SITE_METHOD = local
RECALBOX_3MOUNTS_LICENSE = MIT
RECALBOX_3MOUNTS_DEPENDENCIES = udev lockfile-progs

define RECALBOX_3MOUNTS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/3mounts $(TARGET_DIR)/usr/bin/3mounts
	$(INSTALL) -D -m 0644 $(@D)/3mounts.conf $(TARGET_DIR)/etc/3mounts/3mounts.conf
	$(INSTALL) -D -m 0644 $(@D)/90-3mounts.rules $(TARGET_DIR)/lib/udev/rules.d/90-3mounts.rules
	$(INSTALL) -D -m 0755 $(@D)/recalbox-3mounts.sh $(TARGET_DIR)/recalbox/scripts/recalbox-3mounts.sh
endef

$(eval $(generic-package))
