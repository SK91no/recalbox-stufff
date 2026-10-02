################################################################################
#
# FLYCAST Next
#
################################################################################

# Commit of 05/07/2026
LIBRETRO_FLYCAST_NEXT_VERSION = 1607eb5715e92296b181981d3ae87a29fccd75ab
LIBRETRO_FLYCAST_NEXT_SITE = http://github.com/flyinghead/flycast.git
LIBRETRO_FLYCAST_NEXT_LICENSE = GPL-2.0
LIBRETRO_FLYCAST_NEXT_SITE_METHOD = git
LIBRETRO_FLYCAST_NEXT_GIT_SUBMODULES = y

LIBRETRO_FLYCAST_NEXT_DEPENDENCIES = arcade-dats
LIBRETRO_FLYCAST_NEXT_CONF_OPTS = -DLIBRETRO=ON -DBUILD_SHARED_LIBS=OFF

LIBRETRO_FLYCAST_NEXT_GIT_TAG = \
	$(shell $(GIT) -C $(LIBRETRO_FLYCAST_NEXT_DL_DIR)/git describe --tags --always | tr -d '\n')
LIBRETRO_FLYCAST_NEXT_GIT_HASH = \
	$(shell $(GIT) -C $(LIBRETRO_FLYCAST_NEXT_DL_DIR)/git rev-parse --short HEAD | tr -d '\n')
LIBRETRO_FLYCAST_NEXT_CONF_OPTS += -DGIT_VERSION=$(LIBRETRO_FLYCAST_NEXT_GIT_TAG)
LIBRETRO_FLYCAST_NEXT_CONF_OPTS += -DGIT_HASH=$(LIBRETRO_FLYCAST_NEXT_GIT_HASH)

ifeq ($(BR2_PACKAGE_RECALBOX_HAS_VULKAN),y)
# vulkan support is enabled by default
LIBRETRO_FLYCAST_NEXT_DEPENDENCIES += vulkan-headers
endif

ifeq ($(BR2_PACKAGE_RECALBOX_HAS_LIBGLES),y)
LIBRETRO_FLYCAST_NEXT_DEPENDENCIES += libgles
ifeq ($(BR2_PACKAGE_RPI_USERLAND),y)
LIBRETRO_FLYCAST_NEXT_CONF_OPTS += \
	-DUSE_GLES=OFF \
	-DUSE_GLES2=ON \
	-DUSE_VIDEOCORE=ON
else
LIBRETRO_FLYCAST_NEXT_CONF_OPTS += -DUSE_GLES=ON
endif
endif

ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
LIBRETRO_FLYCAST_NEXT_DEPENDENCIES += libgl
endif

ifeq ($(BR2_PACKAGE_RECALBOX_HAS_LIBMALI),y)
LIBRETRO_FLYCAST_NEXT_SIDE_LIBS = -lmali
endif

ifeq ($(BR2_ARM64_PAGE_SIZE_16K),y)
LIBRETRO_FLYCAST_NEXT_CONF_OPTS += -DPAGESIZE16K=ON
endif

LIBRETRO_FLYCAST_NEXT_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_SO) $(LIBRETRO_FLYCAST_NEXT_SIDE_LIBS)"

define LIBRETRO_FLYCAST_NEXT_INSTALL_TARGET_CMDS
	$(call InstallArcadeFiles,libretro,naomi,$(LIBRETRO_FLYCAST_NEXT_VERSION),naomi-next)
	$(call InstallArcadeFiles,libretro,naomi2,$(LIBRETRO_FLYCAST_NEXT_VERSION),naomi2-next)
	$(call InstallArcadeFiles,libretro,naomigd,$(LIBRETRO_FLYCAST_NEXT_VERSION),naomigd-next)
	$(call InstallArcadeFiles,libretro,atomiswave,$(LIBRETRO_FLYCAST_NEXT_VERSION),atomiswave-next)

	$(INSTALL) -D $(@D)/flycast_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/flycast-next_libretro.so
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/dc
endef

$(eval $(cmake-package))
