################################################################################
#
# TAITO_PT
#
################################################################################

TAITO_PT_VERSION = 20d68d8dda237a28ea68361a6f5836cfe2990fc4
TAITO_PT_SITE = https://github.com/rtomasa/taito-pt.git
TAITO_PT_SITE_METHOD = git
TAITO_PT_LICENSE = GPL-2.0

$(eval $(kernel-module))
$(eval $(generic-package))
