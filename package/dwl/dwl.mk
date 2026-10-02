################################################################################
#
# DWL
#
################################################################################

# Commit of 2023/11/25
DWL_VERSION = v0.5
DWL_SITE = https://codeberg.org/dwl/dwl
DWL_SITE_METHOD = git
DWL_DEPENDENCIES = wayland wayland-protocols wlroots libinput libxkbcommon libxcb
DWL_LICENSE = GPL3
DWL_LICENSE_FILES = LICENSE

define DWL_BUILD_CMDS
	cd $(@D) && \
	PKG_CONFIG_SYSROOT_DIR="$(STAGING_DIR)" \
	PKG_CONFIG_PATH="$(HOST_DIR)/lib/pkgconfig:$(STAGING_DIR)/usr/share/pkgconfig/:$(STAGING_DIR)/usr/lib/pkgconfig/" \
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_NOLTO) -I$(STAGING_DIR)/usr/include/pixman-1/ -I$(STAGING_DIR)/usr/include/glib-2.0/ -I$(STAGING_DIR)/usr/lib/glib-2.0/include/ -I$(TARGET_DIR)/usr/include" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_NOLTO)" \
		$(MAKE) CC="$(TARGET_CC)" XWAYLAND="-DXWAYLAND" XLIBS="xcb xcb-icccm" -C $(@D)/ -f Makefile
endef

define DWL_INSTALL_TARGET_CMDS
	$(INSTALL) -m 755 -D "$(@D)/dwl" "$(TARGET_DIR)/usr/bin/dwl"
endef

$(eval $(generic-package))
