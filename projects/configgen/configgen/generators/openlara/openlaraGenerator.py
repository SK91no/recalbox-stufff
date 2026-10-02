#!/usr/bin/env python
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.generators.Generator import Generator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings


class OpenlaraGenerator(Generator):

    controllersFile = recalboxFiles.CONF + '/openlara/gamecontrollerdb.txt'

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        """
        Load, override keys and save back emulator's configuration file
        This way, any modification is kept across emulator launches
        """

        # Create the config file if it doesn't exist first, avoiding a bug on first play
        import os
        if not os.path.exists(self.controllersFile):
            with open(self.controllersFile, 'w'):
                pass

        # Generate game controller database
        with open(self.controllersFile, "w") as f:
            for controller in playersControllers.values():
                f.write(controller.generateSDLGameDBLine() + '\n')

        import os
        romName = os.path.dirname(args.rom)

        # Command line arguments
        commandArray = [recalboxFiles.recalboxBins[system.Emulator],
                        "-d", romName, "-f"]

        # Add extra arguments
        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray, env={"SDL_VIDEO_GL_DRIVER": "/usr/lib/libGLESv2.so", "SDL_VIDEO_EGL_DRIVER": "/usr/lib/libEGL.so"})
