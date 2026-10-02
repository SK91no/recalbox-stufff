################################################################################
#
# Recalbox themes for EmulationStation : https://gitlab.com/recalbox/recalbox-themes
#
################################################################################

ifneq ($(GITLAB_TOKEN_THEMES),)
RECALBOX_THEMES_VERSION = 73f22c5651fa7f4430342c651a6f640585521258
RECALBOX_THEMES_SITE = https://gitlab-ci-token:$(GITLAB_TOKEN_THEMES)@gitlab.com/recalbox/recalbox-themes-prime
else
RECALBOX_THEMES_VERSION = febce03c483cca4fdcd873938e974e0a52930418
RECALBOX_THEMES_SITE = https://gitlab.com/recalbox/recalbox-themes
endif
RECALBOX_THEMES_SITE_METHOD = git
RECALBOX_THEMES_LICENSE = CC-BY-NC-ND-4.0

define RECALBOX_THEMES_COMMON
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
	cp -r $(@D)/themes/recalbox-next \
		$(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
	cp -r $(@D)/themes/recalbox-next-v9 \
		$(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
	cp -r $(@D)/themes/recalbox-goa2 \
		$(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
	cp -r $(@D)/themes/recalbox-240p \
		$(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
endef

define RECALBOX_THEMES_GPICASE2W
	cp -r $(@D)/themes/recalbox-goa-gpicase \
		$(TARGET_DIR)/recalbox/share_init/system/.emulationstation/themes/
endef

RECALBOX_THEMES_INSTALL_TARGET_CMDS = $(RECALBOX_THEMES_COMMON)
ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2),y)
RECALBOX_THEMES_INSTALL_TARGET_CMDS += && $(RECALBOX_THEMES_GPICASE2W)
endif

$(eval $(generic-package))
