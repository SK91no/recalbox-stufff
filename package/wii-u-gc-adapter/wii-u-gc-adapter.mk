################################################################################
#
# wii-u-gc-adapter
#
################################################################################

# Commit of 2021/09/06
WII_U_GC_ADAPTER_VERSION = fa098efa7f6b34f8cd82e2c249c81c629901976c
WII_U_GC_ADAPTER_SITE = $(call github,ToadKing,wii-u-gc-adapter,$(WII_U_GC_ADAPTER_VERSION))
WII_U_GC_ADAPTER_LICENSE = GPL-2.0
WII_U_GC_ADAPTER_LICENSE_FILES = LICENSE
WII_U_GC_ADAPTER_DEPENDENCIES = libusb udev host-pkgconf

define WII_U_GC_ADAPTER_BUILD_CMDS
	$(TARGET_CONFIGURE_OPTS) $(MAKE) CC="$(TARGET_CC)" -C $(@D)
endef

define WII_U_GC_ADAPTER_COPY_CMDS
	$(INSTALL) -D -m 0755 -t $(TARGET_DIR)/usr/bin $(@D)/wii-u-gc-adapter
	$(INSTALL) -D -m 0755 -t $(TARGET_DIR)/usr/bin $(BR2_EXTERNAL_RECALBOX_PATH)/package/wii-u-gc-adapter/wii-u-gc-adapter-daemon
	$(INSTALL) -D -m 0644 -t $(TARGET_DIR)/etc/udev/rules.d $(BR2_EXTERNAL_RECALBOX_PATH)/package/wii-u-gc-adapter/99-wii-u-gc-adapter.rules
endef

WII_U_GC_ADAPTER_INSTALL_TARGET_CMDS += $(WII_U_GC_ADAPTER_COPY_CMDS)

$(eval $(generic-package))
