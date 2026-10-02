from configgen.Emulator import Emulator
from configgen.controllers.inputItem import InputItem
from configgen.generators.libretro.libretroControllers import LibretroControllers


class _FakeInput:
    def __init__(self, type_):
        self.Type = type_


class _FakeController:
    # HasInput/Input are all getAnalogMode needs; dir_type=None means no directional input.
    def __init__(self, dir_type):
        self._dir_type = dir_type

    def HasInput(self, key):
        return self._dir_type is not None

    def Input(self, key):
        return _FakeInput(self._dir_type)


def _system(name):
    return Emulator(name=name, videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next')


def _mode(dir_type, system_name, rom):
    return LibretroControllers.getAnalogMode(_FakeController(dir_type), _system(system_name), rom)


def test_naomi2_digital_joystick_game_forces_left_analog():
    # Beach Spikers / VF4 / Virtua Striker are digital 8-way: force the stick onto the d-pad (3).
    assert _mode(InputItem.TypeButton, 'naomi2', 'beachspi') == 3
    assert _mode(InputItem.TypeButton, 'naomi2', 'vf4') == 3
    assert _mode(InputItem.TypeHat, 'naomi2', 'vstrik3c') == 3


def test_naomi2_analog_game_stays_analog():
    # Driving / analog games keep the stick analog (0), else forcing (3) turns steering into d-pad.
    assert _mode(InputItem.TypeButton, 'naomi2', 'clubkrt') == 0
    assert _mode(InputItem.TypeButton, 'naomi2', 'initd') == 0
    assert _mode(InputItem.TypeButton, 'naomi2', 'kingrt66') == 0


def test_atomiswave_analog_stays_analog_digital_forced():
    # atomiswave analog (maxspeed) must stay 0; a non-analog rom forces the stick onto the d-pad.
    assert _mode(InputItem.TypeButton, 'atomiswave', 'maxspeed') == 0
    assert _mode(InputItem.TypeButton, 'atomiswave', 'whatever') == 3
    assert _mode(InputItem.TypeButton, 'naomigd', 'whatever') == 3


def test_other_system_uses_non_forced():
    assert _mode(InputItem.TypeHat, 'snes', 'Mario') == 1


def test_analog_directional_uses_none():
    assert _mode(InputItem.TypeAxis, 'naomi2', 'beachspi') == 0


def test_no_directional_input_uses_none():
    assert _mode(None, 'naomi2', 'beachspi') == 0
