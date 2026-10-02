################################################################################
#
# libretro-mednafen-stv
#
################################################################################

# Commit of 2026/06/29
LIBRETRO_MEDNAFEN_STV_VERSION = 2e42ef0c5d8238c170bcf182e68951c214fc6867
LIBRETRO_MEDNAFEN_STV_SITE = $(call github,beudbeud,libretro-stv,$(LIBRETRO_MEDNAFEN_STV_VERSION))
LIBRETRO_MEDNAFEN_STV_LICENSE = GPLv2
LIBRETRO_MEDNAFEN_STV_DEPENDENCIES = retroarch zlib zstd arcade-dats

LIBRETRO_MEDNAFEN_STV_CONF_OPTS = -DCMAKE_BUILD_TYPE=Release
LIBRETRO_MEDNAFEN_STV_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_SO)"
LIBRETRO_MEDNAFEN_STV_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_SO)"

define LIBRETRO_MEDNAFEN_STV_BUILD_DATS
	$(call InstallArcadeFiles,libretro,mednafen-stv,1.32.1)
endef

LIBRETRO_MEDNAFEN_STV_POST_INSTALL_TARGET_HOOKS += LIBRETRO_MEDNAFEN_STV_BUILD_DATS

$(eval $(cmake-package))
