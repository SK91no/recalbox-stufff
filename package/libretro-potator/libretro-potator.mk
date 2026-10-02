################################################################################
#
# Watara Supervision
#
################################################################################

# Commit of 2023/05/28
LIBRETRO_POTATOR_VERSION = aed31f9254cada9826c65ff4528cc8bdda338275
LIBRETRO_POTATOR_SITE = $(call github,libretro,potator,$(LIBRETRO_POTATOR_VERSION))
LIBRETRO_POTATOR_LICENSE = Unlicense

define LIBRETRO_POTATOR_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CPPFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/platform/libretro -f Makefile platform="unix"
endef

define LIBRETRO_POTATOR_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/platform/libretro/potator_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/potator_libretro.so
endef

$(eval $(generic-package))
