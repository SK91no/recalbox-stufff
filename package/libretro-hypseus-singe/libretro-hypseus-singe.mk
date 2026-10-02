################################################################################
#
# libretro-hypseus-singe
#
################################################################################

LIBRETRO_HYPSEUS_SINGE_VERSION = dcbc554f041de0c38331af3ebf6e09e02b26646d
LIBRETRO_HYPSEUS_SINGE_SITE = $(call github,beudbeud,hypseus-singe,$(LIBRETRO_HYPSEUS_SINGE_VERSION))
LIBRETRO_HYPSEUS_SINGE_LICENSE = MIT
LIBRETRO_HYPSEUS_SINGE_DEPENDENCIES = sdl2 sdl2_image sdl2_ttf sdl2_mixer zlib libzip libogg libvorbis libmpeg2

LIBRETRO_HYPSEUS_SINGE_SUBDIR = src
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS = -DBUILD_SHARED_LIBS=OFF -DLIBRETRO=ON -DABSTRACT_SINGE=ON -DBUILD_SINGE=ON -DCMAKE_BUILD_TYPE=Release

LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_C_ARCHIVE_FINISH=true
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_FINISH=true
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_LINKER="$(TARGET_LD)"
# FindVorbis.cmake lists libvorbis.a first, so CMake picks the non-PIC static
# archive and the link into the shared core fails (recompile with -fPIC).
# Force the shared libs: find_library skips its search when the cache var is set.
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DVorbis_Vorbis_LIBRARY=$(STAGING_DIR)/usr/lib/libvorbis.so
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DVorbis_Enc_LIBRARY=$(STAGING_DIR)/usr/lib/libvorbisenc.so
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DVorbis_File_LIBRARY=$(STAGING_DIR)/usr/lib/libvorbisfile.so
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DOGG_LIBRARY=$(STAGING_DIR)/usr/lib/libogg.so

# Core is a shared lib: use _SO flags (no -fwhole-program, which would
# localize the retro_* entry points and let --gc-sections strip them).
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_SO)"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_SO)"
LIBRETRO_HYPSEUS_SINGE_CONF_OPTS += -DCMAKE_LINKER_EXE_FLAGS="$(COMPILER_COMMONS_LDFLAGS_SO)"

define LIBRETRO_HYPSEUS_SINGE_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/src/hypseus_singe_libretro.so \
		$(TARGET_DIR)/usr/lib/libretro/hypseus_singe_libretro.so
	mkdir -p $(TARGET_DIR)/usr/share/hypseus
	cp -r $(@D)/fonts $(@D)/pics $(@D)/sound $(TARGET_DIR)/usr/share/hypseus
endef

$(eval $(cmake-package))
