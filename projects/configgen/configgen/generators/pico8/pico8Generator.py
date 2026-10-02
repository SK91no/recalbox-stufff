from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.generators.Generator import Generator, ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings
import os
import shutil

class Pico8Generator(Generator):

    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:

        # Copy binary file and run the binary with chmod
        if not os.path.exists("/overlay/emulator/pico-8/pico8"):
            if not os.path.exists("/overlay/emulator"):
                os.makedirs("/overlay/emulator")
            shutil.move("/recalbox/share/bios/pico-8", "/overlay/emulator")
            os.rename("/overlay/emulator/pico-8/pico8_64", "/overlay/emulator/pico-8/pico8")
        os.chmod("/overlay/emulator/pico-8/pico8", 0o775)

        # Start emulator
        commandArray = [recalboxFiles.recalboxBins[system.Emulator], "-run", args.rom]
        commandArray.extend(["-windowed", "0"]) # Windowed mode off

        # Screenshot
        commandArray.extend(["-desktop", "/recalbox/share/screenshots"])

        # SPLORE is a built-in utility for browsing and organising both local and bbs (online) cartridges
        commandArray.extend(["-root_path", "/recalbox/share/roms/pico8"])
        romPath: str = args.rom.lower()
        if "splore.p8" in romPath:  commandArray.append("-splore")

        # Add extra arguments
        if system.HasArgs: commandArray.extend(system.Args)

        return Command(videomode=system.VideoMode, array=commandArray)