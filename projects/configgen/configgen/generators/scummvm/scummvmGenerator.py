#!/usr/bin/env python
from configgen.Command import Command
from configgen.Emulator import Emulator
from configgen.controllers.controller import Controller, ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.settings.iniSettings import IniSettings
from configgen.utils.videoMode import deleteCrtDesktopRes


class ScummVMGenerator(Generator):

    # Main entry of the module
    # Return scummvm command
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        # Create a temporary gamecontrollerdb.txt file with controllers mapping
        Controller.GenerateSDLGameDatabase(playersControllers, "/tmp/gamecontrollerdb.txt")

        # Find rom path
        import os.path
        import configgen.recalboxFiles as recalboxFiles
        from configgen.utils.videoMode import createCrtDesktopRes, getCrtResolution
        rez = getCrtResolution(system)
        createCrtDesktopRes(rez, system)

        language = None
        subtitles = "true"

        if os.path.exists(recalboxFiles.HOME + '/configs/recalbox/tmp-scummvm.ini'):
            settings = keyValueSettings(recalboxFiles.HOME + '/configs/recalbox/tmp-scummvm.ini', True).loadFile(False)
            subtitles = settings.getString("subtitles", "true")

        if os.path.isdir(args.rom):
            # rom is a directory: must contains a <game name>.scummvm/.residualvm file
            romPath = args.rom
            romExt = os.path.splitext(romPath)[1]
            import glob

            romFile = glob.glob(romPath + "/*" + romExt)[0]
            romName = os.path.splitext(os.path.basename(romFile))[0]
            engine = False
            if os.path.isfile(romFile):
                with open(romFile, 'r') as f:
                    s = str(f.read())
                    if len(s.split(":")) > 1:
                        engine = s.split(":")[0]
                        romName = engine + ":" + romName
                    if len(s.split(":")) > 2:
                        language = s.split(":")[2].rstrip()
        else:
            # rom is a file: split in directory and file name
            romPath = os.path.dirname(args.rom)
            romName = os.path.splitext(os.path.basename(args.rom))[0]

        smooth = "--filtering" if system.Smooth else "--no-filtering"
        commandArray = [recalboxFiles.recalboxBins[system.Emulator],
                        "--fullscreen",
                        "--joystick=0",
                        smooth,
                        "--extrapath=/usr/share/scummvm",
                        "--savepath=" + recalboxFiles.scummvmSaves,
                        "--path=" "{}" "".format(romPath)]
        if system.ShaderSet == 'scanlines':
            commandArray.append("--gfx-mode=DotMatrix")

        if subtitles == "true":
            commandArray.append("--subtitles")

        if language:
            commandArray.append("--language={}".format(language))

        if system.HasArgs:
            commandArray.extend(system.Args)

        if system.IntegerScale:
            commandArray.append("--stretch-mode=pixel-perfect")

        commandArray.append("""{}""".format(romName))

        return Command(
            videomode="default",
            array=commandArray,
            env={
                "SDL_VIDEO_GL_DRIVER": "/usr/lib/libGLESv2.so",
                "SDL_VIDEO_EGL_DRIVER": "/usr/lib/libEGL.so",
                "SDL_RENDER_VSYNC": "1",
            },
            postExec=[deleteCrtDesktopRes]
        )
