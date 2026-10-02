################################################################################
#
# Beebem
#
################################################################################

# Commit of 2024/10/02
BEEBEM_VERSION = 43e8152aff9c76e49c1123897efd1cfe7ff933a4
BEEBEM_SITE = $(call gitlab,recalbox,packages/standalone/beebem-recalbox,$(BEEBEM_VERSION))
BEEBEM_LICENSE = GPL
BEEBEM_DEPENDENCIES = sdl2 sdl2_ttf zlib libfreeimage libzip libcurl

ifeq ($(BR2_PACKAGE_HAS_LIBGL),y)
BEEBEM_DEPENDENCIES += libgl
endif

ifeq ($(BR2_PACKAGE_HAS_LIBGLES),y)
BEEBEM_DEPENDENCIES += libgles
endif

BEEBEM_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
BEEBEM_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
BEEBEM_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
BEEBEM_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
BEEBEM_CONF_OPTS += -DCMAKE_LINKER="$(TARGET_LD)"
# LTO (-flto -fwhole-program) breaks the link on 32-bit ARM (lto-wrapper ltrans failure), so build without LTO
BEEBEM_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_NOLTO)"
BEEBEM_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_NOLTO)"
BEEBEM_CONF_OPTS += -DCMAKE_EXE_LINKER_FLAGS="$(COMPILER_COMMONS_LDFLAGS_NOLTO)"

$(eval $(cmake-package))
