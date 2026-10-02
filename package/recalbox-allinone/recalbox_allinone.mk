################################################################################
#
# RECALBOX_ALLINONE
#
################################################################################

RECALBOX_ALLINONE_VERSION = custom
RECALBOX_ALLINONE_SITE = $(TOPDIR)/../projects/recalbox-allinone
RECALBOX_ALLINONE_SITE_METHOD = local
RECALBOX_ALLINONE_LICENSE = GPL
RECALBOX_ALLINONE_DEPENDENCIES = linux

define RECALBOX_ALLINONE_BUILD_CMDS
	$(HOST_DIR)/bin/linux-dtc $(@D)/allinone-overlay.dts -o $(@D)/allinone.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/allinone-auto-overlay.dts -o $(@D)/allinone-auto.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/allinone-auto-pi4-overlay.dts -o $(@D)/allinone-auto-pi4.dtbo
endef

define RECALBOX_ALLINONE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/allinone.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/allinone.dtbo
	$(INSTALL) -D -m 0644 $(@D)/allinone-auto.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/allinone-auto.dtbo
	$(INSTALL) -D -m 0644 $(@D)/allinone-auto-pi4.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/allinone-auto-pi4.dtbo
	$(INSTALL) -D -m 0744 $(RECALBOX_ALLINONE_PKGDIR)/S13allinone $(TARGET_DIR)/etc/init.d/S13allinone
	$(INSTALL) -D -m 0744 $(RECALBOX_ALLINONE_PKGDIR)/recalbox_allinone_rgb.sh $(TARGET_DIR)/recalbox/scripts/recalbox_allinone_rgb.sh
endef

$(eval $(kernel-module))
$(eval $(generic-package))
