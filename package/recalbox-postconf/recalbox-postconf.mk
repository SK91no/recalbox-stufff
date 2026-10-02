################################################################################
#
# recalbox-postconf
#
################################################################################

RECALBOX_POSTCONF_VERSION = custom
RECALBOX_POSTCONF_SOURCE =

RECALBOX_POSTCONF_LICENSE = GPLv2
RECALBOX_POSTCONF_DEPENDENCIES = python3 python-pyyaml

define RECALBOX_POSTCONF_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(RECALBOX_POSTCONF_PKGDIR)/recalbox-postconfig $(TARGET_DIR)/recalbox/scripts/recalbox-postconfig
endef

$(eval $(generic-package))
