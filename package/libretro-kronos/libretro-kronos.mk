################################################################################
#
# KRONOS
#
################################################################################

# Commit of 2025/03/12
LIBRETRO_KRONOS_VERSION = d0384bc33aad06bfa9cf5cd308c4bd7ae6cfc814
LIBRETRO_KRONOS_SITE = $(call github,FCare,Kronos,$(LIBRETRO_KRONOS_VERSION))
LIBRETRO_KRONOS_LICENSE = GPL-2.0

LIBRETRO_KRONOS_DEPENDENCIES =

ifeq ($(BR2_PACKAGE_HAS_LIBGLES),y)
  	LIBRETRO_KRONOS_GL = GLESv2
  	LIBRETRO_KRONOS_DEPENDENCIES += libgles
else
  	LIBRETRO_KRONOS_GL = GL
  	LIBRETRO_KRONOS_DEPENDENCIES += libgl
endif

define LIBRETRO_KRONOS_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/yabause/src/libretro/Makefile
	mkdir -p $(@D)/build_retro
	cd $(@D)/build_retro
	$(MAKE) -C $(@D)/yabause/src/libretro generate-files
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" AR="$(TARGET_AR)" LD="$(TARGET_LD)" RANLIB="$(TARGET_RANLIB)" -C $(@D)/yabause/src/libretro -f Makefile platform="unix-$(RETROARCH_LIBRETRO_PLATFORM)" $(LIBRETRO_KRONOS_SUPP_OPT) \
		GIT_VERSION="-$(shell echo $(LIBRETRO_KRONOS_VERSION) | cut -c 1-8)"
endef

define LIBRETRO_KRONOS_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/yabause/src/libretro/kronos_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/kronos_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/saturn
endef

$(eval $(generic-package))
