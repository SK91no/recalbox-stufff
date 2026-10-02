#!/usr/bin/env python3
from typing import Dict, IO

import configgen.recalboxFiles as recalboxFiles
from configgen.Emulator import Emulator
from configgen.controllers.inputItem import InputItem
from configgen.controllers.controller import ControllerPerPlayer
from configgen.settings.keyValueSettings import keyValueSettings

configPath = recalboxFiles.CONF + '/dolphin-emu'

hotkeysCombo: Dict[int, str] =\
{
    InputItem.ItemSouth:      "Stop",  # to unify the unique combination of Recalbox
#    "b":      "Toggle Pause",
    InputItem.ItemL1:     "Take Screenshot",
    InputItem.ItemStart:  "Exit",
    InputItem.ItemEast:      "Reset",
    InputItem.ItemWest:      "Save to selected slot",
    InputItem.ItemNorth:      "Load from selected slot",
#    "r2":     "Stop",
    InputItem.ItemR2:     "Toggle Pause",
    InputItem.ItemUp:     "Select State Slot 1",
    InputItem.ItemDown:   "Select State Slot 2",
    InputItem.ItemLeft:   "Decrease Emulation Speed",
    InputItem.ItemRight:  "Increase Emulation Speed"
}

hotkeysXboxCombo: Dict[int, str] =\
{
    InputItem.ItemSouth:      "General/Stop",  # to unify the unique combination of Recalbox
    #    "b":      "Toggle Pause",
    InputItem.ItemL1:     "General/Take Screenshot",
    InputItem.ItemStart:  "General/Exit",
    InputItem.ItemEast:      "General/Reset",
    InputItem.ItemWest:      "Save State/Save to selected slot",
    InputItem.ItemNorth:      "Load State/Load from selected slot",
    #    "r2":     "Stop",
    InputItem.ItemR2:     "General/Toggle Pause",
    InputItem.ItemUp:     "Select State/Select State Slot 1",
    InputItem.ItemDown:   "Select State/Select State Slot 2",
    InputItem.ItemLeft:   "Emulation Speed/Decrease Emulation Speed",
    InputItem.ItemRight:  "Emulation Speed/Increase Emulation Speed"
}

hotkeysXboxComboValues: Dict[int, str] =\
{
    InputItem.ItemSouth:      "SOUTH",  # to unify the unique combination of Recalbox
    #    "b":      "Toggle Pause",
    InputItem.ItemL1:     "TL",
    InputItem.ItemStart:  "START",
    InputItem.ItemEast:      "EAST",
    InputItem.ItemWest:      "NORTH",
    InputItem.ItemNorth:      "WEST",
    #    "r2":     "Stop",
    InputItem.ItemR2:     "`Full Axis 5+`",
    InputItem.ItemUp:     "`Axis 7+`",
    InputItem.ItemDown:   "`Axis 7-`",
    InputItem.ItemLeft:   "`Axis 6-`",
    InputItem.ItemRight:  "`Axis 6+`"
}

# List here all Wii and WiiWare games which plays sideways only
sidewaysWiiGames = [
    'W8CPXS', 'W8CEXS', 'W8CJJF', 'XHVEXS',            # Bit.Trip Core
    'WRUPXS', 'WRUEXS', 'WRNJJF', 'XHXEXS',            # Bit.Trip Runner
    'WFLE01', 'WFLP01', 'XHIP01', 'XHIE01',            # Fluidity
    'SUKE01', 'SUKJ01', 'SUKP01', 'SUKK01',            # Kirby's Adventure Wii
    'RK5E01', 'RK5P01', 'RK5J01', 'RK5K01',            # Kirby's Epic Yarn
    'WR9E08', 'WR9P08', 'WR9J08',                      # Mega Man 9
    'WRXE08', 'WRXP08', 'WRXJ08',                      # Mega Man 10
    'SMNP01', 'SMNE01', 'SMNK01', 'SMNW01', 'SMNJ01',  # New Super Mario Bros Wii
    'R8PE01', 'R8PP01', 'R8PK01', 'R8PJ01',            # Super Paper Mario Wii
    'RWLP01', 'RWLE01', 'RWLJ01', 'RWLK01',            # Wario Ware: The Shake Dimension (Shake It!)
]

# List all first Game ID letter to recognize Virtual Console platform
wiiVirtualConsoles = [
    'C',  # Commodore 64
    'E',  # Arcade & NeoGeo
    'F',  # Famicom / NES
    'J',  # Super Famicom / SNES
    'L',  # Master System
    'M',  # Genesis / Mega Drive
    'N',  # Nintendo 64
    'P',  # TurboGrafx 16 / PC Engine
    'Q',  # TurboGrafx CD / PC Engine CD
    'X',  # MSX
    # The following are not useful, listed anyway in case this is needed in the future
    # 'H',  # Wii Channels
    # 'R',  # Wii DVD
    # 'W',  # WiiWare
]

