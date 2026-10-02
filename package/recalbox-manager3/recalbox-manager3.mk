################################################################################
#
# RECALBOX_MANAGER3
#
################################################################################

RECALBOX_MANAGER3_VERSION = custom
RECALBOX_MANAGER3_SITE = $(TOPDIR)/../projects/manager3
RECALBOX_MANAGER3_SITE_METHOD = local
RECALBOX_MANAGER3_DEPENDENCIES = host-nodejs host-nodejs-quasar
RECALBOX_MANAGER3_LICENSE = MIT

define RECALBOX_MANAGER3_BUILD_CMDS
	if [ -f $(@D)/package-lock.json ] ; then rm $(@D)/package-lock.json ; fi
	cd $(@D) && \
		$(NPM) install
	cd $(@D) && \
		$(QUASAR) build
endef

define RECALBOX_MANAGER3_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/recalbox/web/manager-v3
	cp -r $(@D)/dist/spa/* \
		$(TARGET_DIR)/recalbox/web/manager-v3
endef

$(eval $(generic-package))
