################################################################################
#
# LIBRETRO_AMIBERRY
#
################################################################################

# Commit 2026/06/19
LIBRETRO_AMIBERRY_VERSION = 5aa08227771d24d01d0e22c3db70d3cc08c4d1e6
LIBRETRO_AMIBERRY_SITE = https://github.com/BlitterStudio/amiberry.git
LIBRETRO_AMIBERRY_LICENSE = GPL-3.0
LIBRETRO_AMIBERRY_LICENSE_FILES = COPYING
# Non commercial because of libcapsimage dependency
LIBRETRO_AMIBERRY_NON_COMMERCIAL = y
LIBRETRO_AMIBERRY_SITE_METHOD=git
LIBRETRO_AMIBERRY_GIT_SUBMODULES = YES
LIBRETRO_AMIBERRY_DEPENDENCIES=zlib zstd host-libcurl

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
LIBRETRO_AMIBERRY_ARCH=aarch64
LIBRETRO_AMIBERRY_PLATFORM=rpi5_64
else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64),y)
LIBRETRO_AMIBERRY_ARCH=aarch64
LIBRETRO_AMIBERRY_PLATFORM=rpi4_64
else ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RG353X),y)
LIBRETRO_AMIBERRY_ARCH=aarch64
LIBRETRO_AMIBERRY_PLATFORM=rg353x
else
LIBRETRO_AMIBERRY_PLATFORM=$(RETROARCH_LIBRETRO_BOARD)
LIBRETRO_AMIBERRY_ARCH=$(BUILD_ARCH)
endif

LIBRETRO_AMIBERRY_NLOHMANN_JSON_VERSION = v3.11.3

define LIBRETRO_AMIBERRY_INSTALL_NLOHMANN_JSON
	$(SED) "s|curl|#curl|g"  $(@D)/libretro/Makefile
	mkdir -p $(@D)/libretro/deps/nlohmann
	$(HOST_DIR)/bin/curl -sfL -o $(@D)/libretro/deps/nlohmann/json.hpp https://github.com/nlohmann/json/releases/download/$(LIBRETRO_AMIBERRY_NLOHMANN_JSON_VERSION)/json.hpp
endef

define LIBRETRO_AMIBERRY_BUILD_CMDS
	cd $(@D)/libretro && \
	$(MAKE) CXX="$(TARGET_CXX)" \
		CC="$(TARGET_CC)" \
		LD="$(TARGET_LD)" \
		AS="$(TARGET_AS)" \
		STRIP="$(TARGET_STRIP)" \
		ARCH=$(LIBRETRO_AMIBERRY_ARCH) \
		system_board=$(LIBRETRO_AMIBERRY_PLATFORM)
endef

define LIBRETRO_AMIBERRY_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/libretro/amiberry_libretro.so $(TARGET_DIR)/usr/lib/libretro/amiberry_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl
	cp -R $(@D)/whdboot/* $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl/
	rm $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl/save-data/Savegames/foo.txt
	rm $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl/save-data/Kickstarts/foo.txt
	rm $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl/save-data/Debugs/foo.txt
	rm $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/whdl/save-data/Autoboots/foo.txt
	# Copy RTB files
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/bios/ && \
		cp $(@D)/whdboot/save-data/Kickstarts/*.RTB $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/bios/
endef

LIBRETRO_AMIBERRY_PRE_CONFIGURE_HOOKS += LIBRETRO_AMIBERRY_INSTALL_NLOHMANN_JSON

$(eval $(generic-package))
