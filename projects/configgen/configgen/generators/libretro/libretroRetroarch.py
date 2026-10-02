from typing import List

import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.architecture import Architecture


class LibretroRetroarch:

    # Warning: The values in the array must be exactly at the same index than
    # https://github.com/libretro/RetroArch/blob/master/retroarch.c#L3042
    RATIO_INDEXES: List[str] = \
    [
        "4/3",
        "16/9",
        "16/10",
        "16/15",
        "21/9",
        "1/1",
        "2/1",
        "3/2",
        "3/4",
        "4/1",
        "9/16",
        "5/4",
        "6/5",
        "7/9",
        "8/3",
        "8/7",
        "19/12",
        "19/14",
        "30/17",
        "32/9",
        "config",  # 20
        "squarepixel",
        "coreprovided",
        "custom",
        "fullscreen"
    ]

    # Define systems not compatible with rewind option
    SYSTEM_WITH_NO_REWIND: List[str] = \
    (
        '3do',
        '64dd',
        'amigacd32',
        'amigacdtv',
        'atomiswave',
        'cassettevision',
        'cdi',
        'channelf',
        'dreamcast',
        'fba_libretro',
        'gamecube',
        'intellivision',
        'jaguar',
        'n64',
        'naomi',
        'naomi2',
        'nds',
        'neogeocd',
        'ngpc',
        'pcfx',
        'ps2',
        'psp',
        'psx',
        'saturn',
        'sega32x',
        'segacd',
        'virtualboy',
        'vectrex',
        'wii',
        'zxspectrum',
    )

    # Netplay modes
    NETPLAY_MODES = ('host', 'client')

    # Define systems compatible with retroachievements
    SYSTEM_COMPATIBLE_RETROACHIEVEMENTS: List[str] = \
    (
        '3do',
        'amstradcpc',
        'apple2',
        'arduboy',
        'atari2600',
        'atari7800',
        'channelf',
        'colecovision',
        'dreamcast',
        'fba_libretro',
        'fbneo',
        'gamecube',
        'gamegear',
        'gb',
        'gba',
        'gbc',
        'intellivision',
        'jaguar',
        'lynx',
        'mame',
        'mastersystem',
        'megadrive',
        'megaduck',
        'msx',
        'nds',
        'neogeo',
        'neogeocd',
        'n64',
        'nes',
        'ngp',
        'ngpc',
        'o2em',
        'pc88',
        'pcengine',
        'pcenginecd',
        'pcfx',
        'pokemini',
        'psx',
        'ps2',
        'satellaview',
        'saturn',
        'sega32x',
        'segacd',
        'sg1000',
        'snes',
        'sufami',
        'supervision',
        'uzebox',
        'vectrex',
        'virtualboy',
        'wasm4',
        'wonderswan',
        'wonderswancolor'
    )

    # AI Service language indexes - Keep in sync with
    # https://github.com/libretro/RetroArch/blob/master/translation_defines.h
    aiLanguages: List[str] =\
    [
        "AUTO",
        "EN",
        "ES",
        "FR",
        "IT",
        "DE",
        "JP",
        "NL",
        "CS",
        "DA",
        "SV",
        "HR",
        "KO",
        "ZH_CN",
        "ZH_TW",
        "CA",
        "BG",
        "BN",
        "EU",
        "AZ",
        "AR",
        "SQ",
        "AF",
        "EO",
        "ET",
        "TL",
        "FI",
        "GL",
        "KA",
        "EL",
        "GU",
        "HT",
        "IW",
        "HI",
        "HU",
        "IS",
        "ID",
        "GA",
        "KN",
        "LA",
        "LV",
        "LT",
        "MK",
        "MS",
        "MT",
        "NO",
        "FA",
        "PL",
        "PT",
        "RO",
        "RU",
        "SR",
        "SK",
        "SL",
        "SW",
        "TA",
        "TE",
        "TH",
        "TR",
        "UK",
        "UR",
        "VI",
        "CY",
        "YI"
    ]

    def languageIndex(self, language: str) -> str:
        try:
            return str(self.aiLanguages.index(language))
        except ValueError:
            return "0"

    def __init__(self, system: Emulator, settings: keyValueSettings, controllers: ControllerPerPlayer, demo: bool, recalboxOptions: keyValueSettings, rom: str = ""):
        self.recalboxOptions: keyValueSettings = recalboxOptions
        self.system: Emulator = system
        self.settings: keyValueSettings = settings
        self.controllers: ControllerPerPlayer = controllers
        self.demo: bool = demo
        self.rom: str = rom

    # Is nVidia driver on?
    @staticmethod
    def hasnVidiaDriver() -> bool:
        import platform
        arch: str = platform.machine()
        if arch == "x86_64":
            import os
            if os.path.exists("/etc/modprobe.d/blacklist.conf"):
               for line in open("/etc/modprobe.d/blacklist.conf"):
                   if "blacklist nouveau" in line:
                       return True
        return False

    def manage_audio_buffer(self, settings: keyValueSettings):
        # naomi2 drops the odd frame; a bigger audio buffer + rate control absorb the late frames.
        if self.system.Name == "naomi2" and Architecture().isPi5:
            # Beach Spikers stutters more than the rest; give it a deeper buffer.
            latency: int = 512 if "beachspi" in self.rom.lower() else 256
            settings.setInt("audio_latency", latency) \
                    .setBool("audio_rate_control", True) \
                    .setString("audio_rate_control_delta", "0.010")
        else:
            settings.setInt("audio_latency", 64) \
                    .setBool("audio_rate_control", True) \
                    .setString("audio_rate_control_delta", "0.005")

    # Fill retroarch configuration
    def fillRetroarchConfiguration(self):
        settings: keyValueSettings = self.settings

        # Screen resolution
        from configgen.utils.resolutions import ResolutionParser
        resolution = ResolutionParser(self.system.VideoMode)
        settings.setBool("video_fullscreen", True)
        if resolution.isSet and resolution.selfProcess:
            settings.setInt("video_fullscreen_x", resolution.width) \
                    .setInt("video_fullscreen_y", resolution.height)
        else:
            settings.removeOption("video_fullscreen_x") \
                    .removeOption("video_fullscreen_y")

        # AI System
        aiFrom: str = self.system.TranslateFrom.upper()
        aiTo: str = self.system.TranslateTo.upper()
        if len(aiTo) > 2:
            if not aiTo.startswith("ZH") and not aiTo.startswith("AUTO"):  # Keep chinese region
                aiTo = aiTo[:2]
        aiKey: str = self.system.TranslateAPIKey
        aiUrl: str = self.system.TranslateURL
        settings.setBool("ai_service_enable", self.system.Translate and aiKey != "") \
                .setDefaultInt("ai_service_mode", 0) \
                .setString("ai_service_source_lang", self.languageIndex(aiFrom)) \
                .setString("ai_service_target_lang", self.languageIndex(aiTo))
        if len(aiKey) == 0: aiKey = "RECALBOX"
        if len(aiUrl) == 0: aiUrl = "https://ztranslate.net/service?api_key={}".format(aiKey)
        settings.setString("ai_service_url", aiUrl)

        # Threaded video
        settings.setBool("video_threaded", False)

        # Enable RetroArch option "quit_press_twice"
        settings.setBool("quit_press_twice", self.system.QuitTwice)

        # Control new RA 1.7.7 key: do not allow upscaling higher than x4
        rguiUpscaling: int = settings.getInt("rgui_internal_upscale_level", 1)
        if rguiUpscaling > 4:
            settings.setInt("rgui_internal_upscale_level", 4)
        # Allow extended ASCII
        settings.setBool("rgui_extended_ascii", True)

        # Smoothing?
        settings.setBool("video_smooth", self.system.Smooth)

        # Shaders?
        import os
        noShaderCores = ["vitaquake3", "vitavoyager"]
        if self.system.Core not in noShaderCores:
            settings.setBool("video_shader_enable", self.system.HasShaderFile) \
                    .setBool("video_threaded", self.system.HasShaderFile) \
                    .setString("video_shader_dir", os.path.dirname(self.system.ShaderFile) if self.system.HasShaderFile else recalboxFiles.shadersRoot)
            if self.system.HasShaderFile:
                settings.setString("video_shader", os.path.basename(self.system.ShaderFile))

        # Screen ratio
        if self.system.Ratio in self.RATIO_INDEXES:
            settings.setInt("aspect_ratio_index", self.RATIO_INDEXES.index(self.system.Ratio)) \
                    .setBool("video_aspect_ratio_auto", False)
        elif self.system.Ratio == "none":
            # Do not fix aspect ratio. Let the user play with RA
            settings.setBool("video_aspect_ratio_auto", False)
        else:
            # default: auto
            settings.setBool("video_aspect_ratio_auto", True) \
                    .removeOption("aspect_ratio_index")

        # Rewind enabled?
        hasRewind: bool = self.system.Rewind and self.system.Name not in self.SYSTEM_WITH_NO_REWIND
        settings.setBool("rewind_enable", hasRewind)

        # Auto-save (not in demo mode! not in netplay mode!)
        hasAutoSave: bool = self.system.AutoSave and not self.demo and not self.system.Netplay
        settings.setBool("savestate_auto_save", hasAutoSave) \
                .setBool("savestate_auto_load", hasAutoSave)
        if self.system.Core == "tamalibretro":
            settings.setBool("savestate_auto_save", True) \
                    .setBool("savestate_auto_load", True)

        # Save thumbnail
        settings.setBool("savestate_thumbnail_enable", True)

        # Save folders
        settings.setString("savestate_directory", recalboxFiles.savesDir + self.system.Name) \
                .setString("savefile_directory", recalboxFiles.savesDir + self.system.Name)

        # Retroachievements (not in demo mode!)
        hasRetroAch = self.system.Retroachievements and not self.demo
        settings.setBool("cheevos_enable", hasRetroAch) \
                .setBool("cheevos_hardcore_mode_enable", False)
        if hasRetroAch and self.system.Name in self.SYSTEM_COMPATIBLE_RETROACHIEVEMENTS:
            settings.setString("cheevos_username", self.system.RetroachievementsUsername) \
                    .setString("cheevos_password", self.system.RetroachievementsPassword) \
                    .setBool("cheevos_hardcore_mode_enable", self.system.RetroachievementsHardcore) \
                    .setBool("cheevos_unlock_sound_enable", self.system.RetroachievementsSounds)

        # Integer scale
        settings.setBool("video_scale_integer", self.system.IntegerScale)

        # Display FPS
        settings.setBool("fps_show", self.system.ShowFPS) \
                .setInt("fps_update_interval", 256)

        # Netplay management
        settings.removeOption("netplay_password") \
                .removeOption("netplay_spectate_password") \
                .removeOption("netplay_start_as_spectator")
        if self.system.Netplay:
            settings.setBool("netplay_use_mitm_server", False)
            if self.system.NetplayHostMode:
                if self.system.HasNetplayMITM and self.system.NetplayMITM != "none":
                    settings.setBool("netplay_use_mitm_server", True) \
                            .setString("netplay_mitm_server", self.system.NetplayMITM)
            else:
                settings.setBool("netplay_start_as_spectator", self.system.NetplayViewerOnly)
            # Netplay passwords
            settings.setString("netplay_password", '"' + self.system.NetplayPlayerPassword + '"') \
                    .setString("netplay_spectate_password", '"' + self.system.NetplayViewerPassword + '"')

        # Experimental features
        settings.setBool("recalbox_experimental", self.system.RecalboxExperimental)
        if self.system.RecalboxExperimental:
            settings.setInt("video_max_swapchain_images", 2)
        else:
            settings.setInt("video_max_swapchain_images", 3)

        # Overlay reset
        settings.setString("input_overlay", "")

        # Ignore core informations (allow rewind, save states, netplay)
        if self.system.Core != "amiberry":
            settings.setBool("core_info_savestate_bypass", True)
        else:
            settings.setBool("core_info_savestate_bypass", False)

        # Force to disable gamemode
        settings.setBool("gamemode_enable", False)

        # Disable CRT calibration menu
        settings.setBool("quick_menu_show_crt_cali", False)

        # Disable autoconfiguration
        self.settings.setString("input_autodetect_enable", "false")

        # Disable cloud sync
        settings.setString("cloud_sync_driver", "null")

        # Ignore saves & save states sorted in subfolders with the core name
        settings.setBool("sort_savefiles_enable", False)
        settings.setBool("sort_savestates_enable", False)

        # Core assets path for dolphin only
        if self.system.Core == "dolphin":
            settings.setString("core_assets_directory", "/recalbox/share/bios/dolphin-emu/")
        else:
            settings.removeOption("core_assets_directory")

        # Audio buffer (naomi2 anti-crackle, HDMI + CRT)
        self.manage_audio_buffer(settings)

        # Use bios in directories
        # Most of the case are by core name
        match self.system.Core:
            case 'a5200':
                settings.setString("system_directory", recalboxFiles.BIOS + "/atari5200")
            case 'bsnes':
                if self.system.Name == 'gb' or self.system.Name == 'gbc':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/sgb")
                elif self.system.Name == 'satellaview':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/satellaview")
                else:
                    settings.setString("system_directory", recalboxFiles.BIOS)
            case 'bsneshd':
                if self.system.Name == 'gb' or self.system.Name == 'gbc':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/sgb")
                elif self.system.Name == 'satellaview':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/satellaview")
                else:
                    settings.setString("system_directory", recalboxFiles.BIOS)
            case 'cdi2015':
                settings.setString("system_directory", recalboxFiles.BIOS + "/cdi")
            case 'dolphin':
                settings.setString("system_directory", recalboxFiles.BIOS + "/gamecube")
            case 'emuscv':
                settings.setString("system_directory", recalboxFiles.BIOS + "/scv")
            case 'fmsx':
                settings.setString("system_directory", recalboxFiles.BIOS + "/msx")
            case 'freechaf':
                settings.setString("system_directory", recalboxFiles.BIOS + "/channelf")
            case 'freeintv':
                settings.setString("system_directory", recalboxFiles.BIOS + "/intellivision")
            case 'gambatte':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'gearsystem':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'genesisplusgx':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'genesisplusgxwide':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'geolith':
                settings.setString("system_directory", recalboxFiles.BIOS + "/geolith")
            case 'gpsp':
                settings.setString("system_directory", recalboxFiles.BIOS + "/gba")
            case 'mednafen_pce_fast':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'mednafen_pcfx':
                settings.setString("system_directory", recalboxFiles.BIOS + "/pcfx")
            case 'mednafen_supergrafx':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'geargrafx':
                settings.setString("system_directory", recalboxFiles.BIOS + "/pcenginecd")
            case 'mesen_s':
                if self.system.SuperGameBoy:
                    settings.setString("system_directory", recalboxFiles.BIOS + "/sgb")
                else:
                    settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'minivmac':
                settings.setString("system_directory", recalboxFiles.BIOS + "/macintosh")
            case 'mu':
                settings.setString("system_directory", recalboxFiles.BIOS + "/palm")
            case 'o2em':
                settings.setString("system_directory", recalboxFiles.BIOS + "/o2em")
            case 'opera':
                settings.setString("system_directory", recalboxFiles.BIOS + "/3do")
            case 'picodrive':
                settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'pokemini':
                settings.setString("system_directory", recalboxFiles.BIOS + "/pokemini")
            case 'prosystem':
                settings.setString("system_directory", recalboxFiles.BIOS + "/atari7800")
            case 'sameboy':
                if self.system.SuperGameBoy:
                    settings.setString("system_directory", recalboxFiles.BIOS + "/sgb")
                else:
                    settings.setString("system_directory", recalboxFiles.BIOS + "/" + self.system.Name)
            case 'snes9x':
                if self.system.Name == 'satellaview':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/satellaview")
                elif self.system.Name == 'sufami':
                    settings.setString("system_directory", recalboxFiles.BIOS + "/sufami")
                else:
                    settings.setString("system_directory", recalboxFiles.BIOS)
            case _:
                settings.setString("system_directory", recalboxFiles.BIOS)

        # We have a few to use by system name - when there are multiple cores with same bios
        amigaSystems = ["amiga600", "amiga1200", "amigacdtv", "amigacd32"]
        if self.system.Name in amigaSystems:
            settings.setString("system_directory", recalboxFiles.BIOS + "/amiga/bios")
        elif self.system.Name == 'fds':
            settings.setString("system_directory", recalboxFiles.BIOS + "/fds")
        elif self.system.Name == 'lynx':
            settings.setString("system_directory", recalboxFiles.BIOS + "/lynx")
        elif self.system.Name == 'nds':
            settings.setString("system_directory", recalboxFiles.BIOS + "/nds")
        elif self.system.Name == 'saturn':
            settings.setString("system_directory", recalboxFiles.BIOS + "/saturn")
        elif self.system.Name == 'stv':
            settings.setString("system_directory", recalboxFiles.BIOS + "/stv")

        print("Bios directory: {}".format(settings.getString("system_directory", recalboxFiles.BIOS)))
