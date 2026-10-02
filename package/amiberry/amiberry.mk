################################################################################
#
# amiberry
#
################################################################################

# Commit of 2026/19/06
AMIBERRY_VERSION = 5aa08227771d24d01d0e22c3db70d3cc08c4d1e6
AMIBERRY_SITE = $(call github,BlitterStudio,amiberry,$(AMIBERRY_VERSION))
AMIBERRY_LICENSE = GPL-3.0
AMIBERRY_LICENSE_FILES = LICENSE
# Non commercial because of libcapsimage dependency
AMIBERRY_NON_COMMERCIAL = y
AMIBERRY_DEPENDENCIES = sdl3 sdl3_image flac mpg123 libpng zlib libcurl \
	json-for-modern-cpp libmpeg2 zstd libcapsimage libxml2

ifeq ($(BR2_PACKAGE_HAS_LIBGLES),y)
AMIBERRY_DEPENDENCIES += libgles
AMIBERRY_CONF_OPTS += -DUSE_GLES=ON
else
AMIBERRY_DEPENDENCIES += libgl
AMIBERRY_CONF_OPTS += -DUSE_GLES=OFF
endif

# Amiberry's CMakeLists rejects in-tree builds, so use a separate build dir
AMIBERRY_SUPPORTS_IN_SOURCE_BUILD = NO

# Toolchain
AMIBERRY_CONF_OPTS += -DCMAKE_BUILD_TYPE=Release
AMIBERRY_CONF_OPTS += -DCMAKE_C_COMPILER="$(TARGET_CC)"
AMIBERRY_CONF_OPTS += -DCMAKE_CXX_COMPILER="$(TARGET_CXX)"

# LTO is on (-flto=auto, no fat objects), so the bundled imgui static lib holds
# LTO objects. The plain ar leaves them out of the archive index and the final
# link fails with undefined ImGui:: refs. Use the LTO-aware gcc-ar wrapper.
AMIBERRY_CONF_OPTS += -DCMAKE_AR="$(TARGET_CC)-ar"
AMIBERRY_CONF_OPTS += -DCMAKE_C_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
AMIBERRY_CONF_OPTS += -DCMAKE_C_ARCHIVE_FINISH=true
AMIBERRY_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_CREATE="<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>"
AMIBERRY_CONF_OPTS += -DCMAKE_CXX_ARCHIVE_FINISH=true
AMIBERRY_CONF_OPTS += -DCMAKE_C_FLAGS="$(COMPILER_COMMONS_CFLAGS_EXE)"
AMIBERRY_CONF_OPTS += -DCMAKE_CXX_FLAGS="$(COMPILER_COMMONS_CXXFLAGS_EXE)"
AMIBERRY_CONF_OPTS += -DCMAKE_EXE_LINKER_FLAGS="$(COMPILER_COMMONS_LDFLAGS_EXE)"

# Rendering: OpenGL (ES), no Vulkan
AMIBERRY_CONF_OPTS += -DUSE_OPENGL=ON
AMIBERRY_CONF_OPTS += -DUSE_VULKAN=OFF
AMIBERRY_CONF_OPTS += -DUSE_JIT=ON

# CD32 FMV / audio
AMIBERRY_CONF_OPTS += -DUSE_LIBMPEG2=ON
AMIBERRY_CONF_OPTS += -DUSE_MPG123=ON
AMIBERRY_CONF_OPTS += -DUSE_ZSTD=ON

# Features not shipped on Recalbox
AMIBERRY_CONF_OPTS += -DUSE_PPC=OFF
AMIBERRY_CONF_OPTS += -DUSE_QEMU_PPC=OFF
AMIBERRY_CONF_OPTS += -DUSE_PCEM=OFF
AMIBERRY_CONF_OPTS += -DUSE_PORTMIDI=OFF
AMIBERRY_CONF_OPTS += -DUSE_LIBSERIALPORT=OFF
AMIBERRY_CONF_OPTS += -DUSE_LIBENET=OFF
AMIBERRY_CONF_OPTS += -DUSE_UAENET_PCAP=OFF
AMIBERRY_CONF_OPTS += -DUSE_UAENET_TAP=OFF
AMIBERRY_CONF_OPTS += -DUSE_DBUS=OFF
AMIBERRY_CONF_OPTS += -DUSE_GPIOD=OFF
AMIBERRY_CONF_OPTS += -DUSE_IPC_SOCKET=OFF

# CMake only installs the binary (into /usr/bin via cmake-package). The data
# and whdboot trees are shipped manually below.
define AMIBERRY_INSTALL_DATA
	mkdir -p $(TARGET_DIR)/usr/share/amiberry
	cp -R $(@D)/data $(TARGET_DIR)/usr/share/amiberry
	cp -R $(@D)/whdboot $(TARGET_DIR)/usr/share/amiberry
	# Copy RTB kickstart replacement files to the bios upgrade folder
	mkdir -p $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/bios/ && \
		cp $(@D)/whdboot/save-data/Kickstarts/*.RTB $(TARGET_DIR)/recalbox/share_upgrade/bios/amiga/bios/
endef
AMIBERRY_POST_INSTALL_TARGET_HOOKS += AMIBERRY_INSTALL_DATA

$(eval $(cmake-package))