# Create the controller configuration file
def generateControllerConfig(system: Emulator, playersControllers: ControllerPerPlayer, recalboxOptions: keyValueSettings, gameid: str):
    generateHotkeys(playersControllers)
    if system.Name == "wii":
        realWiimotes: bool = recalboxOptions.getBool("global.realwiimotes", recalboxOptions.getBool("wii.realwiimotes", False))
        if realWiimotes:
            generateControllerConfigRealwiimotes("WiimoteNew.ini", "Wiimote")
        else:
            generateControllerConfigEmulatedWiimotes(playersControllers, system, gameid)
    elif system.Name == "gamecube":
            generateControllerConfigGamecube(playersControllers, system)
    else:
        raise ValueError("Invalid system name : '" + system.Name + "'")

def generateControllerConfigEmulatedWiimotes(playersControllers: ControllerPerPlayer, system: Emulator, gameid: str | None):
    if gameid and (gameid in sidewaysWiiGames or gameid[0].upper() in wiiVirtualConsoles):
        if gameid[0].upper() == "J":
            # SNES virtual console games requires an classic controller as an extension
            wiiMapping: Dict[int, str] = \
            {
                InputItem.ItemEast: 'Classic/Buttons/A',
                InputItem.ItemSouth: 'Classic/Buttons/B',
                InputItem.ItemNorth: 'Classic/Buttons/X',
                InputItem.ItemWest: 'Classic/Buttons/Y',
                InputItem.ItemL1: 'Classic/Buttons/ZL',
                InputItem.ItemR1: 'Classic/Buttons/ZR',
                InputItem.ItemSelect: 'Classic/Buttons/-',
                InputItem.ItemStart: 'Classic/Buttons/+',
                InputItem.ItemHotkey: 'Classic/Buttons/Home',
                InputItem.ItemUp: 'Classic/D-Pad/Up',
                InputItem.ItemDown: 'Classic/D-Pad/Down',
                InputItem.ItemLeft: 'Classic/D-Pad/Left',
                InputItem.ItemRight: 'Classic/D-Pad/Right'
            }
        else:
            wiiMapping: Dict[int, str] = \
            {
                InputItem.ItemEast: 'Buttons/2',
                InputItem.ItemSouth: 'Buttons/1',
                InputItem.ItemNorth: 'Buttons/A',
                InputItem.ItemWest: 'Buttons/B',
                InputItem.ItemL1: 'Tilt/Left',
                InputItem.ItemR1: 'Tilt/Right',
                InputItem.ItemSelect: 'Buttons/-',
                InputItem.ItemStart: 'Buttons/+',
                InputItem.ItemHotkey: 'Buttons/Home',
                InputItem.ItemR2: 'Shake/Z',
                InputItem.ItemJoy2Up: 'IR/Up',
                InputItem.ItemJoy2Left: 'IR/Left',
                InputItem.ItemJoy2Down: 'IR/Down',
                InputItem.ItemJoy2Right: 'IR/Right',
                InputItem.ItemUp: 'D-Pad/Up',
                InputItem.ItemDown: 'D-Pad/Down',
                InputItem.ItemLeft: 'D-Pad/Left',
                InputItem.ItemRight: 'D-Pad/Right'
            }
    else:
        wiiMapping: Dict[int, str] = \
        {
            InputItem.ItemEast:         'Buttons/2',
            InputItem.ItemSouth:         'Buttons/A',
            InputItem.ItemNorth:         'Buttons/1',
            InputItem.ItemWest:         'Buttons/B',
            InputItem.ItemL1:        'Nunchuk/Buttons/Z',
            InputItem.ItemR1:        'Nunchuk/Buttons/C',
            InputItem.ItemSelect:    'Buttons/-',
            InputItem.ItemStart:     'Buttons/+',
            InputItem.ItemHotkey:    'Buttons/Home',
            InputItem.ItemR2:        'Shake/Z',
            InputItem.ItemJoy1Up:    'Nunchuk/Stick/Up',
            InputItem.ItemJoy1Left:  'Nunchuk/Stick/Left',
            InputItem.ItemJoy1Down:  'Nunchuk/Stick/Down',
            InputItem.ItemJoy1Right: 'Nunchuk/Stick/Right',
            InputItem.ItemJoy2Up:    'IR/Up',
            InputItem.ItemJoy2Left:  'IR/Left',
            InputItem.ItemJoy2Down:  'IR/Down',
            InputItem.ItemJoy2Right: 'IR/Right',
            InputItem.ItemUp:        'D-Pad/Up',
            InputItem.ItemDown:      'D-Pad/Down',
            InputItem.ItemLeft:      'D-Pad/Left',
            InputItem.ItemRight:     'D-Pad/Right'
        }

    generateControllerConfigAny(playersControllers, "WiimoteNew.ini", "Wiimote", wiiMapping, system, gameid)

