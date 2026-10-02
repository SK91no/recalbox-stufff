from typing import Dict

from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.generators.Generator import Generator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.controllers.inputItem import InputItem


class SdlpopGenerator(Generator):

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        """
        Load, override keys and save back emulator's configuration file
        This way, any modification is kept across emulator launches
        """

        # Generate game controller database
        with open(recalboxFiles.sdlpopControllersFile, "w") as f:
            for controller in playersControllers.values():
                f.write(controller.generateSDLGameDBLine() + '\n')

        # Config file
        from configgen.settings.iniSettings import IniSettings
        settings = IniSettings(recalboxFiles.sdlpopConfigFile, True)
        settings.loadFile(True)\
                .defineBool('true', 'false')

        settings.setString("General", "start_fullscreen", "true")                                   # Fullscreen
        settings.setString("CustomGameplay", "use_custom_options", "true")                          # Use custom options - enables the next line
        settings.setString("CustomGameplay", "start_minutes_left", "-1")                            # Disable 60 minutes time limit
        settings.setString("General", "gamecontrollerdb_file", recalboxFiles.sdlpopControllersFile) # Controllers configuration

        # Save config
        settings.saveFile()

        # Command line arguments
        commandArray = [recalboxFiles.recalboxBins[system.Emulator], args.rom]

        # Add extra arguments
        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray)
