################################################################################
#
# BR2_PACKAGE_RECALBOX_GAMEPAD_PROXY
#
################################################################################

RECALBOX_GAMEPAD_PROXY_VERSION = custom
RECALBOX_GAMEPAD_PROXY_SITE = $(TOPDIR)/../projects/recalbox-gamepad-proxy
RECALBOX_GAMEPAD_PROXY_SITE_METHOD = local
RECALBOX_GAMEPAD_PROXY_LICENSE = GPL
RECALBOX_GAMEPAD_PROXY_DEPENDENCIES = linux

define RECALBOX_GAMEPAD_PROXY_BUILD_CMDS
	$(MAKE) -C $(@D)/test CC="$(HOSTCC)" run
	$(TARGET_CPP) -nostdinc -undef -x assembler-with-cpp -I$(LINUX_DIR)/include/ $(@D)/recalbox-gamepad-proxy-overlay.dts $(@D)/recalbox-gamepad-proxy-overlay.dtspp
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalbox-gamepad-proxy-overlay.dtspp -o $(@D)/recalbox-gamepad-proxy.dtbo
endef

define RECALBOX_GAMEPAD_PROXY_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/recalbox-gamepad-proxy.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalbox-gamepad-proxy.dtbo
endef

$(eval $(kernel-module))
$(eval $(generic-package))
