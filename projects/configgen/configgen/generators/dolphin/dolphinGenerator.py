#!/usr/bin/env python3
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.Vulkan import Vulkan
from configgen.utils.architecture import Architecture
from configgen.utils.videoMode import createCrtDesktopRes, getCrtResolution, deleteCrtDesktopRes


class DolphinGenerator(Generator):

    iniFile = recalboxFiles.CONF + '/dolphin-emu/Dolphin.ini'
    gfxFile = recalboxFiles.CONF + '/dolphin-emu/GFX.ini'
    retroAchievementsFile = recalboxFiles.CONF + '/dolphin-emu/RetroAchievements.ini'
    sysconfFile = recalboxFiles.SAVES + '/dolphin-emu/Wii/shared2/sys/SYSCONF'

    # Game Ratio
    GAME_RATIO = \
    {
       "auto": 0,
       "16/9": 1, 
       "4/3": 2,
       "squarepixel": 3,
    }

    # Wii Game Ratio
    WII_GAME_RATIO = \
    {
        "4/3": 0,
        "16/9": 1
    }

    # GameCube languages
    GAMECUBE_LANGUAGES = \
    {
        "en": 0,
        "de": 1,
        "fr": 2,
        "es": 3,
        "it": 4,
        "du": 5,
    }

    # Wii languages
    WII_LANGUAGES = \
    {
        "jp": 0,
        "en": 1,
        "de": 2,
        "fr": 3,
        "es": 4,
        "it": 5,
        "du": 6,
        "zh": 7,  # traditional chinese 8, ignored
        "kr": 9,
    }

    # Netplay servers
    NETPLAY_SERVERS = \
    {
        # Chine
        "CN": "CH",
        "TW": "CH",
        # Est Asia
        "KR": "EA",
        "RU": "EA",
        "JP": "EA",
        # Europe
        "GB": "EU",
        "DE": "EU",
        "FR": "EU",
        "ES": "EU",
        "IT": "EU",
        "PT": "EU",
        "TR": "EU",
        "SU": "EU",
        "NO": "EU",
        "NL": "EU",
        "PL": "EU",
        "HU": "EU",
        "CZ": "EU",
        "GR": "EU",
        "LU": "EU",
        "LV": "EU",
        "SE": "EU",
        # South America
        "BR": "SA",
        # North America
        "US": "NA",
    }

    # Rom paths
    ROM_PATHS = \
    {
        "0": "/recalbox/share/roms/gamecube",
        "1": "/recalbox/share/roms/wii"
    }

    # Video problematic games on Pi5
    game_force_opengl = [
        'GLME01', 'GLMP01', 'GLMJ01',  # Luigi's Mansion
        'GM4E01', 'GM4J01', 'GM4P01',  # Mario Kart: Double Dash!!
        'G4BE08', 'G4BP08', 'G4BJ08',  # Resident Evil 4
    ]

    ## Setting on dolphin.ini
    SECTION_ANALYTICS = "Analytics"
    SECTION_BTPASSTHROUGH = "BluetoothPassthrough"
    SECTION_CONTROLS = "Controls"
    SECTION_CORE = "Core"
    SECTION_GAMELIST = "GameList"
    SECTION_GENERAL = "General"
    SECTION_INTERFACE = "Interface"
    SECTION_NETPLAY = "NetPlay"
    SECTION_DISPLAY = "Display"
    SECTION_DSP = "DSP"
    SECTION_INPUT = "Input"
    ## Setting on GFX.ini
    SECTION_SETTINGS = "Settings"
    SECTION_HARDWARE = "Hardware"
    ## Setting in Retroachievements.ini
    SECTION_ACHIEVEMENTS = "Achievements"

    # Get keyboard layout
    @staticmethod
    def GetLanguage() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        # Try to obtain from keyboard layout, then from system language, then fallback to us
        kl = conf.getString("system.kblayout", conf.getString("system.language", "en")[0:2]).lower()
        return kl

    @staticmethod
    def GetFrontendLanguage() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        # Try to obtain from system language, then fallback to US or 
        esLanguage = conf.getString("system.language", "$")[0:2].upper()
        return esLanguage

    # Get netplay username
    @staticmethod
    def GetNetplay() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        pseudo = conf.getString("global.netplay.nickname", "$")
        return pseudo

    # Check if Gamecube bios IPL.bin in folders
    @staticmethod
    def CheckGamecubeIpl() -> str:
        ipl0 = "/recalbox/share/bios/gamecube/EUR/IPL.bin"
        ipl1 = "/recalbox/share/bios/gamecube/JP/IPL.bin"
        ipl2 = "/recalbox/share/bios/gamecube/USA/IPL.bin"
        # if os.path? return "True" set "False" for disable "SkipIpl=Video boot intro"
        import os.path
        if os.path.exists(ipl0) or os.path.exists(ipl1) or os.path.exists(ipl2):
            return "False"
        else:
            return "True"

    # GameCube Controller Adapter for Wii U in Dolphin controllers if realgamecubepads=1 set in recalbox.conf
    @staticmethod
    def SetGamecubeWiiUAdapter() -> str:
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        adapterStatus = conf.getString("gamecube.realgamecubepads", "$")
        return adapterStatus

    # Set Wii sensor bar position in SYSCONF if wii.sensorbar.position=0 (bottom) or =1 (top) set in recalbox.conf
    # (source : https://wiibrew.org/wiki//shared2/sys/SYSCONF#BT_Settings)
    @staticmethod
    def SetSensorBarPosition():
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)
        sensorBarPosition = conf.getString("wii.sensorbar.position", "$")
        keyValue = '\x00'.encode('utf-8') if sensorBarPosition == '0' else '\x01'.encode('utf-8')
        keyString = "BT.BAR".encode('utf-8')
        import os.path
        if os.path.exists(DolphinGenerator.sysconfFile):
            with open(DolphinGenerator.sysconfFile, 'rb+') as sysconf:
                buf = sysconf.read()
                keyAddr = buf.find(keyString) + len(keyString)
                sysconf.seek(keyAddr)
                sysconf.write(keyValue)

    @staticmethod
    def SetWiiLanguage():
        language = DolphinGenerator.GetLanguage()
        keyValue = chr(DolphinGenerator.WII_LANGUAGES[language]).encode('utf-8') if language in DolphinGenerator.WII_LANGUAGES else '\x01'.encode('utf-8')
        keyString = "IPL.LNG".encode('utf-8')

        import os.path

        if os.path.exists(DolphinGenerator.sysconfFile):
            with open(DolphinGenerator.sysconfFile, 'rb+') as sysconf:
                buf = sysconf.read()
                keyAddr = buf.find(keyString) + len(keyString)
                sysconf.seek(keyAddr)
                sysconf.write(keyValue)

    @staticmethod
    def SetWiiAspectRatio(system: Emulator):
        chosenRatio: str = system.Ratio
        if chosenRatio not in DolphinGenerator.WII_GAME_RATIO:
            from configgen.utils.resolutions import ResolutionParser
            resolution = ResolutionParser(system.VideoMode)
            if resolution.ratio() > 1.51:
                chosenRatio = "16/9"
            else:
                chosenRatio = "4/3"

        keyValue = chr(DolphinGenerator.WII_GAME_RATIO[chosenRatio]).encode('utf-8')
        keyString = "IPL.AR".encode('utf-8')

        import os.path

        if os.path.exists(DolphinGenerator.sysconfFile):
            with open(DolphinGenerator.sysconfFile, 'rb+') as sysconf:
                buf = sysconf.read()
                keyAddr = buf.find(keyString) + len(keyString)
                sysconf.seek(keyAddr)
                sysconf.write(keyValue)

    def mainConfiguration(self, system: Emulator, args):
        # Get Languages
        language = self.GetLanguage()
        systemLanguage = self.GetFrontendLanguage()
        gamecubeLanguage = self.GAMECUBE_LANGUAGES[language] if language in self.GAMECUBE_LANGUAGES else 0
        # Get Netplay configs
        nickname = self.GetNetplay()
        lobbyServer = self.NETPLAY_SERVERS[systemLanguage] if systemLanguage in self.NETPLAY_SERVERS else None
        ## Get if we have all IPL.bin in folders Gc bios
        biosGamecube = self.CheckGamecubeIpl()
        ## set Gamecube Wii U adapter
        padsGamecube = self.SetGamecubeWiiUAdapter()
        # Get current rom path
        import os
        romName = os.path.basename(args.rom)
        homeDir = args.rom.replace("/" + romName, "")
        
        # Load Configuration
        from configgen.settings.iniSettings import IniSettings
        dolphinSettings = IniSettings(DolphinGenerator.iniFile, True)
        dolphinSettings.loadFile(True) \
                       .defineBool('True', 'False')

        # Resolution
        from configgen.utils.resolutions import ResolutionParser
        resolution = ResolutionParser(system.VideoMode)
        if resolution.isSet and resolution.selfProcess:
            dolphinSettings.setString(self.SECTION_DISPLAY, "FullscreenResolution", resolution.string)
            dolphinSettings.setBool(self.SECTION_DISPLAY, "Fullscreen", True)
        if Architecture().isX64:
            dolphinSettings.setBool(self.SECTION_DISPLAY, "RenderToMain", True)
        dolphinSettings.setBool(self.SECTION_DISPLAY, "KeepWindowOnTop", True)
        # Logging
        dolphinSettings.setBool(self.SECTION_DSP, "CaptureLog", args.verbose)
        # Force audio backend on Pulse Audio
        dolphinSettings.setString(self.SECTION_DSP, "Backend", "Pulse")
        # Analytics
        dolphinSettings.setBool(self.SECTION_ANALYTICS, "Enabled", True)
        dolphinSettings.setBool(self.SECTION_ANALYTICS, "PermissionAsked", True)
        # Bluetooth Pass-trough
        dolphinSettings.setBool(self.SECTION_BTPASSTHROUGH, "Enabled", False)
        # Core
        dolphinSettings.setInt(self.SECTION_CORE, "SelectedLanguage", gamecubeLanguage)
        dolphinSettings.setBool(self.SECTION_CORE, "WiimoteContinuousScanning", True)
        dolphinSettings.setBool(self.SECTION_CORE, "WiiKeyboard", False)
        dolphinSettings.setString(self.SECTION_CORE, "SkipIpl", biosGamecube)
        dolphinSettings.setInt(self.SECTION_CORE, "SIDevice0", 12 if padsGamecube == '1' else 6)
        dolphinSettings.setInt(self.SECTION_CORE, "SIDevice1", 12 if padsGamecube == '1' else 6)
        dolphinSettings.setInt(self.SECTION_CORE, "SIDevice2", 12 if padsGamecube == '1' else 6)
        dolphinSettings.setInt(self.SECTION_CORE, "SIDevice3", 12 if padsGamecube == '1' else 6)
        dolphinSettings.setInt(self.SECTION_CORE, "SlotA", 8)
        dolphinSettings.setInt(self.SECTION_CORE, "SlotB", 8)
        dolphinSettings.setBool(self.SECTION_CORE, "AutoDiscChange", True)
        # GameList
        dolphinSettings.setBool(self.SECTION_GAMELIST, "ColumnID", True)
        # General
        dolphinSettings.setString(self.SECTION_GENERAL, "ISOPath0", self.ROM_PATHS['0'])
        dolphinSettings.setString(self.SECTION_GENERAL, "ISOPath1", self.ROM_PATHS['1'])
        if homeDir not in self.ROM_PATHS.values():
            dolphinSettings.setString(self.SECTION_GENERAL, "ISOPath2", homeDir)
            dolphinSettings.setInt(self.SECTION_GENERAL, "ISOPaths", 3)
        else:
            dolphinSettings.setInt(self.SECTION_GENERAL, "ISOPaths", 2)
        dolphinSettings.setBool(self.SECTION_GENERAL, "RecursiveISOPaths", True)
        dolphinSettings.setBool(self.SECTION_GENERAL, "HotkeysRequireFocus", False)
        # Interface
        dolphinSettings.setString(self.SECTION_INTERFACE, "LanguageCode", language)
        dolphinSettings.setBool(self.SECTION_INTERFACE, "AutoHideCursor", True)
        dolphinSettings.setBool(self.SECTION_INTERFACE, "ConfirmStop", False)
        dolphinSettings.setInt(self.SECTION_INTERFACE, "CursorVisibility", 0)
        dolphinSettings.setBool(self.SECTION_INTERFACE, "SkipNKitWarning", True)
        # Force OSD messages if RetroAchievements are enabled despite the choice made in frontend
        if system.Retroachievements and system.Name == "gamecube" and not args.demo:
            dolphinSettings.setBool(self.SECTION_INTERFACE, "OnScreenDisplayMessages", True)
        # Netplay
        dolphinSettings.setString(self.SECTION_NETPLAY, "Nickname", nickname)
        dolphinSettings.setBool(self.SECTION_NETPLAY, "UseUPNP", True)
        dolphinSettings.setString(self.SECTION_NETPLAY, "IndexName", nickname)
        dolphinSettings.setString(self.SECTION_NETPLAY, "TraversalChoice", "traversal")
        dolphinSettings.setBool(self.SECTION_NETPLAY, "UseIndex", True)
        dolphinSettings.setString(self.SECTION_NETPLAY, "IndexRegion", lobbyServer)
        # Input
        dolphinSettings.setBool(self.SECTION_INPUT, "BackgroundInput", True)

        # Save configuration
        dolphinSettings.saveFile()
        
        # Set Wii sensor bar position
        self.SetSensorBarPosition()
        self.SetWiiLanguage()
        self.SetWiiAspectRatio(system)


    def gfxConfiguration(self, system: Emulator):
        # Get Ratio
        gameRatio = self.GAME_RATIO[system.Ratio] if system.Ratio in self.GAME_RATIO else 0
        
        # Load Configuration
        from configgen.settings.iniSettings import IniSettings
        gfxSettings = IniSettings(DolphinGenerator.gfxFile, True)
        gfxSettings.loadFile(True)

        # Settings
        gfxSettings.setBool(self.SECTION_SETTINGS, "ShowFPS", system.ShowFPS)
        gfxSettings.setInt(self.SECTION_SETTINGS, "ShowVPS", system.ShowFPS)
        gfxSettings.setInt(self.SECTION_SETTINGS, "ShowSpeed", system.ShowFPS)
        gfxSettings.setInt(self.SECTION_SETTINGS, "AspectRatio", gameRatio)
        gfxSettings.setBool(self.SECTION_SETTINGS, "HiresTextures", True)
        gfxSettings.setBool(self.SECTION_SETTINGS, "CacheHiresTextures", True)

        # Save configuration
        gfxSettings.saveFile()

    def retroAchievementsConfiguration(self, system: Emulator, demo: bool):
        # Not compatible for Wii, Retroachievements should be enabled and not running from a demo
        hasRAEnabled = system.Retroachievements and system.Name == "gamecube" and not demo

        from configgen.settings.iniSettings import IniSettings
        retroAchievementsSettings = IniSettings(DolphinGenerator.retroAchievementsFile, True)
        retroAchievementsSettings.loadFile(True) \
                                 .defineBool('True', 'False')

        retroAchievementsSettings.setBool(self.SECTION_ACHIEVEMENTS, "Enabled", hasRAEnabled)
        retroAchievementsSettings.setBool(self.SECTION_ACHIEVEMENTS, "HardcoreEnabled", False)
        retroAchievementsSettings.setString(self.SECTION_ACHIEVEMENTS, "Username", system.RetroachievementsUsername)
        retroAchievementsSettings.setBool(self.SECTION_ACHIEVEMENTS, "ProgressEnabled", True)

        if hasRAEnabled and system.RetroachievementsHardcore:
            retroAchievementsSettings.setBool(self.SECTION_ACHIEVEMENTS, "HardcoreEnabled", True)

        retroAchievementsSettings.saveFile()

    @staticmethod
    def canVulkan(system: Emulator):
        return Vulkan.vulkanAvailable() and system.VulkanDriver

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        # Detect first the Game ID
        from configgen.generators.dolphin.dolphinRomType import DolphinRomType
        gameid = DolphinRomType.extract_gameid(args.rom)
        print("GameID: {}".format(gameid))

        video_driver = ''

        if not system.HasConfigFile:
            # Controllers
            import configgen.generators.dolphin.dolphinControllers as dolphinControllers
            dolphinControllers.generateControllerConfig(system, playersControllers, recalboxOptions, gameid)

            self.mainConfiguration(system, args)
            self.gfxConfiguration(system)
            self.retroAchievementsConfiguration(system, args.demo)

            # Render auto choice
            video_driver = "OGL"

            if Architecture().isX64 and self.canVulkan(system):
                video_driver = "Vulkan"

            if Architecture().isPi5:
                if (system.CRTAdapter != "none" and system.Core == "dolphin") or (
                        self.canVulkan(system) and gameid not in self.game_force_opengl) or (
                        system.CRTAdapter != "none" and system.Core == "dolphin-gui" and
                        gameid in ["GZLP01", "GZLE01", "GZLJ01"]):
                    video_driver = "Vulkan"

        # Configure crtdesktop
        rez = getCrtResolution(system)
        createCrtDesktopRes(rez, system)

        commandArray = [recalboxFiles.recalboxBins[system.Emulator], "-v", video_driver, "-C", "Logger.Options.Verbosity=5", "-C", "Logger.Logs.CONSOLE=true", "-C", "Logger.Logs.CORE=true", "-C", "Logger.Logs.Video=true", "-e", args.rom]
        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray, env={"SDL_JOYSTICK_HIDAPI": "0", "SDL_GAMEPAD_HIDAPI": "0", "XDG_CONFIG_HOME": recalboxFiles.CONF, "XDG_DATA_HOME": recalboxFiles.SAVES, "QT_QPA_PLATFORM": "xcb"}, postExec=[deleteCrtDesktopRes])
