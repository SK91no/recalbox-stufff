################################################################################
#
# guichan
#
################################################################################

# Commit of 2024/01/30
GUICHAN_VERSION = v0.8.2
GUICHAN_SITE = $(call github,darkbitsorg,guichan,$(GUICHAN_VERSION))
GUICHAN_DEPENDENCIES = sdl sdl_image
GUICHAN_INSTALL_STAGING = YES
GUICHAN_LICENSE = Custom
GUICHAN_LICENSE_FILES = COPYING
GUICHAN_AUTORECONF = YES

define GUICHAN_FIX_SDL_CONFIG_PATH
	$(SED) "s+sdl-config+$(STAGING_DIR)/usr/bin/sdl-config+g" $(@D)/configure
endef

GUICHAN_CONF_OPTS += --enable-force-sdl

$(eval $(autotools-package))

# Appended AFTER $(eval) so the SED runs after AUTORECONF_HOOK (which
# generates configure from configure.in on the darkbitsorg fork).
GUICHAN_PRE_CONFIGURE_HOOKS += GUICHAN_FIX_SDL_CONFIG_PATH
