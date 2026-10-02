################################################################################
#
# NODEJS_QUASAR
#
################################################################################

# Commit of 2023/07/01
NODEJS_QUASAR_VERSION = 2.12.2
NODEJS_QUASAR_CLI_VERSION = 2.2.1
NODEJS_QUASAR_SOURCE = quasar-v$(NODEJS_QUASAR_VERSION).tar.gz
NODEJS_QUASAR_SITE = https://github.com/quasarframework/quasar/archive/refs/tags
HOST_NODEJS_QUASAR_DEPENDENCIES = host-nodejs
NODEJS_QUASAR_LICENSE = MIT

define HOST_NODEJS_QUASAR_BUILD_CMDS
	cd $(@D)/cli && $(NPM) pack
endef

define HOST_NODEJS_QUASAR_INSTALL_CMDS
	cd $(@D)/cli && $(NPM) install --location=global quasar-cli-$(NODEJS_QUASAR_CLI_VERSION).tgz
endef

# define quasar cli for other packages
QUASAR = $(TARGET_CONFIGURE_OPTS) \
	$(HOST_DIR)/bin/quasar

$(eval $(host-generic-package))
