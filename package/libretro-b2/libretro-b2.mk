################################################################################
#
# B2 (BBC Micro)
#
################################################################################

# Commit of 2025/02/09
LIBRETRO_B2_VERSION = 3aa42677817ece64ef26a09faacdfacb30d098f4
LIBRETRO_B2_SITE = $(call github,zoltanvb,b2-libretro,$(LIBRETRO_B2_VERSION))
LIBRETRO_B2_LICENSE = GPL-3.0
LIBRETRO_B2_LICENSE_FILES = LICENSE
LIBRETRO_B2_DEPENDENCIES = util-linux-libs zlib

define LIBRETRO_B2_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/src/libretro/Makefile
		CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/src/libretro -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_B2_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/src/libretro/b2_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/b2_libretro.so
endef

$(eval $(generic-package))
