#!/usr/bin/env python3
import shutil
import os.path
from configgen.Command import Command
from configgen.Emulator import Emulator
from configgen.generators.Generator import Generator, ControllerPerPlayer
import configgen.recalboxFiles as recalboxFiles
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.Vulkan import Vulkan
from configgen.generators.xemu.xemuControllers import XemuControllers


class XemuGenerator(Generator):
    SECTION_GENERAL = "general"
    SECTION_SYS = "sys"
    SECTION_SYS_FILES = "sys.files"
    SECTION_AUDIO = "audio"
    SECTION_DISPLAY = "display"
    SECTION_DISPLAY_QUALITY = "display.quality"
    SECTION_DISPLAY_UI = "display.ui"
    SECTION_DISPLAY_WINDOW = "display.window"
    SECTION_INPUT = "input"
    SECTION_NET = "net"

    savePath = recalboxFiles.SAVES + "/xbox"
    biosPath = recalboxFiles.BIOS + "/xbox"
    optionsConfigFile = recalboxFiles.CONF + '/xemu/xemu_config.ini'
    xemuConfigFile = recalboxFiles.CONF + '/xemu/xemu.toml'

    def mainConfiguration(self, system: Emulator, args):
        # Load xemu config file defined in frontend options and get values
        from configgen.settings.iniSettings import IniSettings
        configSettings = IniSettings(self.optionsConfigFile, True)
        configSettings.loadFile(True).defineBool('true', 'false')

        skip_boot_anim = configSettings.getBool(self.SECTION_GENERAL, "skip_boot_anim", False)
        show_menubar = configSettings.getBool(self.SECTION_DISPLAY_UI, "show_menubar", False)
        show_notifications = configSettings.getBool(self.SECTION_DISPLAY_UI, "show_notifications", False)
        fit = configSettings.getString(self.SECTION_DISPLAY_UI, "fit", "'scale'")
        aspect_ratio = configSettings.getString(self.SECTION_DISPLAY_UI, "aspect_ratio", "'native'")
        surface_scale = configSettings.getInt(self.SECTION_DISPLAY_QUALITY, "surface_scale", 1)

        print(f"Xemu configuration: skip_boot_anim:{skip_boot_anim}")

        # Load configuration
        xemuSettings = IniSettings(self.xemuConfigFile, True)
        xemuSettings.loadFile(True).defineBool('true', 'false')

        # General
        xemuSettings.setBool(self.SECTION_GENERAL, "skip_boot_anim", skip_boot_anim)
        xemuSettings.setBool(self.SECTION_GENERAL, "show_welcome", False)
        xemuSettings.setString(self.SECTION_GENERAL, "screenshot_dir", f"'{recalboxFiles.SCREENSHOTS}'")

        # Sys
        xemuSettings.setString(self.SECTION_SYS, "mem_limit", "'128'")

        # Sys.files
        xemuSettings.setString(self.SECTION_SYS_FILES, "flashrom_path", f"'{self.biosPath}/Complex_4627.bin'")
        xemuSettings.setString(self.SECTION_SYS_FILES, "bootrom_path", f"'{self.biosPath}/mcpx_1.0.bin'")
        xemuSettings.setString(self.SECTION_SYS_FILES, "hdd_path", f"'{self.savePath}/xbox_hdd.qcow2'")
        xemuSettings.setString(self.SECTION_SYS_FILES, "eeprom_path", f"'{self.savePath}/xemu_eeprom.bin'")

        # Escape rom name for games with ' in their names like "Doom 3 (Europe) (En,Fr,Es,It) (Limited Collector's Edition)"
        escaped_rom_path = str(args.rom).replace('\\', '\\\\').replace('"', '\\"')
        xemuSettings.setString(self.SECTION_SYS_FILES, "dvd_path", f"\"{escaped_rom_path}\"")

        # Audio
        xemuSettings.setBool(self.SECTION_AUDIO, "use_dsp", False)

        # Display
        if Vulkan.vulkanAvailable() and system.VulkanDriver:
            xemuSettings.setString(self.SECTION_DISPLAY, "renderer", "'VULKAN'")
        else:
            xemuSettings.setString(self.SECTION_DISPLAY, "renderer", "'OPENGL'")

        # Display quality
        xemuSettings.setInt(self.SECTION_DISPLAY_QUALITY, "surface_scale", surface_scale)

        # Display UI
        xemuSettings.setBool(self.SECTION_DISPLAY_UI, "show_menubar", show_menubar)
        xemuSettings.setString(self.SECTION_DISPLAY_UI, "fit", fit)
        xemuSettings.setString(self.SECTION_DISPLAY_UI, "aspect_ratio", aspect_ratio)
        xemuSettings.setBool(self.SECTION_DISPLAY_UI, "show_notifications", show_notifications)

        # Display window
        xemuSettings.setBool(self.SECTION_DISPLAY_WINDOW, "fullscreen_on_startup", True)
        xemuSettings.setBool(self.SECTION_DISPLAY_WINDOW, "vsync", True)

        # Inputs
        if system.Rumble:
            xemuSettings.setBool(self.SECTION_INPUT, "allow_vibration", True)
        else:
            xemuSettings.setBool(self.SECTION_INPUT, "allow_vibration", False)

        # Network
        # Documentation: https://github.com/xemu-project/xemu/blob/master/config_spec.yml
        xemuSettings.setBool(self.SECTION_NET, "enable", True)

        # Save configuration
        xemuSettings.saveFile()

    # Main entry of the module
    # Configure xemu and return a command
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings,
                 args) -> Command:
        self.mainConfiguration(system, args)

        from configgen.generators.xemu.xemuEeprom import XemuEeprom
        XemuEeprom().eepromConfiguration()

        # copy the hdd if it doesn't exist
        if not os.path.exists(self.savePath + "/xbox_hdd.qcow2"):
            if not os.path.exists(self.savePath):
                os.makedirs(self.savePath)
            shutil.copyfile("/recalbox/share_init/xemu/data/xbox_hdd.qcow2", self.savePath + "/xbox_hdd.qcow2")

        # the command to run
        commandArray = [recalboxFiles.recalboxBins[system.Emulator], "-config_path", self.xemuConfigFile]
        if system.HasArgs: commandArray.extend(system.Args)

        gamePadDB = XemuControllers(playersControllers)

        return Command(videomode=system.VideoMode, array=commandArray, env={"XDG_CONFIG_HOME": recalboxFiles.CONF, "SDL_GAMECONTROLLERCONFIG": "\n".join(gamePadDB.generateGameControllerConfig())})
