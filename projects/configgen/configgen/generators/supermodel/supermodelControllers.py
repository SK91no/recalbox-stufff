#!/usr/bin/env python
from typing import Dict

from configgen.Emulator import Emulator
from configgen.controllers.controller import Controller, InputItem
from configgen.controllers.controller import ControllerPerPlayer
import configgen.recalboxFiles as recalboxFiles
from configgen.crtswitchres.CRTTypes import CRTAdapter


# A DolphinBar in mouse mode reports the wiimote as a mouse, aiming included.
def hasDolphinBar() -> bool:
    try:
        import pyudev
        for device in pyudev.Context().list_devices(subsystem="input", ID_INPUT_MOUSE="1"):
            if device.sys_name.startswith("event") \
               and device.properties.get("ID_SERIAL", "") == "HJZ_Mayflash_Wiimote_PC_Adapter":
                return True
    except Exception:
        pass
    return False


def generateControllerConfig(self, playersControllers: ControllerPerPlayer, system: Emulator, rom: str = ""):

    configFile = recalboxFiles.CONF + '/model3/ConfigModel3.ini'
    controlsFile = recalboxFiles.CONF + '/model3/Supermodel.ini'

    ################ Mapping Conversion Supermodel to Recalbox ################

    # set recalbox Hotkey
    HOTKEY_BUTTONS: Dict[str, int] = {
        "InputUIExit":              InputItem.ItemStart,
        "InputUISaveState":         InputItem.ItemWest,
        "InputUIChangeSlot":        InputItem.ItemUp,
        "InputUILoadState":         InputItem.ItemNorth,
        "InputUIScreenShot":        InputItem.ItemL1,
        "InputUIReset":             InputItem.ItemEast,
        "InputUIPause":             InputItem.ItemSouth,
    }

    SERVICE_TEST_BUTTON: Dict[str, int] = {
        "InputServiceA":            InputItem.ItemL3,
        "InputTestA":               InputItem.ItemR3,
    }

    ######################## Game controllers ################################
    # Each entry: (P1 key, P2 key or None, P1 item[, P2 item]).
    # P2 item defaults to P1 item; given explicitly only where P1/P2 historically
    # differ (kept as-is by this refactor — see git blame 68793ea97c).

    GAME_BUTTONS = [
        ("InputCoin1",          "InputCoin2",          InputItem.ItemSelect),
        ("InputStart1",         "InputStart2",         InputItem.ItemStart),
        # Fighting games
        ("InputEscape",         "InputEscape2",        InputItem.ItemWest),
        ("InputGuard",          "InputGuard2",         InputItem.ItemNorth),
        ("InputKick",           "InputKick2",          InputItem.ItemSouth),
        ("InputPunch",          "InputPunch2",         InputItem.ItemEast),
        # Spikeout
        ("InputBeat",           None,                  InputItem.ItemSouth),
        ("InputCharge",         None,                  InputItem.ItemNorth),
        ("InputJump",           None,                  InputItem.ItemEast),
        ("InputShift",          None,                  InputItem.ItemWest),
        # Virtua Striker — P2 item differs from P1 (historical, likely a bug)
        ("InputLongPass",       "InputLongPass2",      InputItem.ItemSouth, InputItem.ItemEast),
        ("InputShoot",          "InputShoot2",         InputItem.ItemWest, InputItem.ItemNorth),
        ("InputShortPass",      "InputShortPass2",     InputItem.ItemEast, InputItem.ItemSouth),
        # Ski Champ
        ("InputSkiPollLeft",    None,                  InputItem.ItemL1),
        ("InputSkiPollRight",   None,                  InputItem.ItemR1),
        ("InputSkiSelect1",     None,                  InputItem.ItemWest),
        ("InputSkiSelect2",     None,                  InputItem.ItemSouth),
        ("InputSkiSelect3",     None,                  InputItem.ItemEast),
        # Magical Truck Adventure
        ("InputMagicalPedal1",  "InputMagicalPedal2",  InputItem.ItemSouth),
        # Handbrake (Dirt Devils, Sega Rally 2)
        ("InputHandBrake",      None,                  InputItem.ItemWest),
        # Harley-Davidson
        ("InputMusicSelect",    None,                  InputItem.ItemSouth),
        ("InputRearBrake",      None,                  InputItem.ItemEast),
        # Virtual On
        ("InputTwinJoyShot1",   None,                  InputItem.ItemNorth),
        ("InputTwinJoyShot2",   None,                  InputItem.ItemWest),
        ("InputTwinJoyTurbo1",  None,                  InputItem.ItemL1),
        ("InputTwinJoyTurbo2",  None,                  InputItem.ItemR1),
        ("InputTwinJoyCrouch",  None,                  InputItem.ItemEast),
        ("InputTwinJoyJump",    None,                  InputItem.ItemSouth),
        # Up/down shifter manual transmission (all racers)
        ("InputGearShiftDown",  None,                  InputItem.ItemDown if system.CRTAdapter == CRTAdapter.RECALBOXJVS else InputItem.ItemL1),
        ("InputGearShiftUp",    None,                  InputItem.ItemUp   if system.CRTAdapter == CRTAdapter.RECALBOXJVS else InputItem.ItemR1),
        ("InputGearShiftN",     None,                  "-1"),
        # VR4 view change (Daytona 2, Le Mans 24, Scud Race)
        ("InputVR1",            None,                  InputItem.ItemWest),
        ("InputVR2",            None,                  InputItem.ItemNorth),
        ("InputVR3",            None,                  InputItem.ItemEast),
        ("InputVR4",            None,                  InputItem.ItemSouth),
        # Single view change (Dirt Devils, ECA, Harley-Davidson, Sega Rally 2)
        ("InputViewChange",     None,                  InputItem.ItemNorth),
        # Sega Bass Fishing / Get Bass
        ("InputFishingCast",    None,                  InputItem.ItemWest),
        ("InputFishingSelect",  None,                  InputItem.ItemEast),
    ]

    GAME_DIGITAL_JOYSTICK = [
        ("InputJoyUp",          "InputJoyUp2",         InputItem.ItemUp),
        ("InputJoyDown",        "InputJoyDown2",       InputItem.ItemDown),
        ("InputJoyLeft",        "InputJoyLeft2",       InputItem.ItemLeft),
        ("InputJoyRight",       "InputJoyRight2",      InputItem.ItemRight),
        # Ski Champ
        ("InputSkiLeft",        None,                  InputItem.ItemLeft),
        ("InputSkiRight",       None,                  InputItem.ItemRight),
        ("InputSkiUp",          None,                  InputItem.ItemUp),
        ("InputSkiDown",        None,                  InputItem.ItemDown),
        # Steering wheel (digital)
        ("InputSteeringLeft",   None,                  InputItem.ItemLeft),
        ("InputSteeringRight",  None,                  InputItem.ItemRight),
        # Magical Truck Adventure
        ("InputMagicalLeverUp1",   "InputMagicalLeverUp2",   InputItem.ItemUp),
        ("InputMagicalLeverDown1", "InputMagicalLeverDown2", InputItem.ItemDown),
        # Sega Bass Fishing / Get Bass
        ("InputFishingRodLeft",  None,                 InputItem.ItemLeft),
        ("InputFishingRodRight", None,                 InputItem.ItemRight),
        ("InputFishingRodUp",    None,                 InputItem.ItemUp),
        ("InputFishingRodDown",  None,                 InputItem.ItemDown),
        # Axis inputs (no POS/NEG needed)
        # Magical Truck — P2 item differs from P1 (historical "change to joyleft")
        ("InputMagicalLever1",  "InputMagicalLever2",  InputItem.ItemJoy1Up, InputItem.ItemJoy1Left),
        ("InputSteering",       None,                  InputItem.ItemJoy1Left),
        ("InputSkiX",           None,                  InputItem.ItemJoy1Left),
        ("InputSkiY",           None,                  InputItem.ItemJoy1Up),
        ("InputFishingRodX",    None,                  InputItem.ItemJoy2Left),
        ("InputFishingRodY",    None,                  InputItem.ItemJoy2Up),
        ("InputFishingStickX",  None,                  InputItem.ItemJoy1Left),
        ("InputFishingStickY",  None,                  InputItem.ItemJoy1Up),
    ]

    GAME_BUTTONS_AXIS: Dict[str, int] = {
        # NEED A BYPASS BUTTON OR AXIS ####
        # Pedals
        "InputAccelerator":         InputItem.ItemR2,
        "InputBrake":               InputItem.ItemL2,
        # NEED A BYPASS BUTTON OR AXIS ####
        "InputFishingReel":         InputItem.ItemL2,
        "InputFishingTension":      InputItem.ItemR2,
    }

    GAME_ANALOG_JOYSTICK: Dict[str, int] = {
        # Need setting all axis position (POS/NEG)
        # Virtual On individual joystick mapping
        "InputTwinJoyDown1":        InputItem.ItemJoy1Down,
        "InputTwinJoyDown2":        InputItem.ItemJoy2Down,
        "InputTwinJoyLeft1":        InputItem.ItemJoy1Left,
        "InputTwinJoyLeft2":        InputItem.ItemJoy2Left,
        "InputTwinJoyRight1":       InputItem.ItemJoy1Right,
        "InputTwinJoyRight2":       InputItem.ItemJoy2Right,
        "InputTwinJoyUp1":          InputItem.ItemJoy1Up,
        "InputTwinJoyUp2":          InputItem.ItemJoy2Up,
        # 4-Speed manual transmission (Daytona 2, Sega Rally 2, Scud Race)
        # Setting on joystick2
        "InputGearShift1":          InputItem.ItemJoy2Up,
        "InputGearShift2":          InputItem.ItemJoy2Down,
        "InputGearShift3":          InputItem.ItemJoy2Left,
        "InputGearShift4":          InputItem.ItemJoy2Right,
    }

    POINTING_GAME: Dict[str, int] = {
        # Light guns
        "InputGunX":                InputItem.ItemJoy1Left,
        "InputGunY":                InputItem.ItemJoy1Up,
        # Analog guns (Lost World, Ocean Hunter, LA Machineguns)
        "InputAnalogGunX":          InputItem.ItemJoy1Left,
        "InputAnalogGunY":          InputItem.ItemJoy1Up,
        # Analog joystick (Star Wars Trilogy)
        "InputAnalogJoyX":          InputItem.ItemJoy1Left,
        "InputAnalogJoyY":          InputItem.ItemJoy1Up,
        # Triggers — all switches in Supermodel, must map to buttons (not axes).
        "InputTrigger":             InputItem.ItemSouth,
        "InputAnalogJoyTrigger":    InputItem.ItemSouth,
        "InputAnalogJoyEvent":      InputItem.ItemEast,
        "InputAnalogTriggerLeft":   InputItem.ItemSouth,
        "InputAnalogTriggerRight":  InputItem.ItemEast,
    }

    # GunCon 2: es_input.cfg maps neither the aiming axes nor the trigger
    # (BTN_LEFT, sdl button 5), so the device layout is used as-is.
    GUNCON_VALUES: Dict[str, str] = {
        "InputGunX":                "XAXIS",
        "InputGunY":                "YAXIS",
        "InputAnalogGunX":          "XAXIS",
        "InputAnalogGunY":          "YAXIS",
        "InputAnalogJoyX":          "XAXIS",
        "InputAnalogJoyY":          "YAXIS",
        "InputTrigger":             "BUTTON6",
        "InputAnalogJoyTrigger":    "BUTTON6",
        "InputAnalogJoyEvent":      "BUTTON8",
        "InputAnalogTriggerLeft":   "BUTTON6",
        "InputAnalogTriggerRight":  "BUTTON8",
    }

    # Wiimote through a DolphinBar: the trigger is a mouse click, its joystick
    # node has neither the aiming axes nor that button.
    MOUSE_VALUES: Dict[str, str] = {
        "InputGunX":                "XAXIS",
        "InputGunY":                "YAXIS",
        "InputAnalogGunX":          "XAXIS",
        "InputAnalogGunY":          "YAXIS",
        "InputAnalogJoyX":          "XAXIS",
        "InputAnalogJoyY":          "YAXIS",
        "InputTrigger":             "LEFT_BUTTON",
        "InputAnalogJoyTrigger":    "LEFT_BUTTON",
        "InputAnalogJoyEvent":      "RIGHT_BUTTON",
        "InputAnalogTriggerLeft":   "LEFT_BUTTON",
        "InputAnalogTriggerRight":  "RIGHT_BUTTON",
    }


    ###### Map an Recalbox direction to the corresponding Supermodel ######

    TYPE_TO_NAME: Dict[int, str] = {
        InputItem.TypeAxis:   'AXIS',
        InputItem.TypeButton: 'BUTTON',
        InputItem.TypeHat:    'POV',
    }

    HATS_TO_NAME: Dict[int, str] = {
        1:                        'UP',
        2:                        'RIGHT',
        4:                        'DOWN',
        8:                        'LEFT',
    }

    """SUPERMODEL_DIR = \
    {
        'down':                     'DOWN',
        'left':                     'LEFT',
        'right':                    'RIGHT',
        'up':                       'UP',
    }"""

    SUPERMODEL_JOY: Dict[int, str] = {
        # lEFT JOYSTICK
        InputItem.ItemJoy1Down:            'YAXIS',
        InputItem.ItemJoy1Left:            'XAXIS',
        InputItem.ItemJoy1Right:           'XAXIS',
        InputItem.ItemJoy1Up:              'YAXIS',
        # RIGHT JOYSTICK
        InputItem.ItemJoy2Down:            'RYAXIS',
        InputItem.ItemJoy2Left:            'RXAXIS',
        InputItem.ItemJoy2Right:           'RXAXIS',
        InputItem.ItemJoy2Up:              'RYAXIS',
        # AXIS ON L2/R2 BUTTONS
        InputItem.ItemL2:                  'ZAXIS',
        InputItem.ItemR2:                  'RZAXIS',
        # JAMMA
        InputItem.ItemDown:                'DOWN',
        InputItem.ItemLeft:                'LEFT',
        InputItem.ItemRight:               'RIGHT',
        InputItem.ItemUp:                  'UP',
    }

    # Arcade panel (JAMMA/JVS) — fixed physical button numbers, not pad inputs.
    # Each entry: (P1 key, P2 key or None, jamma button index; -1 = unmapped).
    ARCADE_PANEL_CONTROL = [
        # Fighting games
        ("InputGuard",      "InputGuard2",      InputItem.ItemWest),
        ("InputPunch",      "InputPunch2",      InputItem.ItemEast),
        ("InputKick",       "InputKick2",       InputItem.ItemSouth),
        ("InputEscape",     "InputEscape2",     InputItem.ItemNorth),
        # Spikeout
        ("InputShift",      "InputShift2",      InputItem.ItemWest),
        ("InputBeat",       "InputBeat2",       InputItem.ItemEast),
        ("InputCharge",     "InputCharge2",     InputItem.ItemSouth),
        ("InputJump",       "InputJump2",       InputItem.ItemNorth),
        # Virtua Striker
        ("InputShortPass",  "InputShortPass2",  InputItem.ItemEast),
        ("InputLongPass",   "InputLongPass2",   InputItem.ItemSouth),
        ("InputShoot",      "InputShoot2",      InputItem.ItemNorth),
        # Pedals
        ("InputAccelerator", None,              InputItem.ItemEast),
        ("InputBrake",       None,              InputItem.ItemSouth),
        # Handbrake (Dirt Devils, Sega Rally 2)
        ("InputHandBrake",   None,              InputItem.ItemNorth),
        # Single view change (Dirt Devils, ECA, Harley-Davidson, Sega Rally 2)
        ("InputViewChange",  None,              InputItem.ItemWest),
        # VR4 view change (Daytona 2, Le Mans 24, Scud Race) — unmapped
        ("InputVR1",         None,              -1),
        ("InputVR2",         None,              -1),
        ("InputVR3",         None,              -1),
        ("InputVR4",         None,              -1),
    ]

    def getAxisName(inputItem: InputItem):
        return 'AXIS{}'.format(inputItem.Id + 1)

    def getConfigValue(inputItem: InputItem, logicalItem: int):
        if inputItem.IsButton:
            return '{}{}'.format(TYPE_TO_NAME[inputItem.Type], inputItem.Id + 1)
        if inputItem.IsAxis:
            return getAxisName(inputItem) if logicalItem in SUPERMODEL_JOY else ''
        if inputItem.IsHat:
            return '{}{}_{}'.format(TYPE_TO_NAME[inputItem.Type], inputItem.Id + 1, HATS_TO_NAME[inputItem.Value])
        raise TypeError

    # Axis with a NEG/POS direction (analog joystick directions)
    def getPositionConfigValue(inputItem: InputItem, logicalItem: int):
        if inputItem.IsAxis:
            direction = '_NEG' if inputItem.Value == -1 else '_POS'
            return getAxisName(inputItem) + direction
        raise TypeError

    # For R2/L2 axis or button
    def getAxisOrButton(inputItem: InputItem, logicalItem: int):
        if inputItem.IsButton:
            return '{}{}'.format(TYPE_TO_NAME[inputItem.Type], inputItem.Id + 1)
        if inputItem.IsAxis:
            return getAxisName(inputItem) + '_POS'
        raise TypeError

    # Light gun mapped on a pad: axes and buttons of that pad.
    def getlightGunConfigValue(pad: Controller, inputItem: InputItem, logicalItem: int):
        j = 'JOY{}'.format(pad.SdlIndex + 1)
        if inputItem.IsAxis:
            return '{}_{}'.format(j, SUPERMODEL_JOY[logicalItem])
        if inputItem.IsButton:
            return '{}_BUTTON{}'.format(j, inputItem.Id + 1)
        if inputItem.IsHat:
            return '{}_POV{}_{}'.format(j, inputItem.Id + 1, HATS_TO_NAME[inputItem.Value])
        return ''

    # Load Configuration
    from configgen.settings.iniSettings import IniSettings
    supermodelControllersSettings = IniSettings(controlsFile, True)
    supermodelControllersSettings.loadFile(True)
    from configgen.settings.keyValueSettings import keyValueSettings
    supermodelSettings = keyValueSettings(configFile)
    supermodelSettings.loadFile(True)

    # Set default configuration
    # set emulated Net options desactived on default doesn't work on linux at the moment
    # Network board - experimental build for win32 only
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "AddressOut", '"127.0.0.1"') \
                                 .setInt(self.SECTION_GLOBAL, "Network", 0) \
                                 .setInt(self.SECTION_GLOBAL, "SimulateNet", 0) \
                                 .setInt(self.SECTION_GLOBAL, "PortIn", 1970) \
                                 .setInt(self.SECTION_GLOBAL, "PortOut", 1971)

    # Set auto triggers activate on default
    # automatic reload when off-screen
    supermodelControllersSettings.setInt(self.SECTION_GLOBAL, "InputAutoTrigger", 1) \
                                 .setInt(self.SECTION_GLOBAL, "InputAutoTrigger2", 1)

    # Set sensitivity analog configurable in ConfigModel3.ini
    # InputKeySensitivity
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputKeySensitivity", supermodelSettings.getString("sensitivity", ""))

    # Set deadzone analog configurable in ConfigModel3.ini
    deadzone = supermodelSettings.getString("deadzone", "")
    # joystick1 Player 1
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy1XDeadZone", deadzone) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1YDeadZone", deadzone)
    # joystick2 player 1
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy1RXDeadZone", deadzone) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1RYDeadZone", deadzone)
    # triggers R2/L2 player 1
    # supermodelControllersSettings.setOption(self.SECTION_GLOBAL, "InputJoy1RZDeadZone", deadzone)
    # supermodelControllersSettings.setOption(self.SECTION_GLOBAL, "InputJoy1ZDeadZone", deadzone)
    # joystick1 Player 2
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy2YDeadZone", deadzone) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2ZDeadZone", deadzone)
    # joystick2 Player 2
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy2RXDeadZone", deadzone) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2RYDeadZone", deadzone)
    # triggers R2/L2 player 2
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy2RZDeadZone", deadzone) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2XDeadZone", deadzone)

    # Set Saturation analog configurable in ConfigModel3.ini
    saturation = supermodelSettings.getString("saturation", "")
    # joystick1 Players 1
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy1RXSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1RYSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1RZSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1XSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1YSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy1ZSaturation", saturation)
    # joystick2 Players 2
    supermodelControllersSettings.setString(self.SECTION_GLOBAL, "InputJoy2RXSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2RYSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2RZSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2XSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2YSaturation", saturation) \
                                 .setString(self.SECTION_GLOBAL, "InputJoy2ZSaturation", saturation)

    # Write a GAME_* table for one player. player is 1 or 2; the matching
    # key column is used (None entries are skipped), and the P2 item override
    # (4th tuple element) applies when present.
    def applyTable(pad: Controller, padIndex: str, table, player: int):
        for entry in table:
            key = entry[0] if player == 1 else entry[1]
            if key is None:
                continue
            item = entry[2] if player == 1 or len(entry) < 4 else entry[3]
            if pad.HasInput(item):
                supermodelControllersSettings.setString(self.SECTION_GLOBAL, key,
                    '"{}_{}"'.format(padIndex, getConfigValue(pad.Input(item), item)))

    # Write the light gun mapping. suffix '' for P1, '2' for P2.
    def applyGun(pad: Controller, suffix: str):
        device, values = '', {}
        if pad.DeviceName.startswith("Namco GunCon 2"):
            device, values = 'JOY{}'.format(pad.SdlIndex + 1), GUNCON_VALUES
        # SDL merges every mouse into one cursor, so a single wiimote can aim.
        elif suffix == '' and hasDolphinBar():
            device, values = 'MOUSE', MOUSE_VALUES
        if values:
            for key, value in values.items():
                supermodelControllersSettings.setString(self.SECTION_GLOBAL, key + suffix,
                    '"{}_{}"'.format(device, value))
            return
        for key, item in POINTING_GAME.items():
            if pad.HasInput(item):
                supermodelControllersSettings.setString(self.SECTION_GLOBAL, key + suffix,
                    '"{}"'.format(getlightGunConfigValue(pad, pad.Input(item), item)))

    # Write the arcade panel mapping for one player (fixed jamma button numbers).
    def applyArcadePanel(padIndex: str, player: int):
        for p1key, p2key, index in ARCADE_PANEL_CONTROL:
            key = p1key if player == 1 else p2key
            if key is None:
                continue
            value = '"{}_BUTTON{}"'.format(padIndex, index + 1) if index != -1 else ''
            supermodelControllersSettings.setString(self.SECTION_GLOBAL, key, value)

    for playercontroller in playersControllers:
        pad: Controller = playersControllers[playercontroller]
        # Supermodel joysticks are 1-indexed.
        padIndex = 'JOY{}'.format(pad.SdlIndex + 1)
        is_arcade = pad.GamepadType.isArcade()

        if pad.PlayerIndex == 1:
            # Hotkeys and service/test are global — P1 only.
            for x in HOTKEY_BUTTONS:
                if pad.HasInput(HOTKEY_BUTTONS[x]):
                    inp = pad.Input(HOTKEY_BUTTONS[x])
                    hk = pad.Input(InputItem.ItemHotkey)
                    supermodelControllersSettings.setString(self.SECTION_GLOBAL, x,
                        '"{}_{}+{}_{}"'.format(padIndex, getConfigValue(hk, InputItem.ItemHotkey),
                                               padIndex, getConfigValue(inp, HOTKEY_BUTTONS[x])))

            for x in SERVICE_TEST_BUTTON:
                if pad.HasInput(SERVICE_TEST_BUTTON[x]):
                    inp = pad.Input(SERVICE_TEST_BUTTON[x])
                    if supermodelSettings.getString("service-button", "") == "1":
                        supermodelControllersSettings.setString(self.SECTION_GLOBAL, x,
                            '"{}_{}"'.format(padIndex, getConfigValue(inp, SERVICE_TEST_BUTTON[x])))
                    else:
                        supermodelControllersSettings.setString(self.SECTION_GLOBAL, x, "NONE")

            applyTable(pad, padIndex, GAME_BUTTONS, 1)
            applyTable(pad, padIndex, GAME_DIGITAL_JOYSTICK, 1)

            for x in GAME_ANALOG_JOYSTICK:
                if pad.HasInput(GAME_ANALOG_JOYSTICK[x]):
                    inp = pad.Input(GAME_ANALOG_JOYSTICK[x])
                    supermodelControllersSettings.setString(self.SECTION_GLOBAL, x,
                        '"{}_{}"'.format(padIndex, getPositionConfigValue(inp, GAME_ANALOG_JOYSTICK[x])))


            applyGun(pad, '')

            if is_arcade:
                applyArcadePanel(padIndex, 1)

            for x in GAME_BUTTONS_AXIS:
                if pad.HasInput(GAME_BUTTONS_AXIS[x]):
                    inp = pad.Input(GAME_BUTTONS_AXIS[x])
                    supermodelControllersSettings.setString(self.SECTION_GLOBAL, x,
                        '"{}_{}"'.format(padIndex, getAxisOrButton(inp, GAME_BUTTONS_AXIS[x])))

            # Per-game race-cabinet overrides (JVS): remaps rewrite Supermodel keys,
            # combos are pushed to the R.G.P. (shared with the libretro arcade path).
            from configgen.remapper.RemapProfileStore import RemapProfileStore
            from configgen.remapper.Remapper import Remapper, jvs_cab_type
            import os
            romName = os.path.splitext(os.path.basename(rom))[0] if rom else ""
            profile = RemapProfileStore().resolve("model3", romName, pad.GamepadType.Type.value, jvs_cab_type())
            names = InputItem.ConverterNameToItem
            for smKey, phys in profile.get("remaps", {}).items():
                item = names.get(phys)
                if item is not None and pad.HasInput(item):
                    supermodelControllersSettings.setString(self.SECTION_GLOBAL, smKey,
                        '"{}_{}"'.format(padIndex, getConfigValue(pad.Input(item), item)))
            Remapper()._push_combos(profile.get("combos", []), pad)

        elif pad.PlayerIndex == 2:
            applyTable(pad, padIndex, GAME_BUTTONS, 2)
            applyTable(pad, padIndex, GAME_DIGITAL_JOYSTICK, 2)
            applyGun(pad, '2')

            if is_arcade:
                applyArcadePanel(padIndex, 2)

    # Save configuration
    supermodelControllersSettings.saveFile()
