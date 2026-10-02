################################################################################
#
# NESTOPIA
#
################################################################################

# Commit of 2023/05/28
LIBRETRO_NESTOPIA_VERSION = 3dcbec4682e079312d6943e1357487645ec608c7
LIBRETRO_NESTOPIA_SITE = $(call github,libretro,nestopia,$(LIBRETRO_NESTOPIA_VERSION))
LIBRETRO_NESTOPIA_LICENSE = GPL-2.0
LIBRETRO_NESTOPIA_LICENSE_FILES = COPYING

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64),y)
LIBRETRO_NESTOPIA_BOARD = unix
else
LIBRETRO_NESTOPIA_BOARD = $(RETROARCH_LIBRETRO_BOARD)
endif

define LIBRETRO_NESTOPIA_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/libretro/ platform="$(LIBRETRO_NESTOPIA_BOARD)"
endef

define LIBRETRO_NESTOPIA_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/libretro/nestopia_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/nestopia_libretro.so
	# NstDatabase needed for proper emulation (backed into the core)
	# cp $(@D)/NstDatabase.xml $(TARGET_DIR)/recalbox/share_upgrade/bios
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/fds
endef

$(eval $(generic-package))
