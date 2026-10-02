#!/usr/bin/env python
from __future__ import annotations
from typing import Dict, List, Optional, ValuesView

from configgen.controllers.GamepadInfo import GamepadInfo, GamepadType
# Do not simplify - Allow mocking of Input file path
from configgen.controllers.inputItem import InputItem, InputCollection
import configgen.recalboxFiles as recalboxFiles
import libevdev
import pyudev
import re

esInputs = recalboxFiles.esInputs


def getControllerIndex(node_path: str) -> int:
    """
    return the position of the named joystick, the retroarch way
    """
    try:
        context = pyudev.Context()
    except Exception as e:
        print(f"pyudev error: {e}")
        return -1
    index = 0
    for device in context.list_devices(subsystem='input', ID_INPUT_JOYSTICK='1'):
        # skip empty device nodes
        if device.device_node:
            if device.device_node == node_path:
                return index
            # increment only for event device
            if re.search(r"/dev/input/event", device.device_node):
                index += 1
    return -1  # when not found, return -1


class Controller:

    def __init__(self, deviceName: str, typeName: str, gamepadtype: str, guid: str, playerIndex: int, sdlIndex: int, udevIndex: int,
                 inputs: Optional[InputCollection], devicePath: str, axesCount: int, hatsCount: int, buttonCount: int,
                 layout: str = ""):
        self.__type: str = typeName
        # Build the GamepadInfo from the static stick type (es_input.cfg) and
        # the per-game layout overlay (CLI --jammalayoutp1/p2). The invariant
        # "layout honored only on Arcade6/Arcade8" is enforced inside GamepadInfo.
        self.__gamepadtype: GamepadInfo = GamepadInfo(gamepadtype, layout)
        self.__deviceName: str = deviceName
        self.__guid: str = guid
        self.__sdlIndex: int = sdlIndex        # SDL index - starting from 0
        self.__udevIndex: int = udevIndex      # udev index - starting from 0
        self.__playerIndex: int = playerIndex  # Player index - starting from 1
        self.__naturalIndex: int = 0           # Natural index - starting from 0
        self.__devicePath: str = devicePath
        self.__axesCount: int = axesCount
        self.__hatsCount: int = hatsCount
        self.__buttonsCount: int = buttonCount
        self.__inputs: Optional[InputCollection] = inputs if inputs is not None else {}
        self.axisesNumber: List[int] = self.setAxisNumberList() if inputs is not None else []
        self.__inputCapabilities = self._GetInputCapabilities(self.__devicePath)

    # Getter

    @property
    def Type(self) -> str: return self.__type

    @property
    def GamepadType(self) -> GamepadInfo: return self.__gamepadtype

    @property
    def GUID(self) -> str: return self.__guid

    @property
    def DeviceName(self) -> str: return self.__deviceName

    @property
    def DevicePath(self) -> str: return self.__devicePath

    @property
    def ButtonCount(self) -> int: return self.__buttonsCount

    @property
    def HatCount(self) -> int: return self.__hatsCount

    @property
    def AxisCount(self) -> int: return self.__axesCount

    @property
    def PlayerIndex(self) -> int: return self.__playerIndex

    @property
    def SdlIndex(self) -> int: return self.__sdlIndex

    @property
    def UdevIndex(self) -> int: return self.__udevIndex

    @property
    def NaturalIndex(self) -> int: return self.__naturalIndex

    @property
    def HasStart(self) -> bool: return InputItem.ItemStart in self.__inputs
    @property
    def Start(self) -> InputItem: return self.__inputs[InputItem.ItemStart]

    @property
    def HasSelect(self) -> bool: return InputItem.ItemSelect in self.__inputs
    @property
    def Select(self) -> InputItem: return self.__inputs[InputItem.ItemSelect]

    @property
    def HasHotkey(self) -> bool: return InputItem.ItemHotkey in self.__inputs
    @property
    def Hotkey(self) -> InputItem: return self.__inputs[InputItem.ItemHotkey]

    @property
    def HasEast(self) -> bool: return InputItem.ItemEast in self.__inputs
    @property
    def East(self) -> InputItem: return self.__inputs[InputItem.ItemEast]

    @property
    def HasSouth(self) -> bool: return InputItem.ItemSouth in self.__inputs
    @property
    def South(self) -> InputItem: return self.__inputs[InputItem.ItemSouth]

    @property
    def HasNorth(self) -> bool: return InputItem.ItemNorth in self.__inputs
    @property
    def North(self) -> InputItem: return self.__inputs[InputItem.ItemNorth]

    @property
    def HasWest(self) -> bool: return InputItem.ItemWest in self.__inputs
    @property
    def West(self) -> InputItem: return self.__inputs[InputItem.ItemWest]

    @property
    def HasEastPrime(self) -> bool: return InputItem.ItemEastPrime in self.__inputs
    @property
    def EastPrime(self) -> InputItem: return self.__inputs[InputItem.ItemEastPrime]

    @property
    def HasNorthPrime(self) -> bool: return InputItem.ItemNorthPrime in self.__inputs
    @property
    def NorthPrime(self) -> InputItem: return self.__inputs[InputItem.ItemNorthPrime]

    @property
    def L1(self) -> InputItem: return self.__inputs[InputItem.ItemL1]
    @property
    def HasL1(self) -> bool: return InputItem.ItemL1 in self.__inputs

    @property
    def R1(self) -> InputItem: return self.__inputs[InputItem.ItemR1]
    @property
    def HasR1(self) -> bool: return InputItem.ItemR1 in self.__inputs

    @property
    def L2(self) -> InputItem: return self.__inputs[InputItem.ItemL2]
    @property
    def HasL2(self) -> bool: return InputItem.ItemL2 in self.__inputs

    @property
    def R2(self) -> InputItem: return self.__inputs[InputItem.ItemR2]
    @property
    def HasR2(self) -> bool: return InputItem.ItemR2 in self.__inputs

    @property
    def L3(self) -> InputItem: return self.__inputs[InputItem.ItemL3]
    @property
    def HasL3(self) -> bool: return InputItem.ItemL3 in self.__inputs

    @property
    def R3(self) -> InputItem: return self.__inputs[InputItem.ItemR3]
    @property
    def HasR3(self) -> bool: return InputItem.ItemR3 in self.__inputs

    @property
    def Up(self) -> InputItem: return self.__inputs[InputItem.ItemUp]
    @property
    def HasUp(self) -> bool: return InputItem.ItemUp in self.__inputs

    @property
    def Right(self) -> InputItem: return self.__inputs[InputItem.ItemRight]
    @property
    def HasRight(self) -> bool: return InputItem.ItemRight in self.__inputs

    @property
    def Down(self) -> InputItem: return self.__inputs[InputItem.ItemDown]
    @property
    def HasDown(self) -> bool: return InputItem.ItemDown in self.__inputs

    @property
    def Left(self) -> InputItem: return self.__inputs[InputItem.ItemLeft]
    @property
    def HasLeft(self) -> bool: return InputItem.ItemLeft in self.__inputs

    @property
    def Joy1Up(self) -> InputItem: return self.__inputs[InputItem.ItemJoy1Up]
    @property
    def HasJoy1Up(self) -> bool: return InputItem.ItemJoy1Up in self.__inputs

    @property
    def Joy1Right(self) -> InputItem: return self.__inputs[InputItem.ItemJoy1Right]
    @property
    def HasJoy1Right(self) -> bool: return InputItem.ItemJoy1Right in self.__inputs

    @property
    def Joy1Down(self) -> InputItem: return self.__inputs[InputItem.ItemJoy1Down]
    @property
    def HasJoy1Down(self) -> bool: return InputItem.ItemJoy1Down in self.__inputs

    @property
    def Joy1Left(self) -> InputItem: return self.__inputs[InputItem.ItemJoy1Left]
    @property
    def HasJoy1Left(self) -> bool: return InputItem.ItemJoy1Left in self.__inputs

    @property
    def Joy2Up(self) -> InputItem: return self.__inputs[InputItem.ItemJoy2Up]
    @property
    def HasJoy2Up(self) -> bool: return InputItem.ItemJoy2Up in self.__inputs

    @property
    def Joy2Right(self) -> InputItem: return self.__inputs[InputItem.ItemJoy2Right]
    @property
    def HasJoy2Right(self) -> bool: return InputItem.ItemJoy2Right in self.__inputs

    @property
    def Joy2Down(self) -> InputItem: return self.__inputs[InputItem.ItemJoy2Down]
    @property
    def HasJoy2Down(self) -> bool: return InputItem.ItemJoy2Down in self.__inputs

    @property
    def Joy2Left(self) -> InputItem: return self.__inputs[InputItem.ItemJoy2Left]
    @property
    def HasJoy2Left(self) -> bool: return InputItem.ItemJoy2Left in self.__inputs

    @property
    def AvailableInput(self) -> ValuesView[InputItem]: return self.__inputs.values()

    # Unitary access to avoid value injection
    def Input(self, index: int) -> InputItem: return self.__inputs[index]

    def HasInput(self, item: int) -> bool: return item in self.__inputs

    def HasKey(self, item: int) -> bool: return item in self.__inputCapabilities[libevdev.EV_KEY.value]

    OppositeHat: Dict[int, int] = {
         1 :  4,
         2 :  8,
         3 : 12,
         4 :  1,
         5 :  0, # Not possible
         6 :  9,
         7 :  0, # Not possible
         8 :  2,
         9 :  6,
        10 :  0, # Not possible
        11 :  0, # Not possible
        12 :  3,
        13 :  0, # Not possible
        14 :  0, # Not possible
        15 :  0, # Not possible
    }

    def generateSDLGameDBLine(self, platform: bool = True):
        result: str = "{},{},".format(self.__guid, self.__deviceName.replace(',', ' '))
        if platform: result += "platform:Linux,"
        for inputItem in self.__inputs.values():
            sdlName: str = inputItem.GameDBName
            if sdlName == '': continue
            if inputItem.IsAxis:  result += "{}:{}a{},".format(sdlName, '-' if inputItem.Value < 0 else '+', inputItem.Id)
            elif inputItem.IsHat: result += "{}:h{}.{},".format(sdlName, inputItem.Id, inputItem.Value)
            else:                 result += "{}:{}{},".format(sdlName, inputItem.GameDBType, inputItem.Id)

        return result

    def count(self, inputType: int):
        count: int = 0
        for idx, inp in self.__inputs.items():
            if inp.Type == inputType:
                count += 1
        return count

    def setAxisNumberList(self) -> List[int]:
        # First let's find and sort all those available axes in the controller
        # Remember : a pad can have 6 axes and 1 hat, but only 4 axes are mapped
        # All those tricks are just to mimic https://github.com/xbmc/peripheral.joystick/blob/master/src/api/udev/JoystickUdev.cpp#L321-L334
        axises = []
        for idx, inp in self.__inputs.items():
            if inp.IsAxis:
                # As of now our patched SDL2 gives the same axes number to up/down and left/right. But it's wrong
                leftcode = self.__inputs[InputItem.ItemLeft].Code if InputItem.ItemLeft in self.__inputs else -1
                if inp.Item in (InputItem.ItemUp, InputItem.ItemDown) and inp.Code == leftcode:
                    code = inp.Code + 1
                else:
                    code = inp.Code
                if code not in axises:
                    axises.append(code)
        axises.sort()
        # Complete with existing axes but not in es_input.cfg
        # In the case of a 6 axes pad, complete with the 2 unassigned axes
        for i in range(len(axises), int(self.__axesCount)):
            axises.append(None)

        # Now add hats because they are after all axes
        hats = []
        for idx, inp in self.__inputs.items():
            if inp.IsHat:
                # As of now our patched SDL2 gives the same axes number to up/down and left/right. But it's wrong
                leftcode = self.__inputs[InputItem.ItemLeft].Code if InputItem.ItemLeft in self.__inputs else -1
                if inp.Item in (InputItem.ItemUp, InputItem.ItemDown) and inp.Code == leftcode:
                    code = inp.Code + 1
                else:
                    code = inp.Code
                if code not in hats:
                    hats.append(code)
        hats.sort()
        # Time to sort this
        axises.extend(hats)
        return axises

    def getAxisNumber(self, inputItem: InputItem):
        # ~ print self.axisesNumber
        if inputItem.Type not in (InputItem.TypeAxis, InputItem.TypeHat): return None
        if inputItem.IsHat:
            if inputItem.Item in (InputItem.ItemUp, InputItem.ItemDown) and inputItem.Code == self.__inputs[InputItem.ItemLeft].Code:
                code = inputItem.Code + 1
            else:
                code = inputItem.Code
        else:
            code = inputItem.Code
        return self.axisesNumber.index(code)

    def getTotalAxisNumber(self):
        return int(self.__axesCount) + 2 * int(self.__hatsCount)

    @staticmethod
    def __CompositeIdentifierFrom(guid: str, axeCount: int, hatCount: int, buttonCount: int) -> str:
        return "{}-{}:{}:{}".format(guid,
                                       str(axeCount) if axeCount >= 0 else '*',
                                       str(hatCount) if hatCount >= 0 else '*',
                                       str(buttonCount) if buttonCount >= 0 else '*')

    def __CompositeIdentifier(self) -> str:
        return "{}-{}:{}:{}".format(self.__guid, self.__axesCount, self.__hatsCount, self.__buttonsCount)

    def __CompositeAbstractIdentifier(self) -> str:
        return "{}-*:*:*".format(self.__guid)

    # Twins share every identifier above: only their port tells them apart
    @staticmethod
    def __PortBoundIdentifierFrom(compositeId: str, physicalPath: str) -> str:
        return "{}@{}".format(compositeId, physicalPath)

    @staticmethod
    def __HasOpposite(item: InputItem) -> bool:
        return item.Item in (InputItem.ItemJoy1Up, InputItem.ItemJoy1Left,
                             InputItem.ItemJoy2Up, InputItem.ItemJoy2Left)

    @staticmethod
    def __CreateOpposite(item: InputItem) -> (int, InputItem):
        opposites: Dict[int, int] = \
        {
            InputItem.ItemJoy1Up  : InputItem.ItemJoy1Down ,
            InputItem.ItemJoy1Left: InputItem.ItemJoy1Right,
            InputItem.ItemJoy2Up  : InputItem.ItemJoy2Down,
            InputItem.ItemJoy2Left: InputItem.ItemJoy2Right,
        }
        if item.Item in opposites:
            if item.IsAxis:
                opposite = item.Copy(opposites[item.Item], -item.Value)
                return opposite.Item, opposite
            elif item.IsHat:
                opposite = item.Copy(opposites[item.Item], Controller.OppositeHat[item.Value])
                return opposite.Item, opposite
            else:
                print("[Configgen.Controller] Buttons mapped on analog joystick. Opposite calculation aborted")
                return item.Item, item

        raise ValueError

    # Load all controllers from the es_input.cfg
    @staticmethod
    def LoadControllerConfigurationFromFile() -> ControllerCollection:
        controllers: ControllerCollection = {}
        legacyKeys: set = set()
        import xml.etree.ElementTree as ET
        tree = ET.parse(esInputs)
        root = tree.getroot()

        for controller in root.findall(".//inputConfig"):
            gptype = controller.get("gamepadtype")
            print("[Configgen.Controller] Gamepad {} type: {}".format(controller.get("deviceName"), gptype))

            newController = Controller(deviceName=controller.get("deviceName"), typeName=controller.get("type"),
                                       gamepadtype=gptype,
                                       guid=controller.get("deviceGUID"), playerIndex=0, sdlIndex=0, inputs=None,
                                       devicePath="", axesCount=int(controller.get("deviceNbAxes")),
                                       hatsCount=int(controller.get("deviceNbHats")),
                                       buttonCount=int(controller.get("deviceNbButtons")),
                                       udevIndex=0)
            isVersion2: bool = controller.get("version") == "2"
            physicalPath: str = controller.get("devicePhysicalPath") or ""
            if physicalPath:
                controllers[Controller.__PortBoundIdentifierFrom(newController.__CompositeIdentifier(),
                                                                 physicalPath)] = newController
            # A version 2 entry never loses its key to the fossil an old Recalbox left —
            # the frontend no longer matches nameless legacy entries and configgen must
            # agree. A port-bound entry never steals a twin's shared fallback either;
            # everything else keeps the historical last-entry-wins
            for sharedKey in (newController.__CompositeIdentifier(), newController.__CompositeAbstractIdentifier()):
                free: bool = sharedKey not in controllers
                heldByFossil: bool = sharedKey in legacyKeys
                if free or heldByFossil or (isVersion2 and not physicalPath):
                    controllers[sharedKey] = newController
                    if isVersion2: legacyKeys.discard(sharedKey)
                    else: legacyKeys.add(sharedKey)
            for inp in controller.findall("input"):

                # Read input item attributes
                itemName  = inp.get("name" , None)
                itemType  = inp.get("type" , None)
                itemId    = inp.get("id"   , None)
                itemValue = inp.get("value", None)
                itemCode  = inp.get("code" , None)
                if itemName  is None: print("[Controller] Missing name in pad definition");  continue;
                if itemType  is None: print("[Controller] Missing type in pad definition");  continue;
                if itemId    is None: print("[Controller] Missing id in pad definition");    continue;
                if itemValue is None: print("[Controller] Missing value in pad definition"); continue;
                if itemCode  is None: print("[Controller] Missing code in pad definition");  continue;

                # Store input item
                inputItem = InputItem(itemName, itemType, int(itemId), int(itemValue), int(itemCode))
                newController.__inputs[inputItem.Item] = inputItem

                # Create opposite?
                if Controller.__HasOpposite(inputItem):
                    (oppositeItem, itemOpposite) = Controller.__CreateOpposite(inputItem)
                    newController.__inputs[oppositeItem] = itemOpposite

        return controllers

    @staticmethod
    def __LoadStrFromKwargs(key: str, defaultvalue: str, **kwargs) -> str:
        value: Optional[str] = kwargs.get(key, None)
        return value if value is not None else defaultvalue

    @staticmethod
    def __LoadIntFromKwargs(key: str, defaultvalue: int, **kwargs) -> int:
        value: Optional[str] = kwargs.get(key, None)
        return int(value) if value is not None else defaultvalue

    @staticmethod
    def __StoreNaturalIndexes(controllers: ControllerPerPlayer):
        orderedControllers: ControllerCollection = {}
        for controller in controllers.values():
            orderedControllers[controller.DevicePath] = controller
        i = 0
        for key in sorted(orderedControllers):
            controller: Controller = orderedControllers[key]
            controller.__naturalIndex = i
            i += 1

    @staticmethod
    def LoadDemoControllerConfigurations(**kwargs) -> Dict[str, int]:
        startPerEvent: Dict[str, int] = {}

        controllers: ControllerCollection = Controller.LoadControllerConfigurationFromFile()
        for i in range(1, 11):
            nbbuttons: int = Controller.__LoadIntFromKwargs('p{}nbbuttons'.format(i), -1, **kwargs)
            nbhats: int    = Controller.__LoadIntFromKwargs('p{}nbhats'.format(i), -1, **kwargs)
            nbaxes: int    = Controller.__LoadIntFromKwargs('p{}nbaxes'.format(i), -1, **kwargs)
            dev: str       = Controller.__LoadStrFromKwargs('p{}devicepath'.format(i), "missing-device", **kwargs)
            guid: str      = Controller.__LoadStrFromKwargs('p{}guid'.format(i), "missing-guid", **kwargs)
            name: str      = Controller.__LoadStrFromKwargs('p{}name'.format(i), "missong-name", **kwargs)
            index: int     = Controller.__LoadIntFromKwargs('p{}index'.format(i), -1, **kwargs)
            physical: str  = Controller.__LoadStrFromKwargs('p{}physicalpath'.format(i), "", **kwargs)

            udevIndex = getControllerIndex(dev)

            if name:
                print(f"Joystick {name} is set to udev index {udevIndex}")
            newController = Controller.__FindBestControllerConfig(controllers, i, guid, index, udevIndex, name, dev, nbaxes, nbhats, nbbuttons, physicalPath=physical)
            if newController:
                startPerEvent[dev] = newController.__inputs[InputItem.ItemStart].Code

        return startPerEvent

    @staticmethod
    def LoadUserControllerConfigurations(**kwargs) -> ControllerPerPlayer:
        playerControllers: ControllerPerPlayer = {}

        controllers: ControllerCollection = Controller.LoadControllerConfigurationFromFile()
        for i in range(1, 11):
            index: int     = Controller.__LoadIntFromKwargs('p{}index'.format(i), -1, **kwargs)
            name: str      = Controller.__LoadStrFromKwargs('p{}name'.format(i), "missing-name", **kwargs)
            guid: str      = Controller.__LoadStrFromKwargs('p{}guid'.format(i), "missing-guid", **kwargs)
            dev: str       = Controller.__LoadStrFromKwargs('p{}devicepath'.format(i), "missing-device", **kwargs)
            nbaxes: int    = Controller.__LoadIntFromKwargs('p{}nbaxes'.format(i), -1, **kwargs)
            nbhats: int    = Controller.__LoadIntFromKwargs('p{}nbhats'.format(i), -1, **kwargs)
            nbbuttons: int = Controller.__LoadIntFromKwargs('p{}nbbuttons'.format(i), -1, **kwargs)
            physical: str  = Controller.__LoadStrFromKwargs('p{}physicalpath'.format(i), "", **kwargs)

            udevIndex = getControllerIndex(dev)
            # Per-game gameplay overlay sent by the frontend via the CLI
            # (e.g. --jammalayoutp1=neodefault on JAMMA NeoGeo titles).
            # Honored only on Arcade6/Arcade8 sticks (see GamepadInfo).
            layout: str = kwargs.get("jammalayoutp{}".format(i), "")

            if name:
                print(f"[Configgen.Controller] Joystick {name} is set to udev index {udevIndex}")
            newController = Controller.__FindBestControllerConfig(controllers, i, guid, index, udevIndex, name, dev, nbaxes, nbhats, nbbuttons, layout, physical)
            if newController:
                playerControllers[i] = newController
                # 6-button arcade layout reshuffle. Driven by GamepadInfo —
                # see shouldReshuffle6Btn. Two trigger cases:
                #   1. CLI layout == "6btns" (Jamma 5/6-btn panel + non-arcade game)
                #   2. No CLI layout AND system is non-arcade (HDMI + arcade
                #      stick used on a non-arcade game)
                # Arcade systems without CLI layout never reshuffle: fbneo
                # gets RETROPAD_CLASSIC and handles the buttons natively.
                if newController.GamepadType.shouldReshuffle6Btn(kwargs.get("system", "")) and newController.HasL1:
                    print("[Configgen.Controller] Switching to 6-button arcade layout for controller {}".format(newController.DeviceName))
                    # We transform
                    # South East West   to   West North L1
                    # North L1  R1           South East R1
                    south = newController.South
                    east = newController.East
                    west = newController.West
                    north = newController.North
                    l1 = newController.L1
                    newController.__inputs[newController.East.Item] = l1
                    newController.__inputs[newController.South.Item] = north
                    newController.__inputs[newController.North.Item] = east
                    newController.__inputs[newController.West.Item] = south
                    newController.__inputs[newController.L1.Item] = west
                # Naomi family: swap L1/R1 on any arcade stick — this is
                # independent from the 6-button layout, so keep it under the
                # broader isArcade() guard.
                if newController.GamepadType.isArcade() and kwargs.get("system", "") in ["naomigd", "naomi", "atomiswave"] and newController.HasL1 and newController.HasR1:
                    l = newController.L1
                    newController.__inputs[newController.L1.Item] = newController.R1
                    newController.__inputs[newController.R1.Item] = l
            else:
                print(f"[Configgen.Controller] Joystick {name} not found in FindBestControllerConfig, skipping")
        Controller.__StoreNaturalIndexes(playerControllers)

        return playerControllers

    @staticmethod
    def __FindBestControllerConfig(controllers: ControllerCollection, playerIndex: int, guid: str, sdlIndex, udevIndex, deviceName: str,
                                   devicePath: str, axesCount: int, hatsCount: int, buttonsCount: int,
                                   layout: str = "", physicalPath: str = "") -> Optional[Controller]:
        compositeId = Controller.__CompositeIdentifierFrom(guid, axesCount, hatsCount, buttonsCount)
        # An entry reserved to this very port wins over the one every twin shares
        portBoundId = Controller.__PortBoundIdentifierFrom(compositeId, physicalPath) if physicalPath else ""
        controller = controllers.get(portBoundId) or controllers.get(compositeId)
        if controller is None:
            return None
        # gamepadtype is the stick Type string from es_input.cfg; layout is
        # the per-game overlay forwarded from the CLI (jammalayoutpN).
        return Controller(deviceName=deviceName, typeName=controller.Type,
                          gamepadtype=controller.GamepadType.Type.value, layout=layout,
                          guid=guid, playerIndex=playerIndex, sdlIndex=sdlIndex, inputs=controller.__inputs,
                          devicePath=devicePath, axesCount=axesCount, hatsCount=hatsCount,
                          buttonCount=buttonsCount, udevIndex=udevIndex)

    @staticmethod
    def GenerateSDLGameDatabase(controllers, outputFile ="/tmp/gamecontrollerdb.txt"):
        finalData = ["# Recalbox controller list"]
        for idx, controller in controllers.items(): finalData.append(controller.generateSDLGameDBLine())
        with open(outputFile, "w") as text_file: text_file.write("\n".join(finalData))
        return outputFile

    @staticmethod
    def _GetInputCapabilities(path: str) -> dict:
        if not path:
            return {}
        capabilities = {}
        try:
            with open(path, "rb") as fd:
                dev = libevdev.Device(fd)
                for t, cs in dev.evbits.items():
                    capabilities[t.value] = []
                    for c in cs:
                        capabilities[t.value].append(c.value)
        except Exception as e:
            print(e)
        return capabilities

# Type def for convenience
ControllerCollection = Dict[str, Controller]
ControllerPerPlayer = Dict[int, Controller]
