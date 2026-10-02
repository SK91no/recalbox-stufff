################################################################################
#
# lgpio
#
################################################################################

# Commit of 2024/03/29
LGPIO_VERSION = 746f0df43774175090b93abcc860b6733eefc09b
LGPIO_SITE = $(call github,joan2937,lg,$(LGPIO_VERSION))
LGPIO_LICENSE = unlicense.org
LGPIO_INSTALL_STAGING = YES

define LGPIO_BUILD_CMDS
	CFLAGS="$(TARGET_CFLAGS) $(COMPILER_COMMONS_CFLAGS_SO)" \
	CXXFLAGS="$(TARGET_CXXFLAGS) $(COMPILER_COMMONS_CFXXLAGS_SO)" \
	LDFLAGS="$(TARGET_LDFLAGS) $(COMPILER_COMMONS_LDFLAGS_SO)" \
	$(MAKE) CXX="$(TARGET_CXX)" \
		CC="$(TARGET_CC)" \
		AS="$(TARGET_AS)" \
		config=release \
		STRIP="$(TARGET_STRIP)" \
		-C $(@D) -f Makefile
endef

define LGPIO_INSTALL_STAGING_CMDS
	$(INSTALL) -D -m 0644 $(@D)/liblgpio.so.1 $(STAGING_DIR)/usr/lib/liblgpio.so.1
	ln -sf /usr/lib/liblgpio.so.1 $(STAGING_DIR)/usr/lib/liblgpio.so
endef

define LGPIO_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/liblgpio.so.1 $(TARGET_DIR)/usr/lib/liblgpio.so.1
	ln -sf /usr/lib/liblgpio.so.1 $(TARGET_DIR)/usr/lib/liblgpio.so
endef

$(eval $(generic-package))
