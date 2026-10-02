################################################################################
#
# GEARGRAFX
#
################################################################################

# Commit of 2025/07/07
LIBRETRO_GEARGRAFX_VERSION = eaf14d7efcd8e9f558a11d63c3857ee7e0557ea0
LIBRETRO_GEARGRAFX_SITE = $(call github,drhelius,Geargrafx,$(LIBRETRO_GEARGRAFX_VERSION))
LIBRETRO_GEARGRAFX_LICENSE = GPL-3.0
LIBRETRO_GEARGRAFX_LICENSE_FILES = LICENSE
LIBRETRO_GEARGRAFX_NON_COMMERCIAL = NO

define LIBRETRO_GEARGRAFX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/platforms/libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/platforms/libretro -f Makefile platform="unix"
endef

define LIBRETRO_GEARGRAFX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/platforms/libretro/geargrafx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/geargrafx_libretro.so
endef

$(eval $(generic-package))
