################################################################################
#
# HATARIB
#
################################################################################

# Commit of 2024/07/05
LIBRETRO_HATARIB_VERSION = e8187a7be75ff1c547cdcef214a8da26847561c1
LIBRETRO_HATARIB_SITE = https://github.com/bbbradsmith/hatariB.git
LIBRETRO_HATARIB_LICENSE = GPL-2.0
# Non commercial because of libcapsimage dependency
LIBRETRO_HATARIB_NON_COMMERCIAL = y
LIBRETRO_HATARIB_SITE_METHOD = git
LIBRETRO_HATARIB_DEPENDENCIES = libcapsimage sdl2 libzlib host-cmake

LIBRETRO_HATARIB_BUILD_OPTS += CMAKE=$(HOST_DIR)/bin/cmake

LIBRETRO_HATARIB_CONF_ENV += SDL2_INCLUDE="$(STAGING_DIR)/usr/include/SDL2" \
	SDL2_LIB="$(STAGING_DIR)/usr/lib" \
	SDL2_LINK="$(STAGING_DIR)/usr/lib/libSDL2.so" \
	ZLIB_INCLUDE="$(STAGING_DIR)/usr/include" \
	ZLIB_LIB="$(STAGING_DIR)/usr/lib" \
	ZLIB_LINK="$(STAGING_DIR)/usr/lib/libz.so"

define LIBRETRO_HATARIB_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(LIBRETRO_HATARIB_COMPILER_COMMONS_CFLAGS) $(LIBRETRO_HATARIB_CAPSIMAGE_FLAGS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(LIBRETRO_HATARIB_COMPILER_COMMONS_CXXFLAGS) $(LIBRETRO_HATARIB_CAPSIMAGE_FLAGS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(LIBRETRO_HATARIB_COMPILER_COMMONS_LDFLAGS) $(LIBRETRO_HATARIB_CAPSIMAGE_FLAGS)" \
		ASFLAGS="$(TARGET_ASFLAGS)" PLATFLAGS="$(TARGET_PLATFLAGS)" SHARED="$(TARGET_SHARED)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)" $(LIBRETRO_HATARIB_BUILD_OPTS) $(LIBRETRO_HATARIB_CONF_ENV)
endef

define LIBRETRO_HATARIB_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/build/hatarib.so \
		$(TARGET_DIR)/usr/lib/libretro/hatarib_libretro.so
endef

$(eval $(generic-package))
