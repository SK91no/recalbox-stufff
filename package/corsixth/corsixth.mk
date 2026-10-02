################################################################################
#
# CorsixTH
#
################################################################################

# Commit of 2025/02/21
CORSIXTH_VERSION = 82273234a003e8c2ebebd57f037136257a9a36fe
CORSIXTH_SITE = $(call github,CorsixTH,CorsixTH,$(CORSIXTH_VERSION))
CORSIXTH_LICENSE = GPL-2.1
CORSIXTH_LICENSE_FILES = License.txt
CORSIXTH_DEPENDENCIES = ffmpeg freetype lua luafilesystem lpeg luasocket luasec sdl2 sdl2_mixer pugixml libcurl mbedtls

CORSIXTH_CONF_OPTS += -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
CORSIXTH_CONF_OPTS += -DCMAKE_C_ARCHIVE_FINISH=true
CORSIXTH_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
CORSIXTH_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_FINISH=true
CORSIXTH_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
CORSIXTH_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
CORSIXTH_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
CORSIXTH_CONF_OPTS += -DCMAKE_LINKER="$(TARGET_LD)"
CORSIXTH_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_SO)"
CORSIXTH_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_SO)"
CORSIXTH_CONF_OPTS += -DUSE_SOURCE_DATADIRS=ON
CORSIXTH_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
CORSIXTH_CONF_OPTS += -Wno-dev

define CORSIXTH_INSTALL_TARGET_CMDS
	mkdir -p "$(TARGET_DIR)/recalbox/share_init/roms/ports/Theme Hospital"
	mkdir -p $(TARGET_DIR)/recalbox/share_init/saves/themehospital
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/configs/corsixth
	$(INSTALL) -m 0755 $(@D)/CorsixTH/corsix-th -D $(TARGET_DIR)/usr/bin
	cp -R $(@D)/CorsixTH/Bitmap "$(TARGET_DIR)/recalbox/share_init/roms/ports/Theme Hospital/"
	cp -R $(@D)/CorsixTH/Lua "$(TARGET_DIR)/recalbox/share_init/roms/ports/Theme Hospital/"
	cp -R $(@D)/CorsixTH/CorsixTH.lua "$(TARGET_DIR)/recalbox/share_init/roms/ports/Theme Hospital/"
endef

$(eval $(cmake-package))
