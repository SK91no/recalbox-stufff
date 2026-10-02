################################################################################
#
# MESEN
#
################################################################################

# Commit of 2023/05/21
LIBRETRO_MESEN_VERSION = d25d60fc190f3f7603a1113ef1e11d9da65b7583
LIBRETRO_MESEN_SITE = $(call github,libretro,Mesen,$(LIBRETRO_MESEN_VERSION))
LIBRETRO_MESEN_LICENSE = GPL-3.0
LIBRETRO_MESEN_LICENSE_FILES = LICENSE

define LIBRETRO_MESEN_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Libretro/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/Libretro -f Makefile platform="unix"
endef

define LIBRETRO_MESEN_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/Libretro/mesen_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/mesen_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/fds
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/HdPacks
endef

$(eval $(generic-package))
