################################################################################
#
# ARGON40CASE
#
################################################################################

ARGON40CASE_SOURCE =
ARGON40CASE_VERSION = 2502003 # version can be obtained from http://download.argon40.com/scripts/argon-versioninfo.sh
ARGON40CASE_LICENSE = Unspecified

ARGON40CASE_PYTHON = argonone-eepromconfig.py argoneond.py.pending argononed.py argoneonoled.py.pending \
										 argonstatus.py
ARGON40CASE_PYTHON_LIB = argonsysinfo.py argonregister-v1.py argonregister.py argonpowerbutton.py \
												 argonrtc.py.pending
ARGON40CASE_CONFIG = argoneonoled.conf argonunits.conf argononed.conf argononed-hdd.conf
ARGON40CASE_FONTS = font8x6.bin font16x12.bin font32x24.bin font64x48.bin font16x8.bin \
										font24x16.bin font48x32.bin bgdefault.bin bgram.bin bgip.bin \
										bgtemp.bin bgcpu.bin bgraid.bin bgstorage.bin bgtime.bin

define ARGON40CASE_INSTALL_TARGET_CMDS
	mkdir -p $(TARGET_DIR)/etc/argon
	$(foreach script, $(ARGON40CASE_PYTHON), \
		$(INSTALL) $(ARGON40CASE_PKGDIR)/scripts/$(script) $(TARGET_DIR)/etc/argon/ && \
		chmod +x $(TARGET_DIR)/etc/argon/$(script)
	)

	$(foreach script, $(ARGON40CASE_PYTHON_LIB), \
		$(INSTALL) $(ARGON40CASE_PKGDIR)/scripts/$(script) $(TARGET_DIR)/etc/argon/
	)
	mkdir -p $(TARGET_DIR)/etc/
	$(foreach config, $(ARGON40CASE_CONFIG), \
		$(INSTALL) $(ARGON40CASE_PKGDIR)/configs/$(config) $(TARGET_DIR)/etc/
	)

	mkdir -p $(TARGET_DIR)/etc/argon/oled
	$(foreach font, $(ARGON40CASE_FONTS), \
		$(INSTALL) $(ARGON40CASE_PKGDIR)/fonts/$(font) $(TARGET_DIR)/etc/argon/oled/
	)
endef

$(eval $(generic-package))
