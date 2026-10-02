#!/usr/bin/env python
from configgen.Command import Command
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.settings.keyValueSettings import keyValueSettings
from pyudev.device._device import Properties
from configgen.utils.Vulkan import Vulkan
from configgen.utils.videoMode import createCrtDesktopRes, getCrtResolution, deleteCrtDesktopRes

class DaphneGenerator(Generator):

    @staticmethod
    def getOrDefault(props: Properties, key: str, default: str) -> str:
        if key in props: return props[key];
        return default

    # Main entry of the module
    # Configure daphne and return a command
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        if not system.HasConfigFile:
            import configgen.generators.daphne.daphneControllers as daphneControllers
            daphneControllers.generateControllerConfig(system, playersControllers)

        import os.path
        romName = os.path.splitext(os.path.basename(args.rom))[0]
        homeDir = args.rom.replace("/" + romName + ".daphne", "");
        bezelFile = args.rom + "/../bezels/" + romName + ".png"
        frameFile = args.rom + "/" + romName + ".txt"
        commandsFile = args.rom + "/" + romName + ".commands"
        singeFile = args.rom + "/" + romName + ".singe"
        # the command to run
        import configgen.recalboxFiles as recalboxFiles
        if os.path.exists(singeFile):
            # for a singe game
            commandArray = [recalboxFiles.recalboxBins[system.Emulator],
                "singe", "vldp",
                "-framefile", frameFile,
                "-fullscreen",
                "-script", singeFile,
                "-retropath",
                "-texturestream",
                "-datadir", recalboxFiles.daphneDatadir,
                "-homedir", homeDir,
                "-singedir", homeDir]

            # If a bezel exists, apply it and resize screen to 4:3
            if os.path.exists(bezelFile):
                commandArray.extend(["-bezel", bezelFile, "-force_aspect_ratio"])

            # Define argument if DolphinBars are found for a specific list of games only
            lst = ['crimepatrol-hd', 'drugwars-hd', 'johnnyrock-hd', 'johnnyrocknoir', 'lbh-hd', 'maddog-hd', 'maddog2-hd', 'spacepirates-hd']
            if romName in lst:
                # Find if there some DolphinBars
                print('[Configgen.LightGun] Seeking for Dolphinbars...')
                import pyudev
                player: int = 0
                mouseIndex: int = 0
                for device in pyudev.Context().list_devices(subsystem="input", ID_INPUT_MOUSE="1"):
                    if device.sys_name.startswith("event"):
                        if self.getOrDefault(device.properties, "ID_SERIAL", "") == "HJZ_Mayflash_Wiimote_PC_Adapter":
                            player += 1
                        print("[Configgen.LightGun] Found mouse #{} : {}".format(mouseIndex, self.getOrDefault(device.properties, "ID_SERIAL", "Unknown")));
                        mouseIndex += 1
                print("[Configgen.LightGun] Found {} DolphinBars among {} Mouses".format(player, mouseIndex))
                if player > 0:
                    commandArray.extend(["-manymouse"])
                else:
                    commandArray.extend(["-grabmouse"])
        else:
            # for a classical game
            commandArray = [recalboxFiles.recalboxBins[system.Emulator],
                romName, "vldp",
                "-framefile", frameFile,
                "-fullscreen",
                "-datadir", recalboxFiles.daphneDatadir,
                "-homedir", homeDir]

            # If a bezel exists, apply it and resize screen to 4:3
            if os.path.exists(bezelFile):
                commandArray.extend(["-bezel", bezelFile, "-force_aspect_ratio"])

        # Use Vulkan if available or use OpenGL
        if Vulkan.vulkanAvailable():
            commandArray.extend(["-vulkan"])
        else:
            commandArray.extend(["-opengl"])

        from configgen.utils.resolutions import ResolutionParser
        resolution = ResolutionParser(system.VideoMode)
        if resolution.isSet and resolution.selfProcess:
            commandArray.extend(["-x", str(resolution.width), "-y", str(resolution.height)])

        if system.CRTEnabled:
            createCrtDesktopRes(getCrtResolution(system), system)

        if system.HasArgs: commandArray.extend(system.Args)

        if os.path.isfile(commandsFile):
            commandArray.extend(open(commandsFile,'r').read().split())
        return Command(videomode=system.VideoMode, array=commandArray, env={"SDL_VIDEO_GL_DRIVER": "/usr/lib/libGLESv2.so", "SDL_VIDEO_EGL_DRIVER": "/usr/lib/libEGL.so"}, postExec=[deleteCrtDesktopRes])
