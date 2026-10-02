################################################################################
#
# VITAQUAKE3
#
################################################################################

# Commit of 2026/02/05
LIBRETRO_VITAQUAKE3_VERSION = 9ce48083bb8f62f6168fe5be82fb92c4d6884bc0
LIBRETRO_VITAQUAKE3_SITE = $(call gitlab,Pit64,vitaquake3,$(LIBRETRO_VITAQUAKE3_VERSION))
LIBRETRO_VITAQUAKE3_LICENSE = GPL-2.0
LIBRETRO_VITAQUAKE3_LICENSE_FILES = COPYING.txt

# Required from GCC14
LIBRETRO_VITAQUAKE3_GCC14_OPTIONS = -Wno-implicit-function-declaration -Wno-incompatible-pointer-types -Wno-implicit-int

LIBRETRO_VITAQUAKE3_DEPENDENCIES =

ifeq ($(BR2_PACKAGE_HAS_LIBGLES),y)
LIBRETRO_VITAQUAKE3_DEPENDENCIES += libgles
endif

ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
LIBRETRO_VITAQUAKE3_DEPENDENCIES += libgl
endif

define LIBRETRO_VITAQUAKE3_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO) $(LIBRETRO_VITAQUAKE3_GCC14_OPTIONS)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) $(LIBRETRO_VITAQUAKE3_GCC14_OPTIONS)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D) -f Makefile platform="$(RETROARCH_LIBRETRO_PLATFORM)"
endef

define LIBRETRO_VITAQUAKE3_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/vitaquake3_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/vitaquake3_libretro.so
endef

$(eval $(generic-package))
