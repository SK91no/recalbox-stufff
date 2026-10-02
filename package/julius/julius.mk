################################################################################
#
# JULIUS
#
################################################################################

# Commit of 2025/01/09
JULIUS_VERSION = 33c777019b2640b847b73815c3b4344cf7bd9576
JULIUS_SITE = $(call github,bvschaik,julius,$(JULIUS_VERSION))
JULIUS_LICENSE = AGPL-3.0
JULIUS_LICENSE_FILES = LICENSE.txt
JULIUS_DEPENDENCIES = libpng sdl2 sdl2_mixer

JULIUS_CONF_OPTS += -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
JULIUS_CONF_OPTS += -DCMAKE_C_ARCHIVE_FINISH=true
JULIUS_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
JULIUS_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_FINISH=true
JULIUS_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
JULIUS_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
JULIUS_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"
JULIUS_CONF_OPTS += -DCMAKE_LINKER="$(TARGET_LD)"
JULIUS_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_SO)"
JULIUS_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_SO)"
JULIUS_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release

define JULIUS_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/share_init/roms/ports/Caesar3
	mkdir -p $(TARGET_DIR)/recalbox/share_init/system/configs/julius

	$(INSTALL) -m 0755 $(@D)/julius -D $(TARGET_DIR)/usr/bin
endef

$(eval $(cmake-package))
