#!/usr/bin/env python


class AmiberryGlobalConfig:

    def __init__(self, userGlobalSettingsFile, finalGlobalSettingsFile):
        self.globalSettingsFile = finalGlobalSettingsFile
        self.userSettingsFile = userGlobalSettingsFile

    def createGlobalSettings(self, verbose: bool, scanline: bool):
        # Load user settings first
        from configgen.settings.keyValueSettings import keyValueSettings
        settings = keyValueSettings(self.userSettingsFile)
        settings.loadFile(True).defineBool('yes', 'no')

        # Set default settings if they do not exist (the user may override them
        # through a .amiberry.conf dropped next to the roms)
        settings.setDefaultBool("read_config_descriptions", True)
        settings.setDefaultBool("write_logfile", verbose)
        settings.setDefaultBool("default_horizontal_centering", True)
        settings.setDefaultBool("default_vertical_centering", True)
        settings.setDefaultBool("default_correct_aspect_ratio", True)
        settings.setDefaultBool("default_frameskip", False)
        settings.setDefaultInt("default_fullscreen_mode", 1)
        settings.setDefaultInt("default_scaling_method", -1)
        settings.setDefaultBool("default_retroarch_quit", True)
        settings.setDefaultBool("default_retroarch_menu", True)
        settings.setDefaultBool("default_retroarch_reset", True)

        # Forced values
        import configgen.recalboxFiles as recalboxFiles
        mount = recalboxFiles.amiberryMountPoint
        settings.setBool("Quickstart", True)
        # Host paths (Amiberry 8.x reads them from amiberry.conf, overriding the
        # ones derived from AMIBERRY_HOME_DIR)
        settings.setString("config_path", mount + "/conf/")
        settings.setString("controllers_path", mount + "/conf/")
        settings.setString("retroarch_config", recalboxFiles.retroarchCustom)
        settings.setString("whdboot_path", mount + "/whdboot/")
        settings.setString("rp9_path", mount + "/conf/")
        # Kickstarts: Amiberry scans this folder on startup to build its ROM list
        settings.setString("rom_path", recalboxFiles.BIOS + "/amiga/bios/")
        settings.setString("logfile_path", mount + "/amiberry.log")
        # Saves / screenshots / nvram go to the recalbox share
        import os
        settings.setString("savestate_dir", os.path.join(recalboxFiles.SAVES, "amiga") + "/")
        settings.setString("nvram_dir", os.path.join(recalboxFiles.SAVES, "amiga") + "/")
        settings.setString("screenshot_dir", recalboxFiles.SCREENSHOTS + "/")

        # Save file
        settings.changeSettingsFile(self.globalSettingsFile)
        settings.saveFile()