def generateControllerConfigGamecube(playersControllers: ControllerPerPlayer, system: Emulator):

    gamecubeMapping: Dict[int, str] = \
    {
        InputItem.ItemEast:         'Buttons/A',
        InputItem.ItemSouth:         'Buttons/B',
        InputItem.ItemNorth:         'Buttons/X',
        InputItem.ItemWest:         'Buttons/Y',
        InputItem.ItemR1:        'Buttons/Z',
        InputItem.ItemStart:     'Buttons/Start',
        InputItem.ItemHotkey:    'Buttons/Hotkey',
        InputItem.ItemL2:        'Triggers/L',
        InputItem.ItemR2:        'Triggers/R',
        InputItem.ItemUp:        'D-Pad/Up',
        InputItem.ItemDown:      'D-Pad/Down',
        InputItem.ItemLeft:      'D-Pad/Left',
        InputItem.ItemRight:     'D-Pad/Right',
        InputItem.ItemJoy1Up:    'Main Stick/Up',
        InputItem.ItemJoy1Left:  'Main Stick/Left',
        InputItem.ItemJoy1Down:  'Main Stick/Down',
        InputItem.ItemJoy1Right: 'Main Stick/Right',
        InputItem.ItemJoy2Up:    'C-Stick/Up',
        InputItem.ItemJoy2Left:  'C-Stick/Left',
        InputItem.ItemJoy2Down:  'C-Stick/Down',
        InputItem.ItemJoy2Right: 'C-Stick/Right'
    }

    generateControllerConfigAny(playersControllers, "GCPadNew.ini", "GCPad", gamecubeMapping, system)

def generateControllerConfigRealwiimotes(filename: str, anyDefKey: str):
    configFileName = "{}/{}".format(configPath, filename)
    f = open(configFileName, "w")
    nbPlayer = 1
    while nbPlayer <= 4:
        f.write("[" + anyDefKey + str(nbPlayer) + "]" + "\n")
        f.write("Source = 2\n")
        nbPlayer += 1
    # f.write
    f.close()

def EvdevGetJoystickName(path: str) -> str:
    import fcntl
    with open(path) as fd:
        return fcntl.ioctl(fd, 0x80804506, bytes(128)).decode('utf-8').rstrip().replace('\x00', '')

def getControllerName(device: str):
    import os
    sysFile = "/sys/class/input/{}/device/name".format(os.path.basename(device))
    if os.path.isfile(sysFile):
        with open(sysFile, "r") as f:
            return f.readline().rstrip("\n")
    return ""

def generateControllerConfigAny(playersControllers: ControllerPerPlayer, filename: str, anyDefKey: str, anyMapping: Dict[int, str], system: Emulator, gameid: str = ''):
    configFileName = "{}/{}".format(configPath, filename)
    f = open(configFileName, "w")
    nbPlayer = 1

    # in case of two pads having the same name, dolphin wants a number to handle this
    double_pads = dict()

    for playercontroller in playersControllers:
        # handle x pads having the same name
        pad = playersControllers[playercontroller]
        if pad.DeviceName in double_pads:
            nsamepad = double_pads[pad.DeviceName]
        else:
            nsamepad = 0
        double_pads[pad.DeviceName] = nsamepad + 1

        # Handle first if there is some existing profile file
        from pathlib import Path

        profilePath: Path | None = None

        if system.Name == "gamecube":
            profilePath = Path(configPath) / 'Profiles/GCPad'

        if system.Name == "wii":
            profilePath = Path(configPath) / 'Profiles/Wiimote'

        f.write("[" + anyDefKey + str(nbPlayer) + "]" + "\n")
        f.write('Device = "evdev/' + str(nsamepad) + '/' + EvdevGetJoystickName(pad.DevicePath) + '"\n')

        profileApplied = False

        if profilePath is not None:
            for profileFile in profilePath.glob("*.ini"):
                from configgen.settings.iniSettings import IniSettings
                settings = IniSettings(str(profileFile), True)
                settings.loadFile(True)

                for key, val in settings.items("Profile"):
                    if key != "Device":
                        f.write(f"{key} = {val}\n")

                profileApplied = True
                break

        if system.Name == "wii":
            if gameid and (gameid in sidewaysWiiGames or gameid[0].upper() in wiiVirtualConsoles):
                # We need to disconnect the nunchuk, requires only horizontal Wiimote
                # Required by a few games and all Virtual Console games
                f.write("Options/Sideways Wiimote = True" + "\n")
                if gameid[0].upper() == 'J':
                    # Attach classic controller to Wiimote for SNES Virtual Console
                    f.write("Extension = Classic" + "\n")
            else:
                f.write("Extension = Nunchuk" + "\n")
            f.write("IR/Center = 15.000000000000000" + "\n")
            f.write("IR/Height = 85.000000000000000" + "\n")
            f.write("Nunchuk/Stick/Dead Zone = 25.000000000000000" + "\n")
        elif system.Name == "gamecube":
            f.write("Main Stick/Dead Zone = 25.000000000000000" + "\n")
            f.write("C-Stick/Dead Zone = 25.000000000000000" + "\n")

        if system.Rumble:
            f.write("Rumble/Motor = Sine|Square|Triangle|Strong|Weak" + "\n")

        if not profileApplied:
            for inp in pad.AvailableInput:

                if inp.Item in anyMapping:
                    keyname = anyMapping[inp.Item]
                    # write the configuration for this key
                    writeKey(f, keyname, inp, pad.AxisCount)
                    # write the 2nd part
                    #if inp.IsAnalogJoystick:
                    #    writeKey(f, anyReverseAxes[keyname], inp, pad.AxisCount, True)

        nbPlayer += 1
    f.close()

