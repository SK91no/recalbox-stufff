################################################################################
#
# ymir
#
################################################################################

# Commit form 2026/06/22
LIBRETRO_YMIR_VERSION = 75f4d36f005588715e9ee3fd4eb7eb4441027a94 
LIBRETRO_YMIR_SITE = https://github.com/beudbeud/Ymir.git
LIBRETRO_YMIR_SITE_METHOD = git
LIBRETRO_YMIR_GIT_SUBMODULES = YES
LIBRETRO_YMIR_LICENSE = GPLv2
LIBRETRO_YMIR_LICENSE_FILE = LICENSE

LIBRETRO_YMIR_SUPPORTS_IN_SOURCE_BUILD = NO

LIBRETRO_YMIR_DEPENDENCIES = libzlib zstd

LIBRETRO_YMIR_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release \
		  -DCMAKE_INSTALL_PREFIX=/usr \
		  -DYmir_ENABLE_LIBRETRO=ON \
		  -DYmir_ENABLE_SDL3_APP=OFF \
		  -DYmir_ENABLE_SANDBOX=OFF \
		  -DYmir_ENABLE_YMDASM=OFF \
		  -DYmir_ENABLE_YMIR_HEADLESS=OFF \
		  -DYmir_ENABLE_YMIR_DBG=OFF \
		  -DYmir_ENABLE_TESTS=OFF \
		  -DYmir_INCLUDE_PACKAGING=OFF \
		  -DYmir_DEV_BUILD=OFF \
		  -DYmir_ENABLE_IPO=OFF

ifeq ($(BR2_X86_CPU_HAS_AVX2),y)
    LIBRETRO_YMIR_CONF_OPTS += -DYmir_AVX2=ON
else
    LIBRETRO_YMIR_CONF_OPTS += -DYmir_AVX2=OFF
endif

define LIBRETRO_YMIR_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/buildroot-build/apps/ymir-libretro/ymir_libretro.so \
	    $(TARGET_DIR)/usr/lib/libretro/ymir_libretro.so
endef

$(eval $(cmake-package))
