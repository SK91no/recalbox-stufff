################################################################################
#
# libretro-dolphin
#
################################################################################

# Commit of 2025/12/26
LIBRETRO_DOLPHIN_VERSION = 990bffac731a86ecb075d5d786d4b348310beea6
LIBRETRO_DOLPHIN_SITE = https://github.com/libretro/dolphin
LIBRETRO_DOLPHIN_SITE_METHOD = git
LIBRETRO_DOLPHIN_GIT_SUBMODULES = YES
LIBRETRO_DOLPHIN_LICENSE = GPLv2
LIBRETRO_DOLPHIN_DEPENDENCIES = retroarch libevdev fmt bluez5_utils sdl2

LIBRETRO_DOLPHIN_PLATFORM = $(LIBRETRO_PLATFORM)

LIBRETRO_DOLPHIN_CONF_OPTS = -DLIBRETRO=ON \
	-DENABLE_TESTS=OFF \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_BUILD_TYPE=Release \
	-DLINUX=ON \
	-DENABLE_QT=OFF \
	-DENABLE_NOGUI=OFF \
	-DUSE_DISCORD_PRESENCE=OFF \
	-DUSE_MGBA=OFF \
	-DENABLE_ANALYTICS=OFF

ifeq ($(BR2_PACKAGE_XSERVER_XORG_SERVER),y)
LIBRETRO_DOLPHIN_DEPENDENCIES += xserver_xorg-server
LIBRETRO_DOLPHIN_CONF_OPTS += -DENABLE_X11=ON
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64),y)
LIBRETRO_DOLPHIN_CONF_OPTS += -DENABLE_EGL=OFF
LIBRETRO_DOLPHIN_CONF_OPTS += -DENABLE_X11=OFF
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
LIBRETRO_DOLPHIN_CONF_OPTS += -DENABLE_EGL=ON
LIBRETRO_DOLPHIN_CONF_OPTS += -DENABLE_X11=OFF
LIBRETRO_DOLPHIN_DEPENDENCIES += vulkan-headers
endif

define LIBRETRO_DOLPHIN_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/dolphin_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/dolphin_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/dolphin-emu/Sys
	cp -R $(@D)/Data/Sys/* $(TARGET_DIR)/recalbox/share_upgrade/bios/dolphin-emu/Sys
endef

$(eval $(cmake-package))
