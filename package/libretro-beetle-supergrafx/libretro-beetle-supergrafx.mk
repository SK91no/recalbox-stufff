################################################################################
#
# BEETLE_SUPERGRAFX
#
################################################################################

# Commit of 2023/09/29
LIBRETRO_BEETLE_SUPERGRAFX_VERSION = 733b29460bfa254ccea096d29e8cc566e48aa633
LIBRETRO_BEETLE_SUPERGRAFX_SITE = $(call github,libretro,beetle-supergrafx-libretro,$(LIBRETRO_BEETLE_SUPERGRAFX_VERSION))
LIBRETRO_BEETLE_SUPERGRAFX_LICENSE = GPL-2.0
LIBRETRO_BEETLE_SUPERGRAFX_LICENSE_FILES = COPYING

define LIBRETRO_BEETLE_SUPERGRAFX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_BEETLE_SUPERGRAFX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/mednafen_supergrafx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mednafen_supergrafx_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pcengine
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pcenginecd
endef

$(eval $(generic-package))
