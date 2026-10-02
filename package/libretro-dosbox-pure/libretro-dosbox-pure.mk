################################################################################
#
# DOSBOX_PURE
#
################################################################################

# Commit of 2025/07/29
LIBRETRO_DOSBOX_PURE_VERSION = 1.0-preview2
LIBRETRO_DOSBOX_PURE_SITE = $(call github,schellingb,dosbox-pure,$(LIBRETRO_DOSBOX_PURE_VERSION))
LIBRETRO_DOSBOX_PURE_LICENSE = GPL-2.0
LIBRETRO_DOSBOX_PURE_LICENSE_FILES = LICENSE

ifeq ($(BR2_ARM64_PAGE_SIZE_16K),y)
LIBRETRO_DOSBOX_PURE_SUPP_OPT += PAGESIZE16K=1
endif

define LIBRETRO_DOSBOX_PURE_BUILD_CMDS
	$(SED) "s|-O2|-O3|g" $(@D)/Makefile
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO)" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/ -f Makefile platform="unix" $(LIBRETRO_DOSBOX_PURE_SUPP_OPT)
endef

define LIBRETRO_DOSBOX_PURE_INSTALL_TARGET_CMDS
	$(INSTALL) -D $(@D)/dosbox_pure_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/dosbox_pure_libretro.so
endef

$(eval $(generic-package))
