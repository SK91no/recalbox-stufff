#!/usr/bin/env python
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.generators.solarus.solarusControllers import SolarusControllers
from configgen.settings.keyValueSettings import keyValueSettings
import os


class SolarusGenerator(Generator):

    controllersFile = recalboxFiles.HOME + '/.config/solarus/gamecontrollerdb.txt'

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        """
        Load, override keys and save back emulator's configuration file
        This way, any modification is kept across emulator launches
        """

        # Default args
        arguments = ["-fullscreen=yes", "-cursor-visible=no"]

        # Filtering?
        if system.Smooth:
            arguments.append("-bilinear-filtering")

        # Pad configuration
        controllers = SolarusControllers(playersControllers)
        arguments.append("-quit-combo={}+{}".format(controllers.HotKey(), controllers.Start()))

        # Rom
        arguments.append(args.rom)

        # Create the config file if it doesn't exist first
        if not os.path.exists(self.controllersFile):
            os.makedirs(os.path.dirname(self.controllersFile), exist_ok=True)
            with open(self.controllersFile, 'w') as file:
                pass

        # General game controller database
        with open(self.controllersFile, 'w') as f:
            for controller in playersControllers.values():
                f.write(controller.generateSDLGameDBLine() + '\n')

        commandArray = [recalboxFiles.recalboxBins[system.Emulator]]
        commandArray.extend(arguments)

        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray, env={"SDL_GAMECONTROLLERCONFIG": f"{self.controllersFile}"})
