################################################################################
#
# SOUND_OPEN_FIRMWARE
#
################################################################################

SOUND_OPEN_FIRMWARE_VERSION = 2025.01.1
SOUND_OPEN_FIRMWARE_SOURCE = sof-bin-$(SOUND_OPEN_FIRMWARE_VERSION).tar.gz
SOUND_OPEN_FIRMWARE_SITE = https://github.com/thesofproject/sof-bin/releases/download/v$(SOUND_OPEN_FIRMWARE_VERSION)
SOUND_OPEN_FIRMWARE_LICENSE = BSD-3-Clause
SOUND_OPEN_FIRMWARE_LICENSE_FILES = LICENSE.Intel LICENSE.NXP

define SOUND_OPEN_FIRMWARE_INSTALL_TARGET_CMDS
  [ -d $(TARGET_DIR)/lib/firmware/intel ] || mkdir -p $(TARGET_DIR)/lib/firmware/intel ; \
	rsync -av $(@D)/sof* $(TARGET_DIR)/lib/firmware/intel/
endef

$(eval $(generic-package))
