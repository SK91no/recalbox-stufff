#!/usr/bin/env python
from configgen.Command import Command
import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.crt.CRTTypes import CRTVideoStandard
from configgen.generators.Generator import Generator, ControllerPerPlayer
from configgen.settings import keyValueSettings
import os

from configgen.utils.videoMode import deleteCrtDesktopRes
from configgen.utils.architecture import Architecture

_BOARD_PLATFORM_CODEC = {
    "rpi5_64":  ("v4l2drm", "hevc"),
    "rpi4_64":  ("v4l2drm", "hevc"),
    "rpizero2": ("mmal",    "h264"),
    "rpi3":     ("mmal",    "h264"),
    "x86_64":   ("auto",    "hevc"),
    "unknown":  ("auto",    "hevc"),
}


def _get_platform_codec() -> tuple[str, str]:
    _BOARD_ID = Architecture().Architecture
    result = _BOARD_PLATFORM_CODEC.get(_BOARD_ID)
    if result is None:
        for key, val in _BOARD_PLATFORM_CODEC.items():
            if _BOARD_ID == key:
                result = val
                break
    return result or ("auto", "hevc")


def _sdl_guid_to_moonlight_guid(sdl_guid: str) -> str:
    if len(sdl_guid) != 32:
        return sdl_guid
    return sdl_guid[:4] + '0000' + sdl_guid[8:]


def _generate_moonlight_db(controllers, output_file: str) -> str:
    lines = ["# Recalbox controller list (moonlight-embedded GUID format)"]
    for _, controller in controllers.items():
        ml_guid = _sdl_guid_to_moonlight_guid(controller.GUID)
        line    = controller.generateSDLGameDBLine(platform=True)
        line    = line.replace(controller.GUID, ml_guid, 1)
        print("[MoonlightGenerator] {}: {} -> {}".format(
              controller.DeviceName, controller.GUID, ml_guid))
        lines.append(line)

    with open(output_file, 'w') as f:
        f.write('\n'.join(lines) + '\n')
    return output_file


class MoonlightGenerator(Generator):

    # Main entry of the module
    # Configure moonlight and return a command
    def generate(self, system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, args) -> Command:
        outputFile = recalboxFiles.moonlightCustom + '/gamecontrollerdb.txt'
        from configgen.utils.videoMode import createCrtDesktopRes
        if system.CRTEnabled:
            if system.CRTVideoStandard == CRTVideoStandard.PAL or system.CRTVideoStandard == CRTVideoStandard.AUTO:
                createCrtDesktopRes("768x576", system)
            if system.CRTVideoStandard == CRTVideoStandard.NTSC:
                createCrtDesktopRes("640x480", system)

        _generate_moonlight_db(playersControllers, outputFile)
        (gameName, argument) = self.getRealGameNameAndArgument(args.rom)

        if self.isEmbedded():
            platform_str, codec_str = _get_platform_codec()
            commandArray = [recalboxFiles.recalboxBins[system.Emulator], 'stream', '-config', argument]
            commandArray.extend(["-platform", platform_str])
            commandArray.extend(["-codec", codec_str])
        else:
            commandArray = [recalboxFiles.recalboxBins[system.Emulator], 'stream', argument]

        if system.HasArgs:
            commandArray.extend(system.Args)

        if self.isEmbedded():
            commandArray.append('-app')
        commandArray.append(gameName)

        return Command(
            videomode='default',
            array=commandArray,
            env={"XDG_DATA_DIRS": recalboxFiles.CONF},
            postExec=[deleteCrtDesktopRes]
        )

    def getRealGameNameAndArgument(self, rom: str) -> (str, str):
        import os
        romName = os.path.splitext(os.path.basename(rom))[0]
        f = open(recalboxFiles.moonlightGamelist, 'r')
        gfeGame = None

        for line in f:
            if self.isEmbedded():
                try:
                    gfeRom, gfeGame, confFile = line.rstrip().split(';')
                except ValueError:
                    gfeRom, gfeGame = line.split(';')
                    confFile = recalboxFiles.moonlightConfig
                if gfeRom == romName:
                    f.close()
                    return [gfeGame, confFile]
            else:
                gfeRom, gfeGame, confFile = line.rstrip().split(';')
                streamHost = self.getStreamHostFromConfig(confFile)
                if gfeRom == romName:
                    f.close()
                    return [gfeGame, streamHost]

        return gfeGame, recalboxFiles.moonlightConfig

    @staticmethod
    def getStreamHostFromConfig(configFile: str) -> str:
        import re
        with open(configFile, 'r') as fp:
            for line in fp:
                z = re.match(r"^address\s*=\s*(.*)", line)
                if z:
                    return z.group(1)
        raise "IP not found"

    @staticmethod
    def isEmbedded() -> bool:
        return not os.path.exists(recalboxFiles.moonlightIsQT)
