################################################################################
#
# XONE
#
################################################################################

XONE_VERSION = 29033e16226bea4458b53fffc6177b95f6907f26
XONE_SITE = $(call github,dlundqvist,xone,$(XONE_VERSION))
XONE_LICENSE = GPL-2.0

define XONE_COPY_CMDS
	$(INSTALL) -D -m 0644 $(@D)/install/modprobe.conf $(TARGET_DIR)/etc/modprobe.d/xone-blacklist.conf
	$(INSTALL) -D -m 0644 $(XONE_PKGDIR)/assets/FW_ACC_00U.bin $(TARGET_DIR)/lib/firmware/xow_dongle.bin
endef

XONE_INSTALL_TARGET_CMDS += $(XONE_COPY_CMDS)

$(eval $(kernel-module))
$(eval $(generic-package))
