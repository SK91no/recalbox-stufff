################################################################################
#
# GENESIS-PLUS-GX
#
################################################################################

# Commit of 2025/12/12
LIBRETRO_GENESISPLUSGX_VERSION = 83485a2730ae790ce07dd0803ed73bc4ca85c70c
LIBRETRO_GENESISPLUSGX_SITE = $(call github,libretro,Genesis-Plus-GX,$(LIBRETRO_GENESISPLUSGX_VERSION))
LIBRETRO_GENESISPLUSGX_LICENSE = COPYRIGHT
LIBRETRO_GENESISPLUSGX_LICENSE_FILES = LICENSE.txt
LIBRETRO_GENESISPLUSGX_NON_COMMERCIAL = y

define LIBRETRO_GENESISPLUSGX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile.libretro
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) -f Makefile.libretro platform="$(RETROARCH_LIBRETRO_BOARD)" \
		GIT_VERSION="-$(shell echo $(LIBRETRO_GENESISPLUSGX_VERSION) | cut -c 1-8)"
endef

define LIBRETRO_GENESISPLUSGX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/genesis_plus_gx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/genesisplusgx_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/gamegear
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/mastersystem
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/megadrive
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/segacd
endef

$(eval $(generic-package))
