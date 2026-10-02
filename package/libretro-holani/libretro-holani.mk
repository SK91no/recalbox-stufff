################################################################################
#
# libretro-holani
#
################################################################################

# Commit of 2024/12/31
LIBRETRO_HOLANI_VERSION = 19b974f77247a937369adf48280d2b34a5a66245
LIBRETRO_HOLANI_SITE = $(call github,lleny,holani-retro,$(LIBRETRO_HOLANI_VERSION))
LIBRETRO_HOLANI_LICENSE = GPL-3
LIBRETRO_HOLANI_LICENSE_FILES = LICENSE
LIBRETRO_HOLANI_DEPENDENCIES = host-clang retroarch

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI3)$(BR2_PACKAGE_RECALBOX_TARGET_RPIZERO2),y)
LIBRETRO_HOLANI_FOLDER = armv7-unknown-linux-gnueabihf
else
LIBRETRO_HOLANI_FOLDER = $(ARCH)-unknown-linux-gnu
endif

define LIBRETRO_HOLANI_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/usr/lib/libretro
	cp $(@D)/target/$(LIBRETRO_HOLANI_FOLDER)/release/libholani.so \
		$(TARGET_DIR)/usr/lib/libretro/holani_libretro.so
endef

$(eval $(cargo-package))
