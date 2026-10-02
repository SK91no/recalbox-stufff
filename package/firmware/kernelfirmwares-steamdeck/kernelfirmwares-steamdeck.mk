################################################################################
#
# kernelfirmwares-steamdeck
#
################################################################################

KERNELFIRMWARES_STEAMDECK_VERSION = jupiter-20250731.1
KERNELFIRMWARES_STEAMDECK_SOURCE = linux-firmware-neptune-$(KERNELFIRMWARES_STEAMDECK_VERSION).tar.gz
KERNELFIRMWARES_STEAMDECK_SITE = https://gitlab.com/evlaV/linux-firmware-neptune/-/archive/$(KERNELFIRMWARES_STEAMDECK_VERSION)/$(KERNELFIRMWARES_STEAMDECK_SOURCE)?ref_type=tags
KERNELFIRMWARES_STEAMDECK_LICENSE = COPYRIGHT
KERNELFIRMWARES_STEAMDECK_NON_COMMERCIAL = y
KERNELFIRMWARES_STEAMDECK_FILES = LICENCE.atheros_firmware 

# target path /usr/share is not bin checked
# by buildroot
define KERNELFIRMWARES_STEAMDECK_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/
	rsync -av $(@D)/ath11k $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/
	rsync -av $(@D)/qca $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/
	rsync -av $(@D)/rtl_bt/rtl8822cu_config.bin $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/rtl_bt/
	rsync -av $(@D)/rtl_bt/rtl8822cu_fw.bin $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/rtl_bt/
	ln -sf QCA2066 $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/ath11k/QCA206X
	ln -sf cirrus/cs35l41-dsp1-spk-prot-vlv1776.wmfw $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/cs35l41-dsp1-spk-prot.wmfw
	ln -sf cirrus/cs35l41-dsp1-spk-prot-vlv1776.bin $(TARGET_DIR)/usr/share/recalbox/firmware/steamdeck/cs35l41-dsp1-spk-prot.bin
endef

$(eval $(generic-package))
