################################################################################
#
# VITAVOYAGER
#
################################################################################

# Commit of 2026/02/05
LIBRETRO_VITAVOYAGER_VERSION = f805da8a9609e7fec4ccc94e8fd9af4689fc404e
LIBRETRO_VITAVOYAGER_SITE = $(call gitlab,Pit64,vitaVoyager,$(LIBRETRO_VITAVOYAGER_VERSION))
LIBRETRO_VITAVOYAGER_LICENSE = GPL-2.0
LIBRETRO_VITAVOYAGER_LICENSE_FILES = COPYING.txt

# Required from GCC14
LIBRETRO_VITAVOYAGER_GCC14_OPTIONS = -Wno-implicit-int -Wno-implicit-function-declaration -Wno-incompatible-pointer-types -Wl,--allow-multiple-definition

LIBRETRO_VITAVOYAGER_DEPENDENCIES =

ifeq ($(BR2_PACKAGE_HAS_LIBGLES),y)
LIBRETRO_VITAVOYAGER_DEPENDENCIES += libgles
endif

ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
LIBRETRO_VITAVOYAGER_DEPENDENCIES += libgl
endif

define LIBRETRO_VITAVOYAGER_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO) $(LIBRETRO_VITAVOYAGER_GCC14_OPTIONS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) $(LIBRETRO_VITAVOYAGER_GCC14_OPTIONS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO) $(LIBRETRO_VITAVOYAGER_GCC14_OPTIONS)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_VITAVOYAGER_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/vitavoyager_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/vitavoyager_libretro.so
	mkdir -p "$(TARGET_DIR)/recalbox/share_upgrade/roms/ports/Star Trek Voyager"
	cp -R $(@D)/data/voyager/baseEF "$(TARGET_DIR)/recalbox/share_upgrade/roms/ports/Star Trek Voyager"
	mkdir -p "$(TARGET_DIR)/recalbox/share_upgrade/roms/ports/Star Trek Voyager/baseEF/scripts"
endef

$(eval $(generic-package))
