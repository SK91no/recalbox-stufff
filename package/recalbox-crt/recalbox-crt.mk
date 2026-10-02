################################################################################
#
# recalbox crt
#
################################################################################

RECALBOX_CRT_SOURCE =
RECALBOX_CRT_VERSION = 1
RECALBOX_CRT_LICENSE = MIT

define RECALBOX_CRT_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0744 $(RECALBOX_CRT_PKGDIR)/S13crt $(TARGET_DIR)/etc/init.d/S13crt
	mkdir -p $(TARGET_DIR)/recalbox/system/configs/crt/
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/configs/crt/
	echo "Place your extra CRT configurations in this directory." > $(TARGET_DIR)/recalbox/share_init/system/configs/crt/README.txt
	$(INSTALL) -D -m 644 $(RECALBOX_CRT_PKGDIR)/crt-config/modes.txt $(TARGET_DIR)/recalbox/system/configs/crt/modes.txt
	$(INSTALL) -D -m 644 $(RECALBOX_CRT_PKGDIR)/crt-config/systems.txt $(TARGET_DIR)/recalbox/system/configs/crt/systems.txt
	$(INSTALL) -D -m 644 $(RECALBOX_CRT_PKGDIR)/crt-config/arcade_games.txt $(TARGET_DIR)/recalbox/system/configs/crt/arcade_games.txt
	$(INSTALL) -D -m 644 $(RECALBOX_CRT_PKGDIR)/crt-config/arcade_games_v2.txt $(TARGET_DIR)/recalbox/system/configs/crt/arcade_games_v2.txt
endef

$(eval $(generic-package))
