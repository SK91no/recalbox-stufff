################################################################################
#
# VALVE_HARDWARE_AUDIO_PROCESSING
#
################################################################################

VALVE_HARDWARE_AUDIO_PROCESSING_VERSION = 0.61
VALVE_HARDWARE_AUDIO_PROCESSING_SOURCE = valve-hardware-audio-processing-$(VALVE_HARDWARE_AUDIO_PROCESSING_VERSION).tar.gz
VALVE_HARDWARE_AUDIO_PROCESSING_SITE = https://gitlab.com/evlaV/valve-hardware-audio-processing/-/archive/$(VALVE_HARDWARE_AUDIO_PROCESSING_VERSION)
VALVE_HARDWARE_AUDIO_PROCESSING_LICENSE = GPL-2.0
VALVE_HARDWARE_AUDIO_PROCESSING_LICENSE_FILES = LICENSE

define VALVE_HARDWARE_AUDIO_PROCESSING_INSTALL_TARGET_CMDS
  [ -d $(TARGET_DIR)/lib/firmware/ ] || mkdir -p $(TARGET_DIR)/lib/firmware/ ; \
	rsync -av $(@D)/sof_fw/* $(TARGET_DIR)/lib/firmware/amd/ ; \
	[ -d $(TARGET_DIR)/usr/share/alsa/ucm2/conf.d/ ] || mkdir -p $(TARGET_DIR)/usr/share/alsa/ucm2/conf.d/ \;
	rsync -av $(@D)/ucm2/conf.d/ $(TARGET_DIR)/usr/share/alsa/ucm2/conf.d/
endef

$(eval $(generic-package))
