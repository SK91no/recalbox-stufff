################################################################################
#
# VECX
#
################################################################################

# Commit of 2023/06/01
LIBRETRO_VECX_VERSION = a401c268e425dc8ae6a301e7fdb9a9e96f39b8ea
LIBRETRO_VECX_SITE = $(call github,libretro,libretro-vecx,$(LIBRETRO_VECX_VERSION))
LIBRETRO_VECX_LICENSE = GPL-3.0
LIBRETRO_VECX_LICENSE_FILES = LICENSE.md
LIBRETRO_VECX_DEPENDENCIES =

ifeq ($(BR2_x86_64)$(BR2_i386),y)
LIBRETRO_VECX_GLES = 0
LIBRETRO_VECX_DEPENDENCIES += libgl
else
LIBRETRO_VECX_GLES = 1
LIBRETRO_VECX_DEPENDENCIES += libgles
endif

define LIBRETRO_VECX_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile.libretro platform="unix" HAS_GLES=$(LIBRETRO_VECX_GLES)
endef

define LIBRETRO_VECX_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/vecx_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/vecx_libretro.so
endef

$(eval $(generic-package))
