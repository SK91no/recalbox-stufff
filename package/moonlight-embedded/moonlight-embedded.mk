################################################################################
#
# moonlight-embedded
#
################################################################################

# Commit of 2026/03/28
MOONLIGHT_EMBEDDED_VERSION = f7dc33c87064e97f7f4e29457e312b9cceb24349
MOONLIGHT_EMBEDDED_SITE = https://github.com/moonlight-stream/moonlight-embedded.git
MOONLIGHT_EMBEDDED_SITE_METHOD = git
MOONLIGHT_EMBEDDED_GIT_SUBMODULES = y
MOONLIGHT_EMBEDDED_LICENSE = GPL-3.0+
MOONLIGHT_EMBEDDED_LICENSE_FILES = LICENSE

MOONLIGHT_EMBEDDED_DEPENDENCIES = \
	opus \
	expat \
	libevdev \
	avahi \
	alsa-lib \
	udev \
	libcurl \
	libcec \
	ffmpeg \
	libenet

MOONLIGHT_EMBEDDED_CONF_OPTS = \
	-DCMAKE_INSTALL_SYSCONFDIR=/etc \
	-DENABLE_FFMPEG=ON \
	-DENABLE_SDL=ON \
	-DENABLE_X11=OFF \
	-DENABLE_V4L2_DRM=OFF \
	-DENABLE_CEC=ON \
	-DENABLE_PULSE=ON \
	-DENABLE_VDPAU=OFF \
	-DENABLE_MMAL=OFF

ifeq ($(BR2_PACKAGE_LIBAMCODEC),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += libamcodec
endif

ifeq ($(BR2_PACKAGE_RPI_FIRMWARE),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += rpi-firmware
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += libdrm
MOONLIGHT_EMBEDDED_CONF_OPTS += \
	-DENABLE_V4L2_DRM=ON \
	-DENABLE_MMAL=OFF

else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += libdrm
MOONLIGHT_EMBEDDED_CONF_OPTS += \
	-DENABLE_V4L2_DRM=ON \
	-DENABLE_MMAL=OFF

else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += sdl2 pulseaudio libva xlib_libX11
MOONLIGHT_EMBEDDED_CONF_OPTS += \
	-DENABLE_X11=ON \
	-DENABLE_PULSE=ON \
	-DENABLE_VDPAU=ON

endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64)$(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
MOONLIGHT_EMBEDDED_DEPENDENCIES += libgbm libegl libgles
endif

define MOONLIGHT_EMBEDDED_CUSTOM_CONFIG_HOOK
	$(INSTALL) -D $(MOONLIGHT_EMBEDDED_PKGDIR)/moonlight-other.conf \
		$(TARGET_DIR)/recalbox/share_init/system/configs/moonlight/moonlight-board.conf
	$(INSTALL) -D -m 0755 $(MOONLIGHT_EMBEDDED_PKGDIR)/scripts/Moonlight.sh \
		$(TARGET_DIR)/recalbox/scripts/moonlight/Moonlight.sh
	$(INSTALL) -D -m 0755 $(MOONLIGHT_EMBEDDED_PKGDIR)/scripts/moonlight.inc.sh \
		$(TARGET_DIR)/recalbox/scripts/moonlight/moonlight.inc.sh
	$(INSTALL) -D -m 0755 $(MOONLIGHT_EMBEDDED_PKGDIR)/scripts/generic.inc.sh \
		$(TARGET_DIR)/recalbox/scripts/moonlight/generic.inc.sh
endef

MOONLIGHT_EMBEDDED_POST_INSTALL_TARGET_HOOKS += MOONLIGHT_EMBEDDED_CUSTOM_CONFIG_HOOK

$(eval $(cmake-package))
