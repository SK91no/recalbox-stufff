################################################################################
#
# RECALBOX_CARD_READER
#
################################################################################

RECALBOX_CARD_READER_VERSION = custom
RECALBOX_CARD_READER_SITE = $(TOPDIR)/../projects/recalbox-card-reader
RECALBOX_CARD_READER_SITE_METHOD = local
RECALBOX_CARD_READER_LICENSE = GPL
RECALBOX_CARD_READER_DEPENDENCIES = linux

define RECALBOX_CARD_READER_BUILD_CMDS
	$(TARGET_CPP) -nostdinc -undef -x assembler-with-cpp -I$(LINUX_DIR)/include/ $(@D)/recalbox-card-reader-overlay.dts $(@D)/recalbox-card-reader-overlay.dtspp
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalbox-card-reader-overlay.dtspp -o $(@D)/recalbox-card-reader.dtbo
endef

define RECALBOX_CARD_READER_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/recalbox-card-reader.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalbox-card-reader.dtbo
endef

$(eval $(kernel-module))
$(eval $(generic-package))
