################################################################################
#
# Supermodel
#
################################################################################

# Commit of 2025/05/28
# NOTE: after bumping this version, regenerate the PGO profile (pgo/*.gcda) by running
#       package/supermodel/pgo-regen.sh  (else PGO silently degrades on the changed files).
SUPERMODEL_VERSION = 487d642a808273f99611bcee9badeddc317849f0
SUPERMODEL_SITE = https://github.com/trzy/Supermodel.git
SUPERMODEL_SITE_METHOD = git
SUPERMODEL_LICENSE = GPL2
SUPERMODEL_LICENSE_FILES = Docs/LICENSE.txt
SUPERMODEL_DEPENDENCIES = zlib libpng libogg libvorbis sdl2_net sdl2 libglu arcade-dats

# Perf: LTO + PGO (profile stored in pgo/) + extra flags. See PERF.md (this directory).
SUPERMODEL_EXTRA_FLAGS = -funroll-loops -fno-semantic-interposition -fipa-pta

# Two-pass PGO. Default "use" = production build (consumes pgo/*.gcda).
# "generate" (driven by package/supermodel/pgo-regen.sh) = instrumented build to re-record the profile.
SUPERMODEL_PGO_MODE ?= use
ifeq ($(SUPERMODEL_PGO_MODE),generate)
SUPERMODEL_PGO_FLAGS = -fprofile-generate
SUPERMODEL_PGO_INJECT = true
else
SUPERMODEL_PGO_FLAGS = -fprofile-use -fprofile-correction -Wno-missing-profile -Wno-error=coverage-mismatch -flto=auto -fuse-linker-plugin
SUPERMODEL_PGO_INJECT = mkdir -p $(@D)/obj && cp $(SUPERMODEL_PKGDIR)/pgo/*.gcda $(@D)/obj/
endif

define SUPERMODEL_BUILD_CMDS
	$(call InstallArcadeFiles,supermodel,supermodel,$(SUPERMODEL_VERSION))
	cp $(@D)/Makefiles/Makefile.UNIX $(@D)/Makefile
	$(SED) "s|CC = gcc|CC = $(TARGET_CC) $(COMPILER_COMMONS_CFLAGS_NOLTO) $(SUPERMODEL_PGO_FLAGS) $(SUPERMODEL_EXTRA_FLAGS)|g" $(@D)/Makefile
	$(SED) "s|CXX = g++|CXX = $(TARGET_CXX) $(COMPILER_COMMONS_CXXFLAGS_NOLTO) $(SUPERMODEL_PGO_FLAGS) $(SUPERMODEL_EXTRA_FLAGS)|g" $(@D)/Makefile
	$(SED) "s|LD = gcc|LD = $(TARGET_CC) $(COMPILER_COMMONS_LDFLAGS_NOLTO) $(SUPERMODEL_PGO_FLAGS) $(SUPERMODEL_EXTRA_FLAGS)|g" $(@D)/Makefile
	$(SED) "s|sdl2-config|$(STAGING_DIR)/usr/bin/sdl2-config|g" $(@D)/Makefile
	# PGO (use mode): inject pre-recorded profile next to objects; degrade gracefully on drift.
	$(SUPERMODEL_PGO_INJECT)
	$(TARGET_CONFIGURE_OPTS) $(MAKE) -C $(@D) -f Makefile \
	NET_BOARD=1 DEBUG=0 NEW_FRAME_TIMING=1 ENABLE_DEBUGGER=0
endef

define SUPERMODEL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/bin/supermodel \
		$(TARGET_DIR)/usr/bin/supermodel
	$(INSTALL) -D -m 0644 $(@D)/Config/Games.xml \
		$(TARGET_DIR)/recalbox/share_init/system/configs/model3/Games.xml
	mkdir -p $(TARGET_DIR)/usr/share/supermodel/Assets
	$(INSTALL) -D -m 0644 $(@D)/Assets/* \
		$(TARGET_DIR)/usr/share/supermodel/Assets/
endef

define SUPERMODEL_LINE_ENDINGS_FIXUP
	# DOS2UNIX Supermodel.ini and Main.cpp - patch system does not support different line endings
	sed -i -E -e "s|\r$$||g" $(@D)/Src/OSD/SDL/Main.cpp
	sed -i -E -e "s|\r$$||g" $(@D)/Src/Inputs/Inputs.cpp
	sed -i -E -e "s|\r$$||g" $(@D)/Src/Graphics/New3D/R3DShaderTriangles.h
endef

# SUPERMODEL_PRE_PATCH_HOOKS += SUPERMODEL_LINE_ENDINGS_FIXUP

$(eval $(generic-package))
