################################################################################
#
# RECALBOX_RGB_JAMMA_2
#
################################################################################

RECALBOX_RGB_JAMMA_2_VERSION = custom
RECALBOX_RGB_JAMMA_2_SITE = $(TOPDIR)/../projects/recalbox-rgb-jamma-2
RECALBOX_RGB_JAMMA_2_SITE_METHOD = local
RECALBOX_RGB_JAMMA_2_LICENSE = GPL
RECALBOX_RGB_JAMMA_2_DEPENDENCIES = linux recalbox-rgb-dual recalbox-rgb-jamma

define RECALBOX_RGB_JAMMA_2_BUILD_CMDS
	$(TARGET_CPP) -nostdinc -undef -x assembler-with-cpp -I$(LINUX_DIR)/include/ $(@D)/recalboxrgbjamma2-mcp-overlay.dts $(@D)/recalboxrgbjamma2-mcp-overlay.dtspp
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalboxrgbjamma2-mcp-overlay.dtspp -o $(@D)/recalboxrgbjamma2-mcp.dtbo
endef

define RECALBOX_RGB_JAMMA_2_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/recalboxrgbjamma2-mcp.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalboxrgbjamma2-mcp.dtbo
endef

$(eval $(kernel-module))
$(eval $(generic-package))
