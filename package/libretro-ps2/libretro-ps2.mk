################################################################################
#
# libretro-ps2
#
################################################################################

# https://github.com/libretro/ps2/commits/libretroization/
# Commit of 2025/09/29
LIBRETRO_PS2_VERSION = 9485a53fa5aa2bff17e04518116107f81a8c82e3
LIBRETRO_PS2_SITE = https://github.com/libretro/ps2.git
LIBRETRO_PS2_SITE_METHOD = git
LIBRETRO_PS2_GIT_SUBMODULES = YES
LIBRETRO_PS2_LICENSE = GPLv2
LIBRETRO_PS2_DEPENDENCIES = libaio xz host-xxd retroarch

LIBRETRO_PS2_SUPPORTS_IN_SOURCE_BUILD = NO

LIBRETRO_PS2_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
LIBRETRO_PS2_CONF_OPTS += -DBUILD_SHARED_LIBS=OFF
LIBRETRO_PS2_CONF_OPTS += -DLIBRETRO=ON
LIBRETRO_PS2_CONF_OPTS += -DBUILD_REGRESS=OFF
LIBRETRO_PS2_CONF_OPTS += -DBUILD_TOOLS=OFF
LIBRETRO_PS2_CONF_OPTS += -DUSE_OPENGL=ON
LIBRETRO_PS2_CONF_OPTS += -DDISABLE_ADVANCE_SIMD=ON

ifeq ($(BR2_PACKAGE_RECALBOX_HAS_VULKAN),y)
LIBRETRO_PS2_DEPENDENCIES += vulkan-headers
LIBRETRO_PS2_CONF_OPTS += -DUSE_VULKAN=ON
endif

define LIBRETRO_PS2_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/buildroot-build/bin/pcsx2_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/ps2_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/ps2
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/ps2/resources
	cp -R $(@D)/bin/resources/GameIndex.yaml \
		$(TARGET_DIR)/recalbox/share_upgrade/bios/ps2/resources
endef

$(eval $(cmake-package))
