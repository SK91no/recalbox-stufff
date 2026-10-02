################################################################################
#
# recalbox-luajit
#
################################################################################

# Commit of 2025/01/17
RECALBOX_LUAJIT_VERSION = 93162f34e7424cd0ea3c4046a9ffacce621626bc
RECALBOX_LUAJIT_SITE = $(call github,openresty,luajit2,$(RECALBOX_LUAJIT_VERSION))
RECALBOX_LUAJIT_LICENSE = MIT
RECALBOX_LUAJIT_LICENSE_FILES = COPYRIGHT
RECALBOX_LUAJIT_CPE_ID_VENDOR = luajit

RECALBOX_LUAJIT_INSTALL_STAGING = YES

ifeq ($(BR2_PACKAGE_RECALBOX_LUAJIT_COMPAT52),y)
RECALBOX_LUAJIT_XCFLAGS += -DLUAJIT_ENABLE_LUA52COMPAT
endif

# The luajit build procedure requires the host compiler to have the
# same bitness as the target compiler. Therefore, on a x86 build
# machine, we can't build luajit for x86_64, which is checked in
# Config.in. When the target is a 32 bits target, we pass -m32 to
# ensure that even on 64 bits build machines, a compiler of the same
# bitness is used. Of course, this assumes that the 32 bits multilib
# libraries are installed.
ifeq ($(BR2_ARCH_IS_64),y)
RECALBOX_LUAJIT_HOST_CC = $(HOSTCC)
# There is no LUAJIT_ENABLE_GC64 option.
else
RECALBOX_LUAJIT_HOST_CC = $(HOSTCC) -m32
RECALBOX_LUAJIT_XCFLAGS += -DLUAJIT_DISABLE_GC64
endif

# We unfortunately can't use TARGET_CONFIGURE_OPTS, because the luajit
# build system uses non conventional variable names.
define RECALBOX_LUAJIT_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX="/usr" \
		STATIC_CC="$(TARGET_CC)" \
		DYNAMIC_CC="$(TARGET_CC) -fPIC" \
		TARGET_LD="$(TARGET_CC)" \
		TARGET_AR="$(TARGET_AR) rcus" \
		TARGET_STRIP=true \
		TARGET_CFLAGS="$(TARGET_CFLAGS)" \
		TARGET_LDFLAGS="$(TARGET_LDFLAGS)" \
		HOST_CC="$(RECALBOX_LUAJIT_HOST_CC)" \
		HOST_CFLAGS="$(HOST_CFLAGS)" \
		HOST_LDFLAGS="$(HOST_LDFLAGS)" \
		BUILDMODE=dynamic \
		XCFLAGS="$(RECALBOX_LUAJIT_XCFLAGS)" \
		-C $(@D) amalg
endef

define RECALBOX_LUAJIT_INSTALL_STAGING_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX="/usr" DESTDIR="$(STAGING_DIR)" LDCONFIG=true -C $(@D) install
endef

define RECALBOX_LUAJIT_INSTALL_TARGET_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) PREFIX="/usr" DESTDIR="$(TARGET_DIR)" LDCONFIG=true -C $(@D) install
endef

# host-efl package needs host-luajit to be linked dynamically.
define HOST_RECALBOX_LUAJIT_BUILD_CMDS
	$(HOST_MAKE_ENV) $(MAKE) PREFIX="$(HOST_DIR)" BUILDMODE=dynamic \
		TARGET_LDFLAGS="$(HOST_LDFLAGS)" \
		XCFLAGS="$(RECALBOX_LUAJIT_XCFLAGS)" \
		-C $(@D) amalg
endef

define HOST_RECALBOX_LUAJIT_INSTALL_CMDS
	$(HOST_MAKE_ENV) $(MAKE) PREFIX="$(HOST_DIR)" LDCONFIG=true -C $(@D) install
endef

$(eval $(generic-package))
$(eval $(host-generic-package))
