################################################################################
#
# applewin
#
################################################################################

# Commit of 2025/01/26
LIBRETRO_APPLEWIN_VERSION = 612a87c9773335f5cd2606118d386b4a55dd8242
LIBRETRO_APPLEWIN_SITE = https://github.com/audetto/AppleWin
LIBRETRO_APPLEWIN_SITE_METHOD=git
LIBRETRO_APPLEWIN_GIT_SUBMODULES=YES
LIBRETRO_APPLEWIN_SUPPORTS_IN_SOURCE_BUILD = NO
LIBRETRO_APPLEWIN_LICENSE = GPLv2
LIBRETRO_APPLEWIN_DEPENDENCIES = sdl2 sdl2_image minizip-zlib slirp libpcap boost libyaml

LIBRETRO_APPLEWIN_CONF_OPTS  = -DCMAKE_BUILD_TYPE=Release
LIBRETRO_APPLEWIN_CONF_OPTS += -DBUILD_SA2=ON -DBUILD_LIBRETRO=ON

ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
LIBRETRO_APPLEWIN_CONF_OPTS += -DSA2_OPENGL=ON
else
LIBRETRO_APPLEWIN_CONF_OPTS += -DSA2_OPENGL=OFF
endif

define LIBRETRO_APPLEWIN_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib/libretro
	$(INSTALL) -D $(@D)/buildroot-build/source/frontends/libretro/applewin_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/applewin_libretro.so
	mv $(@D)/buildroot-build/sa2 $(@D)/buildroot-build/applewin
	$(INSTALL) -D $(@D)/buildroot-build/applewin $(TARGET_DIR)/usr/bin/applewin
	mkdir -p $(TARGET_DIR)/usr/share/applewin
	cp -R $(@D)/resource/* $(TARGET_DIR)/usr/share/applewin/
	rm $(TARGET_DIR)/usr/share/applewin/*.h
endef

$(eval $(cmake-package))
