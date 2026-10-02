################################################################################
#
# RECALBOX_JVS
#
################################################################################

RECALBOX_JVS_VERSION = 2.x
RECALBOX_JVS_SITE = $(TOPDIR)/../projects/recalbox-jvs
RECALBOX_JVS_SITE_METHOD = local
RECALBOX_JVS_LICENSE = GPL
RECALBOX_JVS_DEPENDENCIES = python-esptool

define RECALBOX_JVS_BUILD_CMDS
	$(TARGET_CPP) -nostdinc -undef -x assembler-with-cpp -I$(LINUX_DIR)/include/ $(@D)/recalboxjvs-overlay.dts $(@D)/recalboxjvs-overlay.dtspp
	$(HOST_DIR)/bin/linux-dtc -@ $(@D)/recalboxjvs-overlay.dtspp -o $(@D)/recalboxjvs.dtbo
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -O2 -o $(@D)/recalboxjvs-outputsd $(@D)/recalboxjvs-outputsd.c
endef

define RECALBOX_JVS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0744 $(RECALBOX_JVS_PKGDIR)/S05jvs-screentype $(TARGET_DIR)/etc/init.d/S05jvs-screentype
	$(INSTALL) -D -m 0744 $(RECALBOX_JVS_PKGDIR)/S06jvs-outputsd   $(TARGET_DIR)/etc/init.d/S06jvs-outputsd
	$(INSTALL) -D -m 0744 $(RECALBOX_JVS_PKGDIR)/S14jvs            $(TARGET_DIR)/etc/init.d/S14jvs
	$(INSTALL) -D -m 0755 $(@D)/recalboxjvs-outputsd               $(TARGET_DIR)/usr/bin/recalboxjvs-outputsd
	$(INSTALL) -d -m 0755 $(TARGET_DIR)/recalbox/system/jvs/
	for f in $(@D)/recalboxjvs*.bin; do \
		$(INSTALL) -m 0744 "$$f" $(TARGET_DIR)/recalbox/system/jvs/; \
	done
	$(INSTALL) -D -m 0644 $(@D)/recalboxjvs.dtbo $(BINARIES_DIR)/boot-data/overlays/recalboxjvs.dtbo
endef

$(eval $(kernel-module))
$(eval $(generic-package))