def writeKey(f: IO, keyname: str, inputItem: InputItem, inputGlobalId: int):
    f.write(keyname + " = `")
    if inputItem.IsButton:
        f.write("Button " + str(inputItem.Id))
    elif inputItem.IsHat:
        f.write("Axis " + str(inputGlobalId + (1 if inputItem.Value in (1, 4) else 0)) + ('-' if inputItem.Value in (1, 8) else '+'))
    elif inputItem.IsAxis:
        f.write("Axis " + str(inputItem.Id) + ('+' if inputItem.Value > 0 else '-'))
    f.write("`\n")


def generateHotkeys(playersControllers: ControllerPerPlayer):
    player1 = None
    iniValues: Dict[str, str] = {}

    # Find player 1
    for idx, playerController in playersControllers.items():
        if playerController.PlayerIndex == 1:
            print("P1 found")
            player1 = playerController
            break

    if player1 is None:
        print("no P1")
        raise ValueError("Couldn't find Player 1 input config")

    # Read its inputs, get the hotkey
    if player1.DeviceName not in getControllerName(player1.DevicePath):
        # Mostly sure it's an Xbox-recognized controller
        HK: str = "MODE" if player1.HasHotkey else -1
    else:
        # Non-Xbox controller
        HK: int = player1.Hotkey.Id if player1.HasHotkey else -1

        if HK < 0:
            print("no HK")
            raise ValueError("Couldn't find Player 1 hotkey")

    # Now generate the hotkeys
    if player1.DeviceName not in getControllerName(player1.DevicePath):
        for inputItem in player1.AvailableInput:
            if inputItem.Item in hotkeysXboxCombo:
                propertyName = "{}".format(hotkeysXboxCombo[inputItem.Item])
                print(propertyName)
                propertyValue = "@({}+{})".format(HK, hotkeysXboxComboValues[inputItem.Item])
                iniValues[propertyName] = propertyValue

        iniValues["Device"] = '"evdev/0/{}"'.format(getControllerName(player1.DevicePath))
        # Prepare the ini write
        iniSections = {"Hotkeys": iniValues}
    else:
        for inputItem in player1.AvailableInput:
            if inputItem.Item in hotkeysCombo:
                propertyName = "Keys/{}".format(hotkeysCombo[inputItem.Item])
                print(propertyName)
                propertyValue = "`Button {}` & `Button {}`".format(HK, inputItem.Id)
                iniValues[propertyName] = propertyValue
        iniValues["Device"] = '"evdev/0/{}"'.format(player1.DeviceName)
        # Prepare the ini write
        iniSections = {"Hotkeys1": iniValues}
    writeIniFile(configPath + '/Hotkeys.ini', iniSections)


def writeIniFile(filename: str, sectionsAndValues: Dict[str, Dict[str, str]]):
    # filename: file to write
    # sectionsAndValues: a dict indexed on sections on the ini. Each section has a dict of propertyName: propertyValue
    from configgen.settings.iniSettings import IniSettings
    config = IniSettings(filename, True)

    # Write dynamic config
    for section, values in sectionsAndValues.items():
        for propertyName, propertyValue in values.items():
            config.setString(section, propertyName, propertyValue)

    # Open file
    config.saveFile()
