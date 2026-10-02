#!/usr/bin/env python
from typing import List

from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.crt.CRTTypes import CRTAdapter
from configgen.generators.Generator import Generator
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.architecture import Architecture
from configgen.utils.videoMode import createCrtDesktopRes, getCrtResolution, deleteCrtDesktopRes, getCrtResolutionDetail


class SupermodelGenerator(Generator):

    configFile = recalboxFiles.CONF + '/model3/ConfigModel3.ini'

    SECTION_GLOBAL = "Global"

    # -----AUDIO-----

    @staticmethod
    def GetVolume(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        Vol = supermodelSettings.getInt("sound-volume", 100)
        return [f"-sound-volume={Vol}"]

    @staticmethod
    def GetMusicVolume(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        MusicVol = supermodelSettings.getInt("music-volume", 100)
        return [f"-music-volume={MusicVol}"]

    @staticmethod
    def GetBalance(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        balance = supermodelSettings.getInt("balance", 0)
        return [f"-balance={balance}"] if balance != 0 else []

    @staticmethod
    def GetChannels(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        channels = supermodelSettings.getInt("channels", 4)
        return [f"-channels={channels}"] if channels != 0 else []

    @staticmethod
    def GetFlipStereo(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        SwitchStereo = supermodelSettings.getInt("flip-stereo", 1)
        return ["-flip-stereo"] if SwitchStereo == 1 else ["-no-flip-stereo"]

    @staticmethod
    def GetNoSound(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        soundState = supermodelSettings.getInt("no-sound", 0)
        return ["-no-sound"] if soundState == 1 else ["-sound"]

    @staticmethod
    def GetDsb(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        currentDsb = supermodelSettings.getInt("no-dsb", 0)
        return ["-no-dsb"] if currentDsb == 1 else ["-dsb"]

    ## added in version r818 define new or old sound engine
    @staticmethod
    def GetSoundEngine(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        soundType = supermodelSettings.getInt("sound-engine", 1)
        return ["-new-scsp"] if soundType == 0 else ["-legacy-scsp"]

    @staticmethod
    def FindResolution(system: Emulator) -> (int, int, bool):
        # get current resolution.
        rez = getCrtResolutionDetail(system)
        if rez:
            width, height, _ = rez
            return width, height, False
        else:
            supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
            configResolution = supermodelSettings.loadFile(True).getString("resolution", "")
            if configResolution == "auto":
                from configgen.utils.resolutions import ResolutionParser
                resolution = ResolutionParser(system.VideoMode)
                if resolution.isSet and resolution.selfProcess:
                    return resolution.width, resolution.height, Architecture().isPi5
                else:
                    return 0, 0, False
            else:
                return int(configResolution.split(",")[0]), int(configResolution.split(",")[1]), Architecture().isPi5

    @staticmethod
    def GetResolution(system: Emulator) -> List[str]:
        # get current resolution.
        option: List[str] = []
        x, y, _ = SupermodelGenerator.FindResolution(system)
        option.append("-fullscreen")
        option.append("-res={},{}".format(x, y))
        return option

    @staticmethod
    def GetSuperSampling(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        superSampling: int = supermodelSettings.getInt("super-sampling", 1)
        return [f"-ss={superSampling}"]

    # added in version r835 :
    # -wide-bg When wide-screen mode is enabled, also expand the 2D background layer to screen width
    @staticmethod
    def GetScreenRatio(system: Emulator) -> List[str]:
        return ["-wide-screen", "-wide-bg"] if system.WideScreenMode else ["-no-wide-screen"]

    @staticmethod
    def GetUpscale(system: Emulator) -> List[str]:
        if system.CRTAdapter == CRTAdapter.NONE and system.HDMode:
            supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
            supermodelSettings.loadFile(True)
            upscale: int = supermodelSettings.getInt("upscale", 0)
            return ["-upscalemode={}".format(upscale)]
        else:
            return ["-upscalemode=0"]

    @staticmethod
    def GetThrottle(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        throttle: int = supermodelSettings.getInt("no-throttle", 0)
        return ["-no-throttle"] if throttle == 1 else ["-throttle"]

    @staticmethod
    def GetVSync(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        vsync: int = supermodelSettings.getInt("vsync", 1)
        return ["-vsync"] if vsync == 1 else ["-no-vsync"]

    @staticmethod
    def GetTrueHz(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        trueHz: int = supermodelSettings.getInt("true-hz", 0)
        return ["-true-hz"] if trueHz == 1 else []

    @staticmethod
    def GetCrosshairs(_) -> List[str]:
        SetCrosshairs: List[str] = []
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        Crosshairs: int = supermodelSettings.getInt("crosshairs", 0)
        # on default set 3 (for players one and two) if enabled in config file
        if Crosshairs == 1:
            SetCrosshairs.append("-crosshairs=1")
        elif Crosshairs == 2:
            SetCrosshairs.append("-crosshairs=3")
        return SetCrosshairs

    @staticmethod
    def GetRendering(_) -> List[str]:
        Rendering: List[str] = []
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        QuadRendering: int = supermodelSettings.getInt("quad-rendering", 0)
        if QuadRendering == 1:
            Rendering.append("-quad-rendering")
        return Rendering

    @staticmethod
    def GetMultiTexture(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        MultiTexture = supermodelSettings.getInt("multi-texture", 0)
        return ["-multi-texture"] if MultiTexture == 1 else ["-no-multi-texture"]

    @staticmethod
    def GetEngine(_) -> List[str]:
        # Set new3d engine only if you have a nvidia driver else use legacy3d engine
        # Is nVidia driver on?
        import platform
        arch = platform.machine()
        hasNVidia = False
        if arch == "x86_64":
            # check if we have nvidia driver
            import os
            if os.path.exists("/etc/modprobe.d/blacklist.conf"):
                with open("/etc/modprobe.d/blacklist.conf") as f:
                    for line in f:
                        if "blacklist nouveau" in line:
                            hasNVidia = True
                            break
        return [] if hasNVidia else ['-legacy3d']

    # -----CORE-----

    @staticmethod
    def GetShowsFps(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        showfps = supermodelSettings.getInt("show-fps", 0)
        return ["-show-fps"] if showfps == 1 else ["-no-fps"]

    @staticmethod
    def GetThreads(_) -> List[str]:
        Thread: List[str] = []
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        ThreadStatus: int = supermodelSettings.getInt("multi-threading", 0)
        if ThreadStatus == 1:
            Thread.append("-no-gpu-thread")
        elif ThreadStatus == 2:
            Thread.append("-no-threads")
        else:
            Thread.append("-gpu-multi-threaded")
        return Thread

    @staticmethod
    def GetPpcFrequency(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        Frequency: int = supermodelSettings.getInt("ppc-frequency", 50)
        return [f"-ppc-frequency={Frequency}"]

    @staticmethod
    def GetSavesState(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        model3saves = "/recalbox/share/saves/model3/"
        return [f"-load-state={model3saves}"]

    @staticmethod
    def GetLogsPath(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        model3logs = recalboxFiles.CONF + "/model3/Supermodel.log"
        return [f"-log-output={model3logs}"]

    @staticmethod
    def GetLogsLevel(_) -> List[str]:
        supermodelSettings = keyValueSettings(SupermodelGenerator.configFile)
        supermodelSettings.loadFile(True)
        loglevelset = supermodelSettings.getString("log-level", "")
        return [f"-log-level={loglevelset}"]

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        if not system.HasConfigFile:
            # Controllers
            import configgen.generators.supermodel.supermodelControllers as supermodelControllers
            supermodelControllers.generateControllerConfig(self, playersControllers, system, args.rom)
        width, height, useWlr = SupermodelGenerator.FindResolution(system)
        
        commandArray = [recalboxFiles.recalboxBins[system.Emulator], args.rom]

        if system.CRTAdapter != CRTAdapter.NONE:
            createCrtDesktopRes(getCrtResolution(system), system)

        # add other cmd option for more configuration
        # -----AUDIO------
        commandArray.extend(self.GetVolume(system))
        commandArray.extend(self.GetMusicVolume(system))
        commandArray.extend(self.GetBalance(system))
        commandArray.extend(self.GetFlipStereo(system))
        commandArray.extend(self.GetNoSound(system))
        commandArray.extend(self.GetDsb(system))
        commandArray.extend(self.GetSoundEngine(system))
        # -----VIDEO------
        commandArray.extend(self.GetResolution(system))
        commandArray.extend(self.GetSuperSampling(system))
        commandArray.extend(self.GetScreenRatio(system))
        commandArray.extend(self.GetUpscale(system))
        commandArray.extend(self.GetThrottle(system))
        commandArray.extend(self.GetVSync(system))
        commandArray.extend(self.GetCrosshairs(system))
        commandArray.extend(self.GetRendering(system))
        commandArray.extend(self.GetMultiTexture(system))
        commandArray.extend(self.GetEngine(system))
        # -----CORE------
        # commandArray.extend(self.GetShowsFps(system))  # Disabled because FPS show only on status bar
        commandArray.extend(self.GetThreads(system))
        commandArray.extend(self.GetPpcFrequency(system))
        # commandArray.extend(self.GetSavesState(system))  # Disabled because make crash on few games
        commandArray.extend(self.GetLogsPath(system))
        commandArray.extend(self.GetLogsLevel(system))

        if system.HasArgs: commandArray.extend(system.Args)

        if useWlr:
            longString = "wlr-randr --output '' --mode {width}x{height}@60Hz &&".format(width=width, height=height)
            for arg in commandArray:
                longString += " '{}'".format(arg)
            commandArray = ["bash", "-c", longString]
        return Command(videomode=system.VideoMode, array=commandArray, env={
            "MESA_GLSL_VERSION_OVERRIDE": "450",
            "MESA_GL_VERSION_OVERRIDE": "4.5"
        }, postExec=[deleteCrtDesktopRes])
