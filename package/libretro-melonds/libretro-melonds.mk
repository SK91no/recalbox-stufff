################################################################################
#
# MELONDS
#
################################################################################

# Commit of 2024/10/21
LIBRETRO_MELONDS_VERSION = 7a3c11ff970cd36ca806961fae6db94b30dd5401
LIBRETRO_MELONDS_SITE = $(call github,libretro,melonds,$(LIBRETRO_MELONDS_VERSION))
LIBRETRO_MELONDS_LICENSE = GPLv3

LIBRETRO_MELONDS_DEPENDENCIES =

ifeq ($(BR2_arm)$(BR2_aarch64),y)
LIBRETRO_MELONDS_PIC = -fPIC
LIBRETRO_MELONDS_PLATFORM=linux-nogl
else
LIBRETRO_MELONDS_PLATFORM=unix
# unix platform uses HAVE_OPENGL -> desktop GL needed
LIBRETRO_MELONDS_DEPENDENCIES += libgl
endif

define LIBRETRO_MELONDS_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO) $(LIBRETRO_MELONDS_PIC)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) $(LIBRETRO_MELONDS_PIC)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO) $(LIBRETRO_MELONDS_PIC)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="$(LIBRETRO_MELONDS_PLATFORM)" \
		GIT_VERSION="-$(shell echo $(LIBRETRO_MELONDS_VERSION) | cut -c 1-8)"
endef

define LIBRETRO_MELONDS_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/melonds_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/melonds_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/nds
endef

$(eval $(generic-package))
