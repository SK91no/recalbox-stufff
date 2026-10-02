################################################################################
#
# python-pygame
#
################################################################################

PYTHON_PYGAME2_VERSION = 2.2.0
PYTHON_PYGAME2_SITE = $(call github,pygame,pygame,$(PYTHON_PYGAME2_VERSION))
PYTHON_PYGAME2_SETUP_TYPE = setuptools
PYTHON_PYGAME2_LICENSE = LGPL-2.1+
PYTHON_PYGAME2_LICENSE_FILES = LGPL
PYTHON_PYGAME2_DEPENDENCIES = sdl2
PYTHON_PYGAME2_BUILD_OPTS = -C-config -C-auto

ifeq ($(BR2_PACKAGE_PYTHON_PYGAME2_IMAGE),y)
PYTHON_PYGAME2_DEPENDENCIES += sdl2_image
endif

ifeq ($(BR2_PACKAGE_PYTHON_PYGAME2_FONT),y)
PYTHON_PYGAME2_DEPENDENCIES += sdl2_ttf
endif

ifeq ($(BR2_PACKAGE_PYTHON_PYGAME2_MIXER),y)
PYTHON_PYGAME2_DEPENDENCIES += sdl2_mixer
endif

ifeq ($(BR2_ARM_CPU_HAS_NEON),y)
PYTHON_PYGAME2_BUILD_OPTS += -C-enable-arm-neon
endif

PYTHON_PYGAME2_ENV = \
	PNG_CONFIG="$(STAGING_DIR)/usr/bin/libpng-config" \
	SDL_CONFIG="$(STAGING_DIR)/usr/bin/sdl2-config" \
	PKG_CONFIG_PATH="$(STAGING_DIR)/usr/lib/pkgconfig" \
	LOCALBASE="$(STAGING_DIR)/usr"

define PYTHON_PYGAME2_REMOVE_DOC
	rm -rf $(TARGET_DIR)/usr/lib/python*/site-packages/pygame/docs
endef

PYTHON_PYGAME2_POST_INSTALL_TARGET_HOOKS += PYTHON_PYGAME2_REMOVE_DOC

define PYTHON_PYGAME2_REMOVE_TESTS
	rm -rf $(TARGET_DIR)/usr/lib/python*/site-packages/pygame/tests
endef

PYTHON_PYGAME2_POST_INSTALL_TARGET_HOOKS += PYTHON_PYGAME2_REMOVE_TESTS

ifneq ($(BR2_PACKAGE_PYTHON_PYGAME2_EXAMPLES),y)
define PYTHON_PYGAME2_REMOVE_EXAMPLES
	rm -rf $(TARGET_DIR)/usr/lib/python$(PYTHON3_VERSION_MAJOR)/site-packages/pygame/examples
endef
PYTHON_PYGAME2_POST_INSTALL_TARGET_HOOKS += PYTHON_PYGAME2_REMOVE_EXAMPLES
endif

$(eval $(python-package))
