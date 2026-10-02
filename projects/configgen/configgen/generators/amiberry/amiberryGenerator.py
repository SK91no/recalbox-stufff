#!/usr/bin/env python
from typing import List, Dict

from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.controller import ControllerPerPlayer
from configgen.generators.Generator import Generator
from configgen.generators.amiberry.amiberryConfig import ConfigGenerator
from configgen.generators.amiberry.amiberrySubSystems import SubSystems
from configgen.settings.keyValueSettings import keyValueSettings


class AmiberryGenerator(Generator):

    # Generate ADF Arguments
    @staticmethod
    def getADFArguments(rom: str, system: SubSystems, configFile: ConfigGenerator) -> List[str]:
        from configgen.utils.diskCollector import DiskCollector
        collector = DiskCollector(rom, 4, True)
        configFile.SetFloppies(system, collector.disks)
        return []

    # Generate CDROM Arguments
    @staticmethod
    def getCDROMArguments(rom: str, system: SubSystems, configFile: ConfigGenerator) -> List[str]:
        # Set CD image
        configFile.SetCD(system, rom)
        return []

    # Generate HDFS Arguments
    @staticmethod
    def getHDDFSArguments(rom: str, system: SubSystems, configFile: ConfigGenerator) -> List[str]:
        if system in SubSystems.COMPUTERS:
            # mount an amiga volume DH0 to the mount point given by the rom argument
            configFile.SetHDDFS(system, rom)
            return []
        raise Exception("HDFS not allowed on non-computer devices")

    # Generate HDF Arguments
    @staticmethod
    def getHDFArguments(rom: str, system: SubSystems, configFile: ConfigGenerator) -> List[str]:
        if system in SubSystems.COMPUTERS:
            # mount an amiga volume DH0 using the filename given by the rom argument
            configFile.SetHDF(system, rom)
            return []
        raise Exception("HDF not allowed on non-computer devices")

    # Unknown rom processing
    @staticmethod
    def unknownRomType(rom: str, system: SubSystems, configFile: ConfigGenerator) -> List[str]:
        # Force quit
        raise Exception("Unknown rom type: {}".format(rom))

    # Prepare the WHDBoot folder so Amiberry 8.x's automatic WHDLoad booter
    # (whdload_auto_prefs) can find boot-data.zip, game-data and the kickstart
    # symlinks. Saves are redirected to the recalbox share.
    @staticmethod
    def prepareWHDLoad(system: SubSystems):
        if system not in SubSystems.COMPUTERS:
            raise Exception("WHDL not allowed on non-computer devices")

        import os.path
        import subprocess

        sourceSaveFolder = os.path.join(recalboxFiles.amiberryMountPoint, "whdboot/save-data/Savegames")

        # Copy whdboot structure to the amiberry mount point (boot-data.zip, game-data, ...)
        if os.path.islink(sourceSaveFolder):
            os.remove(sourceSaveFolder)  # Remove soft link
        subprocess.check_output(["cp", "-r", "/usr/share/amiberry/whdboot", recalboxFiles.amiberryMountPoint])

        # Create kickstart symlinks expected by the WHDBoot booter
        from configgen.generators.amiberry.amiberryKickstarts import KickstartManager
        KickstartManager.GenerateWHDSymLinks(os.path.join(recalboxFiles.amiberryMountPoint, "whdboot/save-data/Kickstarts"))

    # Get keyboard layout
    @staticmethod
    def GetKeyboardLayout(recalboxOptions: keyValueSettings):
        # Try to obtain from keyboard layout, then from system language, then fallback to us
        kl = recalboxOptions.getString("system.kblayout", recalboxOptions.getString("system.language", "us")[-2:]).lower()
        return kl

    # return true if the option is considered enabled (for boolean options)
    @staticmethod
    def defined(key: str, dictio: Dict[str, str]):
        return key in dictio and isinstance(dictio[key], str) and len(dictio[key]) > 0

    # Build the Amiberry command (Amiberry 8.x uses GNU-style long options)
    @staticmethod
    def buildCommand(array: List[str], system: Emulator) -> Command:
        return Command(videomode=system.VideoMode, array=array,
                       env={"AMIBERRY_HOME_DIR": recalboxFiles.amiberryMountPoint,
                            # capsimg/IPF plugin: Amiberry 8.x tries "<plugins>/libcapsimage.so",
                            # which is exactly what the libcapsimage package installs in /usr/lib
                            "AMIBERRY_PLUGINS_DIR": "/usr/lib"},
                       cwdPath=recalboxFiles.amiberryMountPoint, postExec=None)

    # Main entry of the module
    # Return command
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        # Get rom type and associated configuration file if any
        from configgen.generators.amiberry.amiberryRomType import RomType
        rom, romType, romHasUAE = RomType.Identify(args.rom)

        # Get subsystem - Force A1200 with WHDL
        subSystem = SubSystems.A1200 if romType == RomType.WHDL else system.Name
        needSlowCPU = system.Name != subSystem

        # Generate global config file (amiberry.conf: host paths, kickstart list, defaults)
        import os.path
        globalOverride = os.path.join(os.path.dirname(rom), ".amiberry.conf")
        from configgen.generators.amiberry.amiberryGlobalConfig import AmiberryGlobalConfig
        globalConfig = AmiberryGlobalConfig(globalOverride,
                                            os.path.join(recalboxFiles.amiberryMountPoint, "conf/amiberry.conf"))
        globalConfig.createGlobalSettings(args.verbose, system.ShaderSet == 'scanlines')

        commandArray = [recalboxFiles.recalboxBins[system.Emulator]]

        # WHDLoad: fully automatic in Amiberry 8.x.
        # Just hand the archive to --autoload, the WHDBoot booter configures the
        # hardware, kickstart and volumes on its own.
        if romType == RomType.WHDL:
            AmiberryGenerator.prepareWHDLoad(subSystem)
            commandArray += ["--autoload", rom, "-G"]
            return AmiberryGenerator.buildCommand(commandArray, system)

        # RP9 all-in-one package: it embeds its own configuration.
        if romType == RomType.PACKAGE:
            if subSystem not in SubSystems.COMPUTERS:
                raise Exception("PACKAGE not allowed on non-computer devices")
            commandArray += ["--config", rom, "-G"]
            return AmiberryGenerator.buildCommand(commandArray, system)

        # Any other rom type: build a default.uae configuration and hand it over.
        settingsFullPath = os.path.join(recalboxFiles.amiberryMountPoint, "conf/default.uae")

        # Prepare configuration file
        romFile, _ = os.path.splitext(rom)
        userSettings = romFile + ".uae" if romHasUAE else None
        overridenSettings = os.path.join(os.path.dirname(rom), ".uaeconfig.uae")
        configFile = ConfigGenerator(settingsFullPath)

        # Load default settings
        configFile.SetDefaults(subSystem)
        configFile.SetDefaultPath(subSystem)
        configFile.SetUI(AmiberryGenerator.GetKeyboardLayout(recalboxOptions), system.ShowFPS)
        configFile.SetInput(subSystem)
        configFile.SetJoystick(subSystem, playersControllers)
        configFile.SetCPU(subSystem, needSlowCPU)
        configFile.SetChipset(subSystem)
        configFile.SetMemory(subSystem)
        configFile.SetGraphics(subSystem)
        configFile.SetSound(subSystem)
        configFile.SetNetwork(romType in (RomType.HDDFS, RomType.HDF))
        configFile.SetFloppies(subSystem, [])
        configFile.SetCD(subSystem, "")
        configFile.SetKickstarts(subSystem, romType)

        # Load overriden settings of current system
        if os.path.exists(overridenSettings):
            configFile.loadConfigFile(overridenSettings)
        # Load user settings
        if userSettings is not None:
            if os.path.exists(userSettings):
                configFile.loadConfigFile(userSettings)

        # Get arguments (and write configuration) regarding the rom type
        switcher = \
        {
            RomType.UNKNOWN: AmiberryGenerator.unknownRomType,
            RomType.DISK: AmiberryGenerator.getADFArguments,
            RomType.CDROM: AmiberryGenerator.getCDROMArguments,
            RomType.HDDFS: AmiberryGenerator.getHDDFSArguments,
            RomType.HDF: AmiberryGenerator.getHDFArguments,
        }
        # Get the function from switcher dictionary
        func = switcher.get(romType)
        # Execute the function and get arguments
        commandArray.extend(func(rom, subSystem, configFile))

        # Save configuration
        configFile.saveConfigFile()

        # Hand the generated configuration over to Amiberry
        commandArray += ["--config", settingsFullPath, "-G"]

        return AmiberryGenerator.buildCommand(commandArray, system)
