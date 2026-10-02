################################################################################
#
# recalbox romfs 2
#
################################################################################

RECALBOX_ROMFS2_LICENSE = MIT
RECALBOX_ROMFS2_DEPENDENCIES = host-python3

define RECALBOX_ROMFS2_BUILD_CMDS
	$(HOST_DIR)/bin/python3 $(RECALBOX_ROMFS2_PKGDIR)/scripts/romfs.py \
		-systems $(RECALBOX_ROMFS2_PKGDIR)/systems -buildsystems $(@D)/systemlist.xml
endef

ifeq ($(BR2_PACKAGE_RECALBOX_LITE),y)
RECALBOX_ROMFS2_OPTS += -lite
endif

define RECALBOX_ROMFS2_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/.emulationstation
	cp $(@D)/systemlist.xml $(TARGET_DIR)/recalbox/share_init/system/.emulationstation/
	$(HOST_DIR)/bin/python3 $(RECALBOX_ROMFS2_PKGDIR)/scripts/romfs.py \
		-systems $(RECALBOX_ROMFS2_PKGDIR)/systems -installroms $(TARGET_DIR)/recalbox/ $(RECALBOX_ROMFS2_OPTS)
endef

$(eval $(generic-package))
