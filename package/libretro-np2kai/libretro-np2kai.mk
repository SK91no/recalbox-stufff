################################################################################
#
# NP2KAI
#
################################################################################

# Commit of 2023/02/26
LIBRETRO_NP2KAI_VERSION = 53a4fa3d698c22bf02a7fc78c3b73ba55e4a0732
LIBRETRO_NP2KAI_SITE = $(call github,AZO234,NP2kai,$(LIBRETRO_NP2KAI_VERSION))
LIBRETRO_NP2KAI_LICENSE = MIT

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4)$(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64)$(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
LIBRETRO_NP2KAI_PLATFORM=rpi4-embedded
else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI3)$(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2LEGACY)$(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2),y)
LIBRETRO_NP2KAI_PLATFORM=rpi3-embedded
else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_ODROIDXU4),y)
LIBRETRO_NP2KAI_PLATFORM=xu4-embedded
else
LIBRETRO_NP2KAI_PLATFORM=unix
endif

# Np2Kai is no longer compiling w/ GXX14+ because of strict pointer type check
LIBRETRO_NP2KAI_GCC14_OPTIONS = -Wno-incompatible-pointer-types

define LIBRETRO_NP2KAI_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/sdl/Makefile.libretro
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO) $(LIBRETRO_NP2KAI_GCC14_OPTIONS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) $(LIBRETRO_NP2KAI_GCC14_OPTIONS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) NP2KAI_VERSION=rev.1 NP2KAI_HASH=$(LIBRETRO_NP2KAI_VERSION) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/sdl -f Makefile.libretro platform="$(LIBRETRO_NP2KAI_PLATFORM)"
endef

define LIBRETRO_NP2KAI_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/sdl/np2kai_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/np2kai_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/np2kai
endef

$(eval $(generic-package))
