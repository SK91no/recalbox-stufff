################################################################################
#
# libretro-mojozork
#
################################################################################

# Commit of 2023/08/14
LIBRETRO_MOJOZORK_VERSION = 89832e31d9d05c4e81908286b99db21924f3ba1a
LIBRETRO_MOJOZORK_SITE = $(call github,icculus,mojozork,$(LIBRETRO_MOJOZORK_VERSION))
LIBRETRO_MOJOZORK_LICENSE = zlib
LIBRETRO_MOJOZORK_LICENSE_FILE = LICENSE.txt
LIBRETRO_MOJOZORK_DEPENDENCIES = retroarch

LIBRETRO_MOJOZORK_CONF_OPTS += \
	-DCMAKE_BUILD_TYPE=Release \
	-DLIBRETRO=ON

define LIBRETRO_MOJOZORK_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/mojozork_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mojozork_libretro.so
endef

$(eval $(cmake-package))
