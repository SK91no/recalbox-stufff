################################################################################
#
# SAMEDUCK
#
################################################################################

# Commit of 2022/07/03 (https://github.com/LIJI32/SameBoy/commits/SameDuck)
LIBRETRO_SAMEDUCK_VERSION = 5cc014b23af963f9e9a77091a1e6ff7a7852e8a7
LIBRETRO_SAMEDUCK_SITE = $(call github,LIJI32,SameBoy,$(LIBRETRO_SAMEDUCK_VERSION))
LIBRETRO_SAMEDUCK_LICENSE = MIT
LIBRETRO_SAMEDUCK_LICENSE_FILES = LICENSE

define LIBRETRO_SAMEDUCK_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/libretro -f Makefile platform="unix"
endef

define LIBRETRO_SAMEDUCK_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/build/bin/sameduck_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/sameduck_libretro.so
endef

$(eval $(generic-package))
