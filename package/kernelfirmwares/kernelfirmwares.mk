################################################################################
#
# kernelfirmwares
#
################################################################################

KERNELFIRMWARES_VERSION = 20250311
KERNELFIRMWARES_SOURCE = linux-firmware-$(KERNELFIRMWARES_VERSION).tar.gz
KERNELFIRMWARES_SITE = https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git
KERNELFIRMWARES_SITE_METHOD = git
KERNELFIRMWARES_LICENSE = MULTIPLE
KERNELFIRMWARES_NON_COMMERCIAL = y

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI3)$(BR2_PACKAGE_RECALBOX_TARGET_RPI4)$(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64)$(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
define KERNELFIRMWARES_REMOVE_BRCM_FOLDER_CMDS
	$(RM) -rf $(@D)/brcm/ $(@D)/nvidia/ $(@D)/amd* $(@D)/radeon $(@D)/i915 \
		$(@D)/iwlwifi-* $(@D)/qcom $(@D)/intel
endef
KERNELFIRMWARES_PRE_INSTALL_TARGET_HOOKS += KERNELFIRMWARES_REMOVE_BRCM_FOLDER_CMDS
endif

define KERNELFIRMWARES_REMOVE_USELESS_FOLDER_CMDS
	$(RM) -rf $(@D)/mellanox $(@D)/bnx2x $(@D)/liquidio $(@D)/netronome \
		$(@D)/qcom/sc8280xp $(@D)/qcom/sm8250 $(@D)/qcom/sdm845 $(@D)/qcom/apq8096 \
		$(@D)/mrvl/prestera $(@D)/dpaa2
endef
KERNELFIRMWARES_PRE_INSTALL_TARGET_HOOKS += KERNELFIRMWARES_REMOVE_USELESS_FOLDER_CMDS

define KERNELFIRMWARES_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/lib/firmware
	cp -pr $(@D)/* $(TARGET_DIR)/lib/firmware/
	cd $(TARGET_DIR)/lib/firmware/ ; \
	sed -r -e '/^Link: (.+) -> (.+)$$/!d; s//\1 \2/' $(@D)/WHENCE | \
	while read f d; do \
		if test -f "$$(readlink -m $$(dirname $$f)/$$d)"; then \
			mkdir -p $$(dirname $$f) || exit 1; \
			ln -sf $$d $$f || exit 1; \
		fi ; \
	done
endef

$(eval $(generic-package))
