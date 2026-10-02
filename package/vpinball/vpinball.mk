################################################################################
#
# vpinball
#
################################################################################

# Commit of 2026/07/15. When bumping, resync every dependency below with the
# matching SHA in the tree's platforms/config.sh -- that file is the reference,
# and letting a package drift from it silently breaks features (a libpinmame
# without PinmameSetMsgAPI builds fine but never renders a DMD).
VPINBALL_VERSION = 23b022797c12f89a4223c1515ff8e5282031ceb4
VPINBALL_SITE = $(call github,vpinball,vpinball,$(VPINBALL_VERSION))
VPINBALL_LICENSE = GPLv3+
VPINBALL_LICENSE_FILES = LICENSE
VPINBALL_DEPENDENCIES = libfreeimage libpinmame libaltsound libdmdutil libdof ffmpeg sdl3 sdl3_image sdl3_ttf sdl3_mixer libwinevbs
VPINBALL_SUPPORTS_IN_SOURCE_BUILD = NO

# Upstream dropped the per-target make/CMakeLists_<renderer>-<platform>-<arch>.txt
# files for a single root CMakeLists.txt driven by RENDERER/PLATFORM/ARCH, so the
# file no longer needs to be copied over before configuring.
# RENDERER: BGFX (upstream default), GL or DX9. Ancillary windows (a separate
# backglass/DMD window, hence multi screen) are BGFX only -- the GL build simply
# renders nothing for them and can only embed them in the playfield.
ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_X86_64),y)
    VPINBALL_ARCH = x64
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
    VPINBALL_ARCH = aarch64
    VPINBALL_CONF_OPTS += -DBUILD_RPI=ON
endif

VPINBALL_CONF_OPTS += -DRENDERER=GL
VPINBALL_CONF_OPTS += -DPLATFORM=linux
VPINBALL_CONF_OPTS += -DARCH=$(VPINBALL_ARCH)

# Link against buildroot's staging tree rather than the bundled third-party one.
# This replaces the former 004-libwinevbs-staging-paths patch.
VPINBALL_CONF_OPTS += -DUSE_SYSTEM_LIBS=ON
VPINBALL_CONF_OPTS += -DSYSTEM_LIBS_INCLUDE_DIR=$(STAGING_DIR)/usr/include
VPINBALL_CONF_OPTS += -DSYSTEM_LIBS_LIB_DIR=$(STAGING_DIR)/usr/lib

VPINBALL_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
VPINBALL_CONF_OPTS += -DBUILD_SHARED_LIBS=OFF
VPINBALL_CONF_OPTS += -DPOST_BUILD_COPY_EXT_LIBS=OFF

define VPINBALL_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/bin/vpinball
	# install binary. Named after the renderer only (VPinballX_GL / VPinballX_BGFX)
	# on linux, whatever the arch -- the '64' suffix upstream appends on x64 is a
	# windows convention, see RUNTIME_OUTPUT_NAME in the linux branch of CMakeLists.
	$(INSTALL) -D -m 0755 $(@D)/buildroot-build/VPinballX_GL \
	    $(TARGET_DIR)/usr/bin/vpinball/VPinballX_GL
	# copy folders
	cp -R $(@D)/buildroot-build/plugins $(TARGET_DIR)/usr/bin/vpinball/
	cp -R $(@D)/buildroot-build/assets $(TARGET_DIR)/usr/bin/vpinball/
	cp -R $(@D)/buildroot-build/scripts $(TARGET_DIR)/usr/bin/vpinball/
	# glob: the folder carries the VPX version (shaders-<major>.<minor>.<rev>, see
	# SHADER_DIR in CMakeLists), which VPX looks up under that exact name at
	# runtime. Copying it as-is keeps the name and survives a version bump.
	cp -R $(@D)/buildroot-build/shaders-* $(TARGET_DIR)/usr/bin/vpinball/
	# install default ini for configgen
	$(INSTALL) -D -m 0644 $(VPINBALL_PKGDIR)/VPinballX.ini \
	    $(TARGET_DIR)/usr/bin/vpinball/assets/Default_VPinballX.ini
endef



$(eval $(cmake-package))
