################################################################################
#
# RECALBOX_RGB_JAMMA
#
################################################################################

RECALBOX_RGB_JAMMA_VERSION = custom
RECALBOX_RGB_JAMMA_SITE = $(TOPDIR)/../projects/recalbox-rgb-jamma
RECALBOX_RGB_JAMMA_SITE_METHOD = local
RECALBOX_RGB_JAMMA_LICENSE = GPL
RECALBOX_RGB_JAMMA_DEPENDENCIES = linux recalbox-rgb-dual mosquitto libdrm

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI5_64),y)
RECALBOX_RGB_JAMMA_EXTRA_ARGS=ARCH_RPI5=1
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI4_64),y)
RECALBOX_RGB_JAMMA_EXTRA_ARGS=ARCH_RPI4=1
endif

ifeq ($(BR2_PACKAGE_RECALBOX_TARGET_RPI3),y)
RECALBOX_RGB_JAMMA_EXTRA_ARGS=ARCH_RPI3=1
endif

define RECALBOX_RGB_JAMMA_BUILD_CMDS
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalboxrgbjamma-overlay.dts -o $(@D)/recalboxrgbjamma.dtbo
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalboxrgbjamma-pi5-overlay.dts -o $(@D)/recalboxrgbjamma-pi5.dtbo
	$(TARGET_CPP) -nostdinc -undef -x assembler-with-cpp -I$(LINUX_DIR)/include/ $(@D)/recalboxrgbjamma2-pi5-overlay.dts $(@D)/recalboxrgbjamma2-pi5-overlay.dtspp
	$(HOST_DIR)/bin/linux-dtc $(@D)/recalboxrgbjamma2-pi5-overlay.dtspp -o $(@D)/recalboxrgbjamma2-pi5.dtbo
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" CROSS_DIR="$(STAGING_DIR)" \
		CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CXXFLAGS_SO) -std=c++11" \
		LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
		$(MAKE) CXX="$(TARGET_CXX)" CC="$(TARGET_CC)" -C $(@D)/screen -f Makefile $(RECALBOX_RGB_JAMMA_EXTRA_ARGS)
endef

define RECALBOX_RGB_JAMMA_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0644 $(@D)/recalboxrgbjamma.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalboxrgbjamma.dtbo
	# Working overlay for runtime dtoverlay loading
	$(INSTALL) -D -m 0644 $(@D)/recalboxrgbjamma-pi5.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalboxrgbjamma-pi5.dtbo
	$(INSTALL) -D -m 0644 $(@D)/recalboxrgbjamma2-pi5.dtbo $(BINARIES_DIR)/rpi-firmware/overlays/recalboxrgbjamma2-pi5.dtbo
	$(INSTALL) -D -m 0744 $(RECALBOX_RGB_JAMMA_PKGDIR)/S04rrgbjamma $(TARGET_DIR)/etc/init.d/S04rrgbjamma
	$(INSTALL) -D -m 0744 $(@D)/screen/recalboxrgbjamma_screen $(TARGET_DIR)/usr/bin/recalboxrgbjamma_screen

endef

$(eval $(kernel-module))
$(eval $(generic-package))
