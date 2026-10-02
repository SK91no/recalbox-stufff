################################################################################
#
# BEETLE_PCE_FAST
#
################################################################################

# Commit of 2023/06/10
LIBRETRO_BEETLE_PCE_FAST_VERSION = 1ce7a4a941b10aa0c2973cb441b89ee99e2c8d0e
LIBRETRO_BEETLE_PCE_FAST_SITE = $(call github,libretro,beetle-pce-fast-libretro,$(LIBRETRO_BEETLE_PCE_FAST_VERSION))
LIBRETRO_BEETLE_PCE_FAST_LICENSE = GPL-2.0
LIBRETRO_BEETLE_PCE_FAST_LICENSE_FILES = COPYING

define LIBRETRO_BEETLE_PCE_FAST_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) platform="$(RETROARCH_LIBRETRO_BOARD)"
endef

define LIBRETRO_BEETLE_PCE_FAST_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/mednafen_pce_fast_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mednafen_pce_fast_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pcengine
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pcenginecd
endef

$(eval $(generic-package))
