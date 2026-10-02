################################################################################
#
# python-smbus2
#
################################################################################

PYTHON_SMBUS2_VERSION = ee73e1d65652dfedf8a14be8747d487fd59c4a57
PYTHON_SMBUS2_SITE =  $(call github,kplindegaard,smbus2,$(PYTHON_SMBUS2_VERSION))

PYTHON_SMBUS2_SETUP_TYPE=setuptools
PYTHON_SMBUS2_LICENSE = MIT
PYTHON_SMBUS2_LICENSE_FILES = LICENSE

$(eval $(python-package))
