from typing import Dict
from configgen.Emulator import Emulator
from configgen.crt.CRTTypes import CRTAdapter
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.controllers.controller import ControllerPerPlayer
from configgen.utils.architecture import Architecture


def _jvsCabFamily() -> str:
    try:
        with open("/sys/devices/platform/recalboxjvs/cab_family") as f:
            return f.read().strip()
    except OSError:
        return ""


class LibretroCores:

    def __init__(self, system: Emulator, settings: keyValueSettings, controllers: ControllerPerPlayer, rom: str):
        self.system: Emulator = system
        self.settings: keyValueSettings = settings
        self.controllers: ControllerPerPlayer = controllers
        self.rom = rom

    #
    # Core specific configurations
    #

    @staticmethod
    def configureBlueMSX(coreSettings: keyValueSettings):
        # Default core configuration - Actual value for legacy msx folder
        coreSettings.setString("bluemsx_msxtype", '"Auto"')

    @staticmethod
    def configureMAME2003plus(coreSettings: keyValueSettings):
        coreSettings.setString("mame2003-plus_analog", '"digital"')

    def configureMAME(self, coreSettings: keyValueSettings):
        coreSettings.setString("mame_read_config", '"enabled"')
        coreSettings.setString("mame_mouse_enable", '"enabled"')
        import shutil
        import configgen.recalboxFiles as recalboxFiles
        from pathlib import Path
        Path(recalboxFiles.mameConfigFolder).mkdir(parents=True, exist_ok=True)
        if self.system.CRTAdapter.hasServiceButtons():
            mameControllerSettings = "/jamma.cfg"
        else:
            mameControllerSettings = "/keyboard.cfg"
        shutil.copyfile(recalboxFiles.mameDefaultFolder + mameControllerSettings, recalboxFiles.mameSettingsFile)

    @staticmethod
    def configureOpera(coreSettings: keyValueSettings):
        coreSettings.setString("opera_dsp_threaded", '"enabled"')

    @staticmethod
    def configurePARALLELN64(coreSettings: keyValueSettings):
        coreSettings.setString("parallel-n64-boot-device", '"Default"')
        coreSettings.setString("parallel-n64-64dd-hardware", '"disabled"')
        coreSettings.setString("parallel-n64-gfxplugin", '"auto"')
        coreSettings.setString("parallel-n64-alt-map", '"disabled"')

    @staticmethod
    def configureMUPEN64(coreSettings: keyValueSettings):
        coreSettings.setString("mupen64plus-alt-map", '"False"')

    @staticmethod
    # (pc98) force the "joymode" option to have automatic joystick support.
    def configureNPKAI(coreSettings: keyValueSettings):
        coreSettings.setString("np2kai_joymode", '"Keypad"')

    def configurePicodrive(self, coreSettings: keyValueSettings):
        for p in (1, 2):
            ctrl = self.controllers.get(p)
            if ctrl is not None and ctrl.GamepadType.isArcade6Buttons():
                coreSettings.setString("picodrive_input{}".format(p), '"6 button pad"')

    #
    # System specific configurations
    #

    @staticmethod
    def configureAmigaCDTV(coreSettings: keyValueSettings):
        coreSettings.setString("puae_model", '"CDTV"')
        coreSettings.setString("puae_model_fd", "A500")
        coreSettings.setString("puae_model_hd", "A1200")
        coreSettings.setString("puae_model_cd", '"CDTV"')
        coreSettings.setString("puae_cpu_compatibility", '"compatible"')
        coreSettings.setString("uae4arm_model", '"CDTV"')
        coreSettings.setString("amiberry_model", '"CDTV"')
        coreSettings.setString("amiberry_port0_device", '"mouse"')
        coreSettings.setString("amiberry_port1_device", '"joystick"')

    @staticmethod
    def configureAmigaCD32(coreSettings: keyValueSettings):
        coreSettings.setString("puae_model", '"CD32"')
        coreSettings.setString("puae_model_fd", "A1200")
        coreSettings.setString("puae_model_hd", "A1200")
        coreSettings.setString("puae_model_cd", "CD32")
        coreSettings.setString("puae_cpu_compatibility", '"compatible"')
        coreSettings.setString("uae4arm_model", '"CD32"')
        coreSettings.setString("amiberry_model", '"CD32"')
        coreSettings.setString("amiberry_port0_device", '"joystick"')
        coreSettings.setString("amiberry_port1_device", '"joystick"')
        coreSettings.setString("amiberry_joyport_order", '"21"')

    @staticmethod
    def configureAmiga1200(coreSettings: keyValueSettings):
        coreSettings.setString("puae_model", '"A1200"')
        coreSettings.setString("puae_model_fd", "A1200")
        coreSettings.setString("puae_model_hd", "A1200")
        coreSettings.setString("puae_model_cd", "CD32FR")
        coreSettings.setString("puae_cpu_compatibility", '"compatible"')
        coreSettings.setString("uae4arm_model", '"A1200"')
        coreSettings.setString("amiberry_model", '"A1200"')
        coreSettings.setString("amiberry_port0_device", '"mouse"')
        coreSettings.setString("amiberry_port1_device", '"joystick"')

    @staticmethod
    def configureAmiga600(coreSettings: keyValueSettings):
        coreSettings.setString("puae_model", '"A600"')
        coreSettings.setString("puae_model_fd", "A500")
        coreSettings.setString("puae_model_hd", "A600")
        coreSettings.setString("puae_model_cd", "CDTV")
        coreSettings.setString("puae_cpu_compatibility", '"compatible"')
        coreSettings.setString("uae4arm_model", '"A600"')
        coreSettings.setString("amiberry_model", '"A600"')
        coreSettings.setString("amiberry_port0_device", '"mouse"')
        coreSettings.setString("amiberry_port1_device", '"joystick"')

    @staticmethod
    def configureAmstradCPC(coreSettings: keyValueSettings):
        coreSettings.setString("cap32_model", '"6128"')

    @staticmethod
    def configureAmstradGX4000(coreSettings: keyValueSettings):
        coreSettings.setString("cap32_model", '"6128+ (experimental)"')
        coreSettings.setString("cap32_gfx_colors", "24bit")

    @staticmethod
    def configureAtari5200(coreSettings: keyValueSettings):
        coreSettings.setString("atari800_system", '"5200"')

        import configgen.recalboxFiles as recalboxFiles
        a800settings = keyValueSettings(recalboxFiles.atari800CustomConfig)
        a800settings.loadFile(True)
        a800settings.setString("MACHINE_TYPE", "Atari 5200")
        a800settings.setInt("RAM_SIZE", 16)
        a800settings.setInt("STEREO_POKEY", 0)
        a800settings.setInt("BUILTIN_BASIC", 0)
        a800settings.saveFile()

    @staticmethod
    def configureAtari800(coreSettings: keyValueSettings):
        coreSettings.setString("atari800_system", '"130XE (128K)"')

        import configgen.recalboxFiles as recalboxFiles
        a800settings = keyValueSettings(recalboxFiles.atari800CustomConfig)
        a800settings.loadFile(True)
        a800settings.setString("MACHINE_TYPE", "Atari XL/XE")
        a800settings.setInt("RAM_SIZE", 64)
        a800settings.setInt("STEREO_POKEY", 1)
        a800settings.setInt("BUILTIN_BASIC", 1)
        a800settings.saveFile()

    def configureAtariSTNew(self, coreSettings: keyValueSettings):
        # Machine list as defined in HatariB core
        MACHINE_UNKNOWN = -1
        MACHINE_ST = 0
        MACHINE_MEGA_ST = 1
        MACHINE_STE = 2
        MACHINE_MEGA_STE = 3
        MACHINE_TT = 4
        MACHINE_FALCON = 5

        # Try identify machine folder
        machines: dict[str, int] = \
        {
            "atari-st": MACHINE_ST,
            "/st/": MACHINE_ST,
            "atarimegast": MACHINE_MEGA_ST,
            "atari-megast": MACHINE_MEGA_ST,
            "atari-mega-st": MACHINE_MEGA_ST,
            "megast": MACHINE_MEGA_ST,
            "mega-st": MACHINE_MEGA_ST,
            "atariste": MACHINE_STE,
            "atari-ste": MACHINE_STE,
            "/ste/": MACHINE_STE,
            "atarimegaste": MACHINE_MEGA_STE,
            "atari-megaste": MACHINE_MEGA_STE,
            "atari-mega-ste": MACHINE_MEGA_STE,
            "megaste": MACHINE_MEGA_STE,
            "mega-ste": MACHINE_MEGA_STE,
            "ataritt": MACHINE_TT,
            "atari-tt": MACHINE_TT,
            "/tt/": MACHINE_TT,
            "falcon": MACHINE_FALCON,
            "atari-falcon": MACHINE_FALCON,
        }
        defaultMachine: int = MACHINE_UNKNOWN
        romLower: str = self.rom.lower()
        for key in machines:
            if key in romLower:
                defaultMachine: int = machines[key]
                break
        if not defaultMachine == MACHINE_UNKNOWN:
            coreSettings.setInt("hatarib_machine", defaultMachine)
            print("MACHINE: {}".format(defaultMachine))
        else:
            defaultMachine = coreSettings.getInt("hatarib_machine", MACHINE_ST)

        # Select bios
        subdir: str = "atarist"
        import os
        biosFile = os.path.join(subdir, "st.img")
        # ST/MEGAST use both st.img
        if   defaultMachine == MACHINE_FALCON  : biosFile = os.path.join(subdir, "falcon.img")
        elif defaultMachine == MACHINE_MEGA_STE: biosFile = os.path.join(subdir, "megaste.img")
        elif defaultMachine == MACHINE_TT      : biosFile = os.path.join(subdir, "tt.img")
        elif defaultMachine == MACHINE_STE     : biosFile = os.path.join(subdir, "ste.img")
        import configgen.recalboxFiles as recalboxFiles
        if not os.path.exists(os.path.join(recalboxFiles.BIOS, biosFile)):
            biosFile = os.path.join(subdir, "tos.img")
            if not os.path.exists(os.path.join(recalboxFiles.BIOS, biosFile)):
                biosFile = "tos.img"
                if not os.path.exists(os.path.join(recalboxFiles.BIOS, biosFile)):
                    biosFile = "<etos1024k>"
        if "emutos" in self.rom:
            biosFile = os.path.join(recalboxFiles.BIOS, subdir, "<etos1024k>")
        coreSettings.setString("hatarib_tos", str(biosFile))
        print("BIOS   : {}".format(biosFile))

    def configureAtariST(self, _):
        # Machines as defined in old Hatari core
        MACHINE_UNKNOWN = -1
        MACHINE_ST = 0
        MACHINE_STE = 1
        MACHINE_TT = 2
        MACHINE_FALCON = 3,
        MACHINE_MEGA_STE = 4

        # Load config
        from configgen.settings.iniSettings import IniSettings
        import configgen.recalboxFiles as recalboxFiles
        atariStSettings = IniSettings(recalboxFiles.hatariCustomConfig)
        atariStSettings.loadFile(True)

        # Auto set machine
        from typing import Dict
        # Try identify machine folder
        machines: Dict[str, int] = \
        {
            "atari-st": MACHINE_ST,
            "/st/": MACHINE_ST,
            "atariste": MACHINE_STE,
            "atari-ste": MACHINE_STE,
            "/ste/": MACHINE_STE,
            "ataritt": MACHINE_TT,
            "atari-tt": MACHINE_TT,
            "/tt/": MACHINE_TT,
            "atarimegaste": MACHINE_MEGA_STE,
            "atari-megaste": MACHINE_MEGA_STE,
            "atari-mega-ste": MACHINE_MEGA_STE,
            "megaste": MACHINE_MEGA_STE,
            "mega-ste": MACHINE_MEGA_STE,
            "falcon": MACHINE_FALCON,
            "atari-falcon": MACHINE_FALCON,
        }
        defaultMachine: int = MACHINE_UNKNOWN
        romLower: str = self.rom.lower()
        for key in machines:
            if key in romLower:
                defaultMachine: int = machines[key]
                break
        if not defaultMachine == MACHINE_UNKNOWN:
            print("MACHINE: {}".format(defaultMachine))
            atariStSettings.setString("System", "nMachineType", str(defaultMachine))

        # Select bios
        import os
        subdir: str = "atarist"
        biosPath = os.path.join(recalboxFiles.BIOS, subdir, "st.img")
        if   defaultMachine == MACHINE_STE     : biosPath = os.path.join(recalboxFiles.BIOS, subdir, "ste.img")
        elif defaultMachine == MACHINE_TT      : biosPath = os.path.join(recalboxFiles.BIOS, subdir, "tt.img")
        elif defaultMachine == MACHINE_MEGA_STE: biosPath = os.path.join(recalboxFiles.BIOS, subdir, "megaste.img")
        elif defaultMachine == MACHINE_FALCON  : biosPath = os.path.join(recalboxFiles.BIOS, subdir, "falcon.img")
        if not os.path.exists(biosPath):
            biosPath = os.path.join(recalboxFiles.BIOS, subdir, "tos.img")
            if not os.path.exists(biosPath):
                biosPath = os.path.join(recalboxFiles.BIOS, "tos.img")
                if not os.path.exists(biosPath):
                    biosPath = os.path.join(recalboxFiles.BIOS, subdir, "emutos.img")
        if "emutos" in self.rom:
            biosPath = os.path.join(recalboxFiles.BIOS, subdir, "emutos.img")
        atariStSettings.setString("ROM", "szTosImageFileName", str(biosPath))
        print("BIOS   : {}".format(biosPath))

        # Save config
        atariStSettings.saveFile()

    @staticmethod
    def configure64DD(coreSettings: keyValueSettings):
        coreSettings.setString("parallel-n64-boot-device", '"64DD IPL"')
        coreSettings.setString("parallel-n64-64dd-hardware", '"enabled"')
        coreSettings.setString("parallel-n64-gfxplugin", '"gln64"')
        coreSettings.setString("parallel-n64-alt-map", '"disabled"')
        coreSettings.setString("mupen64plus-alt-map", '"False"')

    def configureNDS(self, coreSettings: keyValueSettings) -> None:
        # Auto-apply game languages
        DS_LANGUAGES = \
        {
            "jp": "Japanese",
            "en": "English",
            "fr": "French",
            "de": "German",
            "it": "Italian",
            "es": "Spanish"
        }

        import configgen.recalboxFiles as recalboxFiles
        conf = keyValueSettings(recalboxFiles.recalboxConf).loadFile(True)
        kl = conf.getString("system.kblayout", conf.getString("system.language", "en")[0:2]).lower()
        language = DS_LANGUAGES[kl] if kl in DS_LANGUAGES else '"English"'
        settingName: str = ""

        if self.system.Core == "melonds":
            settingName = "melonds"
        elif self.system.Core == "desmume":
            settingName = "desmume_firmware"

        coreSettings.setString(settingName + "_language", f'"{language}"')

    @staticmethod
    def configureSpectravideo(coreSettings: keyValueSettings):
        coreSettings.setString("bluemsx_msxtype", '"SVI - Spectravideo SVI-328 MK2"')

    @staticmethod
    def configureMsx1(coreSettings: keyValueSettings):
        coreSettings.setString("bluemsx_msxtype", '"MSX"')

    @staticmethod
    def configureMsx2(coreSettings: keyValueSettings):
        coreSettings.setString("bluemsx_msxtype", '"MSX2+"')

    @staticmethod
    def configureMsxTurboR(coreSettings: keyValueSettings):
        coreSettings.setString("bluemsx_msxtype", '"MSXturboR"')

    @staticmethod
    def configureOdyssey2(coreSettings: keyValueSettings):
        coreSettings.setString("o2em_bios", '"o2rom.bin"')

    @staticmethod
    def configureVideoPacPlus(coreSettings: keyValueSettings):
        coreSettings.setString("o2em_bios", '"g7400.bin"')

    @staticmethod
    def configureSg1000(coreSettings: keyValueSettings):
        coreSettings.setString("bluemsx_msxtype", "SEGA - SC-3000")

    def configureNaomi2(self, coreSettings: keyValueSettings):
        # Beach Spikers needs a little frameskip to hold its audio; every other naomi2 game keeps the
        # "disabled" base that configureFlycast sets.
        if "beachspi" in self.rom.lower():
            coreSettings.setString("reicast_auto_skip_frame", '"some"')
        if Architecture().isPi5:
            coreSettings.setString("reicast_alpha_sorting", '"per-strip (fast, least accurate)"')
            coreSettings.setString("reicast_anisotropic_filtering", '"2"')
            # Override configureFlycast's sh4clock=300 back to 200: the dynarec scales guest_cycles by
            # 200/sh4clock, so 300 runs the SH4 too fast vs fixed-cycle Elan DMA events and hangs
            # timing-sensitive naomi2 loading (Initial D, Beach Spikers). No fps loss (memory-bound).
            coreSettings.setString("reicast_sh4clock", '"200"')
        else:
            coreSettings.setString("reicast_alpha_sorting", '"per-triangle (normal)"')
            coreSettings.setString("reicast_anisotropic_filtering", '"4"')
            coreSettings.setString("reicast_sh4clock", '"200"')

    def configureSwanstation(self, coreSettings: keyValueSettings):
        coreSettings.setString("duckstation_Controller2.Type", '"DigitalController"')
        coreSettings.setString("swanstation_ControllerPorts_MultitapMode", '"Disabled"')
        if len(self.controllers) > 2:
            coreSettings.setString("swanstation_ControllerPorts_MultitapMode", '"Port1Only"')
        elif len(self.controllers) > 5:
            coreSettings.setString("swanstation_ControllerPorts_MultitapMode", '"BothPorts"')

    @staticmethod
    def configureDosBoxPure(coreSettings: keyValueSettings):
        coreSettings.setString("dosbox_pure_savestate", '"rewind"')

    def configureFlycast(self, coreSettings: keyValueSettings):
        coreSettings.setString("reicast_alpha_sorting", '"per-triangle (normal)"')
        coreSettings.setString("reicast_anisotropic_filtering", '"4"')
        # Overclock the emulated SH4 (300 vs stock 200) to help CPU-bound flycast games (naomi/dreamcast/
        # atomiswave, e.g. Crazy Taxi). naomi2 overrides back to 200 (configureNaomi2) since 300 hangs its
        # timing-sensitive loading.
        coreSettings.setString("reicast_sh4clock", '"300"')
        coreSettings.setString("reicast_per_content_vmus", '"All VMUs"')
        # Base: no frameskip. configureNaomi2 turns it on for Beach Spikers only (else it would leak to
        # every flycast game via the shared core-options file).
        coreSettings.setString("reicast_auto_skip_frame", '"disabled"')
        if self.system.CRTAdapter.hasServiceButtons():
            coreSettings.setString("reicast_allow_service_buttons", '"enabled"')
        if self.system.CRTAdapter == CRTAdapter.RECALBOXJVS and _jvsCabFamily() == "race":
            coreSettings.setString("reicast_analog_stick_deadzone", '"0%"')
            coreSettings.setString("reicast_trigger_deadzone", '"0%"')
        if self.system.Core == "flycast-next":
            coreSettings.setString("reicast_network_output", '"enabled"')

    def configurePPSSPP(self, coreSettings: keyValueSettings):
        # The ppsspp core selects its GPU backend from this core option (not from retroarch video_driver).
        # "auto" follows the driver picked by createVideoDriverConfiguration, so vulkan flows from there.
        # CRT switchres only works under OpenGL, so force it when CRT is enabled.
        coreSettings.setString("ppsspp_backend", '"opengl"' if self.system.CRTEnabled else '"auto"')

    def configureFBNeo(self, coreSettings: keyValueSettings):
        coreSettings.setString("fbneo-analog-speed", '"100%"')
        coreSettings.setString("fbneo-diagnostic-input", '"Hold Start"')
        if self.system.CRTAdapter.hasServiceButtons():
            coreSettings.setString("fbneo-diagnostic-input", '"L3"')
        if self.system.Netplay and not self.system.NetplayHostMode:
            coreSettings.setString("fbneo-diagnostic-input", '"None"')

    def configureScummvm(self, coreSettings: keyValueSettings):
        import configgen.recalboxFiles as recalboxFiles
        import configparser
        import os.path

        language = None
        subtitles = "true"
        scummvmSettings = configparser.ConfigParser(
                strict=False, allow_no_value=False, empty_lines_in_values=False
            )
        scummvmSettings.read(recalboxFiles.libretroScummvmConfig)

        if os.path.exists(recalboxFiles.HOME + '/configs/recalbox/tmp-scummvm.ini'):
            settings = keyValueSettings(recalboxFiles.HOME + '/configs/recalbox/tmp-scummvm.ini', True).loadFile(False)
            subtitles = settings.getString("subtitles", "true")

        scummvmSettings.read(recalboxFiles.libretroScummvmConfig)
        scummvmSettings.set("scummvm", "extrapath", recalboxFiles.libretroScummvmBios)
        scummvmSettings.set("scummvm", "savepath", recalboxFiles.scummvmSaves)

        if self.system.CRTEnabled:
            coreSettings.setString("scummvm_gui_h_res", '"240"')
            coreSettings.setString("scummvm_gui_aspect_ratio", '"0"')
        else:
            coreSettings.setString("scummvm_gui_h_res", '"720"')
            coreSettings.setString("scummvm_gui_aspect_ratio", '"1"')

        if scummvmSettings.get("scummvm", "mt32_device") == "fluidsynth":
            scummvmSettings.set("scummvm", "soundfont", recalboxFiles.libretroScummvmBios + "/Roland_SC-55.sf2")

        if self.system.IntegerScale:
            scummvmSettings.set("scummvm", "stretch_mode", "pixel-perfect")
        else:
            scummvmSettings.set("scummvm", "stretch_mode", "default")

        if os.path.isdir(self.rom):
            scummfiles = [fn for fn in os.listdir(self.rom) if fn.endswith('.scummvm')]
            if len(scummfiles) == 1:
                rom = os.path.join(self.rom, scummfiles[0])
                if os.path.isfile(rom):
                    with open(rom, 'rb') as f:
                        s = str(f.read())
                        if len(s.split(":")) > 2:
                            language = s.split(":")[2][:2]

        scummvmSettings.set("scummvm", "subtitles", subtitles)

        if language:
            scummvmSettings.set("scummvm", "language", language)
        else:
            scummvmSettings.remove_option("scummvm", "language")

        with open(recalboxFiles.libretroScummvmConfig, 'w') as f:
            scummvmSettings.write(f, space_around_delimiters=False)

    def configureSaturn(self, coreSettings: keyValueSettings):
        if self.system.Core == "mednafen_saturn":
            core = "beetle_saturn"
        else:
            core = self.system.Core
        coreSettings.setString("{}_multitap_port1".format(core), '"1"')
        coreSettings.setString("{}_multitap_port2".format(core), '"1"')
        if len(self.controllers) > 2:
            coreSettings.setString("{}_multitap_port1".format(core), '"1"')
        if len(self.controllers) > 7:
            coreSettings.setString("{}_multitap_port2".format(core), '"1"')

    def configureDolphin(self, coreSettings: keyValueSettings) -> None:
        coreSettings.setString("dolphin_emulator_speed", '"1.0"')
        coreSettings.setString("dolphin_cpu_clock_rate", '"1.0"')

        ipl0 = "/recalbox/share/bios/gamecube/EUR/IPL.bin"
        ipl1 = "/recalbox/share/bios/gamecube/JP/IPL.bin"
        ipl2 = "/recalbox/share/bios/gamecube/USA/IPL.bin"

        import os.path
        if os.path.exists(ipl0) or os.path.exists(ipl1) or os.path.exists(ipl2):
            coreSettings.setString("dolphin_skip_gc_bios", '"disabled"')
        else:
            coreSettings.setString("dolphin_skip_gc_bios", '"enabled"')

        if Architecture().isX64:
            coreSettings.setString("dolphin_shader_compilation_mode", '"1"')
        else:
            coreSettings.setString("dolphin_shader_compilation_mode", '"0"')

        # Define auto options
        import configgen.recalboxFiles as recalboxFiles
        conf = keyValueSettings(recalboxFiles.recalboxConf)
        conf.loadFile(True)

        # Define language
        languages: Dict[str, int] = {"jp": 0, "en": 1, "de": 2, "fr": 3, "es": 4, "it": 5, "du": 6}

        kl = conf.getString("system.kblayout", conf.getString("system.language", "en")[0:2]).lower()
        language = languages[kl] if kl in languages else 1  # English as default
        coreSettings.setInt("dolphin_language", language)

        # Define sensor bar position
        sensorBar: Dict[str, int] = {"top": 1, "bottom": 2}
        sb = conf.getString("wii.sensorbar.position", "$")
        sensorBarPosition = sensorBar[sb] if sb in sensorBar else 1  # Top as default
        coreSettings.setInt("dolphin_sensor_bar_position", sensorBarPosition)

        # Define aspect ratio for Wii only
        wiiGameRatio: Dict[str, str] = {"16/9": "enabled", "4/3": "disabled"}
        chosenRatio: str = self.system.Ratio
        if chosenRatio not in wiiGameRatio:
            from configgen.utils.resolutions import ResolutionParser
            resolution = ResolutionParser(self.system.VideoMode)
            if resolution.ratio() > 1.51:
                coreSettings.setString("dolphin_widescreen", '"enabled"')
            else:
                coreSettings.setString("dolphin_widescreen", '"disabled"')

    @staticmethod
    def configureMelonDS(coreSettings: keyValueSettings):
        if LibretroCores.touchscreen():
            coreSettings.setString('melonds_touch_mode', '"Touch"')
        pass

    @staticmethod
    def configureDesmume(coreSettings: keyValueSettings):
        if LibretroCores.touchscreen():
            coreSettings.setString('desmume_pointer_type', '"touch"')
        pass

    def configurePCSXRearmed(self, coreSettings: keyValueSettings):
        coreSettings.setString("pcsx_rearmed_multitap", '"disabled"')
        if len(self.controllers) > 2:
            coreSettings.setString("pcsx_rearmed_multitap", '"port 1"')
        elif len(self.controllers) > 5:
            coreSettings.setString("pcsx_rearmed_multitap", '"ports 1 and 2"')

    def configureBeetlePSX(self, coreSettings: keyValueSettings):
        coreSettings.setString("beetle_psx_enable_multitap_port1", '"disabled"')
        coreSettings.setString("beetle_psx_enable_multitap_port2", '"disabled"')
        if len(self.controllers) > 2:
            coreSettings.setString("beetle_psx_enable_multitap_port1", '"enabled"')
        if len(self.controllers) > 5:
            coreSettings.setString("beetle_psx_enable_multitap_port2", '"enabled"')

    def configureGambatte(self, coreSettings: keyValueSettings):
        if self.system.Name == "gb":
            coreSettings.setString('gambatte_gb_colorization', '"internal"')
            coreSettings.setString('gambatte_gb_internal_palette', '"GB - DMG"')
        elif self.system.Name == "gbc":
            coreSettings.setString('gambatte_gb_colorization', '"GBC"')
            coreSettings.setString('gambatte_gb_internal_palette', '""')

    def configureSTV(self, coreSettings: keyValueSettings):
        if Architecture().isX64:
            coreSettings.setString('mednafen_stv_cpu_cache', 'full')

    def configureEightyOne(self, coreSettings: keyValueSettings):
        # Keep the core's "eightyone_machine" option aligned with the Recalbox
        # system we were launched from — the user might otherwise carry over an
        # incompatible machine type saved from a previous session.
        ZX81_FAMILY = {
            '"Auto (ZX81 family)"',
            '"ZX80"', '"ZX81"', '"ZX97LE"',
            '"Timex TS1000"', '"Timex TS1500"',
            '"TK85"', '"Lambda 8300"', '"Ringo R470"',
            '"Jupiter ACE"',
        }
        SPECTRUM_FAMILY = {
            '"Auto (Spectrum family)"',
            '"Spectrum 16K"', '"Spectrum 48K"', '"Spectrum+"',
            '"Spectrum 128K"', '"Spectrum +2"', '"Spectrum +2A"', '"Spectrum +3"',
            '"Timex TC2048"', '"Timex TC2068"', '"Timex TS2068"',
        }
        key = 'eightyone_machine'
        current = coreSettings.getString(key, '')
        if self.system.Name == "zx81":
            if current not in ZX81_FAMILY:
                coreSettings.setString(key, '"Auto (ZX81 family)"')
        elif self.system.Name == "zxspectrum":
            if current not in SPECTRUM_FAMILY:
                coreSettings.setString(key, '"Auto (Spectrum family)"')

    def configureTGBDual(self, coreSettings: keyValueSettings):
        # Screen AND audio must follow the same Game Boy: the host plays GB1,
        # the netplay client plays GB2 — otherwise the client hears the host's game.
        if self.system.Netplay:
            # Netplay from anywhere (not only roms/gb/gamelink): both GBs must be linked
            coreSettings.setString("tgbdual_gblink_enable", '"enabled"')
        if self.system.Netplay and self.system.NetplayHostMode:
            coreSettings.setString("tgbdual_single_screen_mp", '"player 1 only"')
            coreSettings.setString("tgbdual_audio_output", '"Game Boy #1"')
        elif self.system.Netplay and not self.system.NetplayHostMode and not self.system.NetplayViewerOnly:
            coreSettings.setString("tgbdual_single_screen_mp", '"player 2 only"')
            coreSettings.setString("tgbdual_audio_output", '"Game Boy #2"')
        else:
            coreSettings.setString("tgbdual_single_screen_mp", '"both players"')
            coreSettings.setString("tgbdual_audio_output", '"Game Boy #1"')

    @staticmethod
    def touchscreen() -> bool:
        import pyudev

        context = pyudev.Context()
        for device in context.list_devices(subsystem='input'):
            if device.device_node and "event" in device.device_node:
                if device.properties.get("ID_INPUT_TOUCHSCREEN") == "1":
                    return True

        return False

    # Fill cores configuration
    def fillCoresConfiguration(self):
        settings = self.settings

        # Specific configuration handlers per core
        specificCoreHandlers =\
        {
            "bluemsx" : LibretroCores.configureBlueMSX,
            "dolphin": self.configureDolphin,
            "mame2003_plus": LibretroCores.configureMAME2003plus,
            "mame": self.configureMAME,
            "mame0258": self.configureMAME,
            "mame0278": self.configureMAME,
            "mupen64plus_nx" : LibretroCores.configureMUPEN64,
            "opera": LibretroCores.configureOpera,
            "parallel_n64": LibretroCores.configurePARALLELN64,
            "np2kai" : LibretroCores.configureNPKAI,
            "picodrive" : self.configurePicodrive,
            "swanstation" : self.configureSwanstation,
            "dosbox_pure" : LibretroCores.configureDosBoxPure,
            "flycast": self.configureFlycast,
            "flycast-next": self.configureFlycast,
            "fbneo": self.configureFBNeo,
            "scummvm": self.configureScummvm,
            "hatari": self.configureAtariST,
            "hatarib": self.configureAtariSTNew,
            "melonds": LibretroCores.configureMelonDS,
            "desmume": LibretroCores.configureDesmume,
            "ppsspp": self.configurePPSSPP,
            "pcsx_rearmed": self.configurePCSXRearmed,
            "mednafen_psx_hw": self.configureBeetlePSX,
            "mednafen_psx": self.configureBeetlePSX,
            "gambatte": self.configureGambatte,
            "mednafen_stv" : self.configureSTV,
            "eightyone": self.configureEightyOne,
            "tgbdual": self.configureTGBDual,
        }

        # Get handler and execute
        core = self.system.Core
        if core in specificCoreHandlers:
            function = specificCoreHandlers.get(core)
            function(settings)

        # Specific configuration handlers per system
        specificSystemHandlers =\
        {
            "amiga600"     : LibretroCores.configureAmiga600,
            "amiga1200"    : LibretroCores.configureAmiga1200,
            "amigacdtv"    : LibretroCores.configureAmigaCDTV,
            "amigacd32"    : LibretroCores.configureAmigaCD32,
            "atari5200"    : LibretroCores.configureAtari5200,
            "atari800"     : LibretroCores.configureAtari800,
            "amstradcpc"   : LibretroCores.configureAmstradCPC,
            "gx4000"       : LibretroCores.configureAmstradGX4000,
            "nds"          : self.configureNDS,
            "spectravideo" : LibretroCores.configureSpectravideo,
            "64dd"         : LibretroCores.configure64DD,
            "msx1"         : LibretroCores.configureMsx1,
            "msx2"         : LibretroCores.configureMsx2,
            "msxturbor"    : LibretroCores.configureMsxTurboR,
            "pc98"         : LibretroCores.configureNPKAI,
            "odyssey2"     : LibretroCores.configureOdyssey2,
            "videopacplus" : LibretroCores.configureVideoPacPlus,
            "sg1000"       : LibretroCores.configureSg1000,
            "naomi2"       : self.configureNaomi2,
            "saturn"       : self.configureSaturn,
        }

        # Get handler and execute
        if self.system.Name in specificSystemHandlers:
            function = specificSystemHandlers.get(self.system.Name)
            function(settings)
