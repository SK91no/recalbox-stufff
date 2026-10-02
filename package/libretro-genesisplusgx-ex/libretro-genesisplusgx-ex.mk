################################################################################
#
# GENESIS-PLUS-GX-EX
#
################################################################################

# Commit of 2025/07/07
LIBRETRO_GENESISPLUSGX_EX_VERSION = dea3528cc0e3f54098e1fb91e3ae7e0e5d90a590
LIBRETRO_GENESISPLUSGX_EX_SITE = $(call github,BillyTimeGames,Genesis-Plus-GX-Expanded-Rom-Size,$(LIBRETRO_GENESISPLUSGX_EX_VERSION))
LIBRETRO_GENESISPLUSGX_EX_LICENSE = COPYRIGHT
LIBRETRO_GENESISPLUSGX_EX_LICENSE_FILES = LICENSE.txt
LIBRETRO_GENESISPLUSGX_EX_NON_COMMERCIAL = y

define LIBRETRO_GENESISPLUSGX_EX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile.libretro
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) -f Makefile.libretro platform="$(RETROARCH_LIBRETRO_BOARD)"
endef

define LIBRETRO_GENESISPLUSGX_EX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/genesis_plus_gx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/genesisplusgx_ex_libretro.so
endef

$(eval $(generic-package))
