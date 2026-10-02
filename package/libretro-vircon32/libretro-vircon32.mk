################################################################################
#
# VIRCON32
#
################################################################################

# Commit version of 03/31/2026
LIBRETRO_VIRCON32_VERSION = 3faedc6c577333eb785a93cec96ad7d484309f3c
LIBRETRO_VIRCON32_SITE = https://github.com/vircon32/vircon32-libretro
LIBRETRO_VIRCON32_SITE_METHOD = git
LIBRETRO_VIRCON32_GIT_SUBMODULES = YES
LIBRETRO_VIRCON32_LICENSE = BSD-3-Clause
LIBRETRO_VIRCON32_LICENSE_FILES = LICENSE

LIBRETRO_VIRCON32_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release

LIBRETRO_VIRCON32_DEPENDENCIES =

ifeq ($(BR2_PACKAGE_RECALBOX_HAS_LIBGLES),y)
LIBRETRO_VIRCON32_CONF_OPTS += -DPLATFORM=EMUELEC
LIBRETRO_VIRCON32_DEPENDENCIES += libgles
ifeq ($(BR2_PACKAGE_RECALBOX_HAS_LIBGLES3),y)
LIBRETRO_VIRCON32_CONF_OPTS += -DENABLE_OPENGLES3=1
else
LIBRETRO_VIRCON32_CONF_OPTS += -DENABLE_OPENGLES2=1
endif
else ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
LIBRETRO_VIRCON32_DEPENDENCIES += libgl
endif

define LIBRETRO_VIRCON32_INSTALL_TARGET_CMDS
		$(INSTALL) -D $(@D)/vircon32_libretro.so \
			$(TARGET_DIR)/usr/lib/libretro/vircon32_libretro.so
		mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/vircon32
endef

$(eval $(cmake-package))
