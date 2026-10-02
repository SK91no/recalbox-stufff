from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.settings.keyValueSettings import keyValueSettings


class VvvvvvGenerator(Generator):
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args,) -> Command:
        """
        Load, override keys and save back emulator's configuration file
        This way, any modification is kept accross emulator launhes
        """

        commandArray = [
            recalboxFiles.recalboxBins[system.Emulator],
            "-basedir",
            "/recalbox/share/saves/vvvvvv/",
            "-assets",
            "/recalbox/share/roms/ports/vvvvvv/data.zip",
        ]

        if system.HasArgs:
            commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray)
