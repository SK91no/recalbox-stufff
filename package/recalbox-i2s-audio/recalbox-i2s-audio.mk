################################################################################
#
# RECALBOX_I2S_AUDIO
#
################################################################################

RECALBOX_I2S_AUDIO_VERSION = custom
RECALBOX_I2S_AUDIO_SITE = $(TOPDIR)/../projects/recalbox-i2s-audio
RECALBOX_I2S_AUDIO_SITE_METHOD = local
RECALBOX_I2S_AUDIO_LICENSE = GPL
RECALBOX_I2S_AUDIO_DEPENDENCIES = linux

$(eval $(kernel-module))
$(eval $(generic-package))
