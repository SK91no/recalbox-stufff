################################################################################
#
# recalbox-hardware
#
################################################################################

RECALBOX_HARDWARE_VERSION = custom
RECALBOX_HARDWARE_SITE = $(TOPDIR)/../projects/recalbox-hardware
RECALBOX_HARDWARE_SITE_METHOD = local
RECALBOX_HARDWARE_LICENSE = MIT
RECALBOX_HARDWARE_LICENSE_FILES = LICENSE.md
RECALBOX_HARDWARE_DEPENDENCIES = recalbox-themes host-python3 linux
RECALBOX_HARDWARE_INSTALL_IMAGES = YES

define RECALBOX_HARDWARE_BUILD_CMDS_PI
	$(HOST_DIR)/bin/linux-dtc $(@D)/case/installers/retroflags/assets/overlays/retroflag-case-overlay.dts -o $(@D)/retroflag-case.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/case/installers/gpi/assets/overlays/recalbox-gpicase1-overlay.dts -o $(@D)/recalbox-gpicase1.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/case/installers/gpi2w/assets/overlays/recalbox-gpicase2w-overlay.dts -o $(@D)/recalbox-gpicase2w.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalbox-official-hardware/i2ccustom-pi5-overlay.dts -o $(@D)/recalbox-official-hardware/i2ccustom-pi5.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalbox-official-hardware/i2ccustom-overlay.dts -o $(@D)/recalbox-official-hardware/i2ccustom.dtbo
endef

define RECALBOX_HARDWARE_INSTALL_IMAGE_CMDS_PI
	$(INSTALL) -D -m 0644 $(@D)/retroflag-case.dtbo $(BINARIES_DIR)/boot-data/overlays/retroflag-case.dtbo
	$(INSTALL) -D -m 0644 $(@D)/recalbox-gpicase1.dtbo $(BINARIES_DIR)/boot-data/overlays/recalbox-gpicase1.dtbo
	$(INSTALL) -D -m 0644 $(@D)/recalbox-gpicase2w.dtbo $(BINARIES_DIR)/boot-data/overlays/recalbox-gpicase2w.dtbo
	$(INSTALL) -D -m 0644 $(@D)/recalbox-official-hardware/i2ccustom-pi5.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/i2ccustom-pi5.dtbo
	$(INSTALL) -D -m 0644 $(@D)/recalbox-official-hardware/i2ccustom.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/i2ccustom.dtbo
	$(INSTALL) -D -m 0744 $(@D)/recalbox-official-hardware/S03recalbox-official-harware-detector $(TARGET_DIR)/etc/init.d/S03recalbox-official-harware-detector
endef

ifeq ($(BR2_PACKAGE_RECALBOX_RPI_ANY),y)
RECALBOX_HARDWARE_POST_BUILD_HOOKS += RECALBOX_HARDWARE_BUILD_CMDS_PI
RECALBOX_HARDWARE_POST_INSTALL_IMAGES_HOOKS += RECALBOX_HARDWARE_INSTALL_IMAGE_CMDS_PI
endif

define RECALBOX_HARDWARE_BUILD_CMDS
	PYTHONPATH="$(PYTHON3_PATH)" $(HOST_DIR)/bin/python$(PYTHON3_VERSION_MAJOR) -m compileall $(@D)
endef

define RECALBOX_HARDWARE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/system/hardware
	cp -r $(@D)/* $(TARGET_DIR)/recalbox/system/hardware/
endef

$(eval $(generic-package))
