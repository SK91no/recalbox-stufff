################################################################################
#
# rtw89
#
################################################################################

# commit of 2024/10/05
# switched to the fork by Dosenpfand
# Maybe switch back to kernel module
RTW89_VERSION = 74033931a602ec84bb152d4a8c1df600aca447c5
RTW89_SITE = $(call github,Dosenpfand,rtw89,$(RTW89_VERSION))
RTW89_LICENSE = GPL-2.0
RTW89_LICENSE_FILES = LICENSE

$(eval $(kernel-module))
$(eval $(generic-package))
