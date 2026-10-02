################################################################################
#
# moonlight-qt
#
################################################################################

# Commit of 2023/10/15
MOONLIGHT_QT_VERSION = eef270de2697e98cc6d7a26cbdadfc89ddbea82e
MOONLIGHT_QT_SITE = https://github.com/moonlight-stream/moonlight-qt.git
MOONLIGHT_QT_SITE_METHOD = git
MOONLIGHT_QT_GIT_SUBMODULES=y
MOONLIGHT_QT_DEPENDENCIES = opus expat libevdev avahi alsa-lib udev libcurl libcec ffmpeg sdl2 libenet \
			qt5svg pulseaudio openssl sdl2_ttf qt5base qt5quickcontrols2
MOONLIGHT_QT_LICENSE = GPL-3.0
MOONLIGHT_QT_LICENSE_FILES = LICENSE

MOONLIGHT_QT_CONF_OPTS = CONFIG+=embedded CONFIG+=release PREFIX=$(STAGING_DIR)/usr

ifeq ($(BR2_PACKAGE_RECALBOX_VIDEO_XORG_SERVER),y)
MOONLIGHT_QT_DEPENDENCIES += xlib_libX11
endif

ifeq ($(BR2_PACKAGE_HAS_LIBEGL),y)
MOONLIGHT_QT_DEPENDENCIES += libegl
endif

ifeq ($(BR2_PACKAGE_HAS_GLES),y)
MOONLIGHT_QT_DEPENDENCIES += libgles
endif

ifeq ($(BR2_PACKAGE_HAS_GL),y)
MOONLIGHT_QT_DEPENDENCIES += libgl
endif

ifeq ($(BR2_PACKAGE_LIBAMCODEC),y)
MOONLIGHT_QT_DEPENDENCIES += libamcodec
endif

ifeq ($(BR2_PACKAGE_RPI_USERLAND),y)
MOONLIGHT_QT_DEPENDENCIES += rpi-userland
endif

define MOONLIGHT_QT_SCRIPT_INSTALL
	$(INSTALL) -D -m 0755 $(MOONLIGHT_QT_PKGDIR)/scripts/Moonlight.sh $(TARGET_DIR)/recalbox/scripts/moonlight/Moonlight.sh
	$(INSTALL) -D -m 0755 $(MOONLIGHT_QT_PKGDIR)/scripts/moonlight.inc.sh $(TARGET_DIR)/recalbox/scripts/moonlight/moonlight.inc.sh
	$(INSTALL) -D -m 0755 $(MOONLIGHT_QT_PKGDIR)/scripts/generic.inc.sh $(TARGET_DIR)/recalbox/scripts/moonlight/generic.inc.sh
	touch $(TARGET_DIR)/recalbox/scripts/moonlight/is_qt
endef

MOONLIGHT_QT_POST_INSTALL_TARGET_HOOKS += MOONLIGHT_QT_SCRIPT_INSTALL

$(eval $(qmake-package))
