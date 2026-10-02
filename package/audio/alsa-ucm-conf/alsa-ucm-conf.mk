################################################################################
#
# ALSA_UCM_CONF
#
################################################################################

ALSA_UCM_CONF_VERSION = v1.2.13
ALSA_UCM_CONF_SITE = $(call github,alsa-project,alsa-ucm-conf,$(ALSA_UCM_CONF_VERSION))
ALSA_UCM_CONF_LICENSE = BSD-3
ALSA_UCM_CONF_LICENSE_FILES = LICENSE

define ALSA_UCM_CONF_INSTALL_TARGET_CMDS
	$(INSTALL) -d $(TARGET_DIR)/usr/share/alsa
	rsync -avP $(@D)/ucm $(TARGET_DIR)/usr/share/alsa/
	rsync -avP $(@D)/ucm2 $(TARGET_DIR)/usr/share/alsa/
endef

$(eval $(generic-package))
