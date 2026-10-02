################################################################################
#
# Pico-8 Lexaloffle (source files not supplied, must be purchased)
#
################################################################################

#PICO8_LEXALOFFLE_SITE = https://www.lexaloffle.com/pico-8.php
PICO8_LEXALOFFLE_VERSION = 0.2.7
PICO8_LEXALOFFLE_SOURCE =
PICO8_LEXALOFFLE_LICENSE = COMMERCIAL

define PICO8_LEXALOFFLE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pico-8
endef

$(eval $(generic-package))
