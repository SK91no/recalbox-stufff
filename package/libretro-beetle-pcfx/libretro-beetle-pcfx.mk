################################################################################
#
# BEETLE_PCFX
#
################################################################################

# Commit of 2023/05/28
LIBRETRO_BEETLE_PCFX_VERSION = 47c355b6a515aef6dc57f57df1535570108a0e21
LIBRETRO_BEETLE_PCFX_SITE = $(call github,libretro,beetle-pcfx-libretro,$(LIBRETRO_BEETLE_PCFX_VERSION))
LIBRETRO_BEETLE_PCFX_LICENSE = GPL-2.0
LIBRETRO_BEETLE_PCFX_LICENSE_FILES = COPYING

ifeq ($(BR2_aarch64),y)
LIBRETRO_BEETLE_PCFX_MAKEFILE_EXTRA_OPTS = IS_X86=0
endif

define LIBRETRO_BEETLE_PCFX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" AR="$(TARGET_AR)" LD="$(TARGET_CXX)" RANLIB="$(TARGET_RANLIB)" -C $(@D) platform="$(RETROARCH_LIBRETRO_PLATFORM)" $(LIBRETRO_BEETLE_PCFX_MAKEFILE_EXTRA_OPTS)
endef

define LIBRETRO_BEETLE_PCFX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/mednafen_pcfx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mednafen_pcfx_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/pcfx
endef

$(eval $(generic-package))
