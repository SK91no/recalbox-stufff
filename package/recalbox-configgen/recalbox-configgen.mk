################################################################################
#
# recalbox configgen version
#
################################################################################

RECALBOX_CONFIGGEN_VERSION = custom
RECALBOX_CONFIGGEN_SITE = $(TOPDIR)/../projects/configgen
RECALBOX_CONFIGGEN_SITE_METHOD = local

RECALBOX_CONFIGGEN_LICENSE = GPLv2
RECALBOX_CONFIGGEN_DEPENDENCIES = python3 python-pyudev python-pygame2

RECALBOX_CONFIGGEN_SETUP_TYPE = setuptools

# `python -m build -n -w` against a legacy setup.py still leaves a non-wheel
# artefact (typically an .egg dir) next to the .whl in dist/, even with
# `setuptools.build_meta` declared via pyproject.toml. Buildroot's install
# step passes `dist/*` to pyinstaller.py which only accepts a single wheel
# argument, so prune everything but the wheel after the build.
define RECALBOX_CONFIGGEN_CLEAN_DIST
	find $(@D)/dist -mindepth 1 -maxdepth 1 ! -name '*.whl' -exec rm -rf {} +
endef
RECALBOX_CONFIGGEN_POST_BUILD_HOOKS += RECALBOX_CONFIGGEN_CLEAN_DIST

define RECALBOX_CONFIGGEN_CREATE_PYC_FILE
	$(PYTHON3_FIX_TIME)
	PYTHONPATH="$(PYTHON3_PATH)" \
	$(HOST_DIR)/bin/python$(PYTHON3_VERSION_MAJOR) \
		$(PYTHON3_DIR)/Lib/compileall.py \
		-s $(TARGET_DIR) \
		-p / \
		-b \
		$(TARGET_DIR)/usr/lib/python$(PYTHON3_VERSION_MAJOR)/site-packages/configgen/ \
		$(TARGET_DIR)/usr/bin/emulatorlauncher.py
endef

RECALBOX_CONFIGGEN_POST_INSTALL_TARGET_HOOKS += RECALBOX_CONFIGGEN_CREATE_PYC_FILE

$(eval $(python-package))
