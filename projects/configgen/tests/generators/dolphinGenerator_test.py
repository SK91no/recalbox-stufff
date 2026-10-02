#!/usr/bin/env python3
import pytest
import shutil
from configgen.Emulator import Emulator
import configgen.generators.dolphin.dolphinGenerator as dolphinGenerator
from configgen.generators.dolphin.dolphinGenerator import DolphinGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    dolphinGenerator.iniFile = 'tests/tmp/dolphin.ini'
    dolphinGenerator.sysconfFile = 'tests/tmp/dolphin.SYSCONF'
    dolphinGenerator.gfxFile = 'tests/tmp/dolphin-GFX.ini'
    shutil.copyfile("tests/resources/dolphin.SYSCONF", "tests/tmp/dolphin.SYSCONF")
    return DolphinGenerator()


@pytest.fixture
def system_wii():
    return Emulator(name='wii', videoMode='1920x1080', ratio='auto', emulator='dolphin', core='dolphin-gui')


@pytest.fixture
def system_gamecube():
    return Emulator(name='gamecube', videoMode='1920x1080', ratio='auto', emulator='dolphin', core='dolphin-gui')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_wii(emulator, system_wii, mocker, controller_configuration):
    mocker.patch("configgen.generators.dolphin.dolphinControllers.EvdevGetJoystickName", return_value="Sony PLAYSTATION(R)3 Controller")
    command = emulator.generate(system_wii, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/dolphin', '-v', 'OGL', '-C', 'Logger.Options.Verbosity=5', '-C', 'Logger.Logs.CONSOLE=true', '-C', 'Logger.Logs.CORE=true', '-C', 'Logger.Logs.Video=true', '-e', 'path/to/test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_gamecube(emulator, system_gamecube, mocker, controller_configuration):
    mocker.patch("configgen.generators.dolphin.dolphinControllers.EvdevGetJoystickName", return_value="Sony PLAYSTATION(R)3 Controller")
    command = emulator.generate(system_gamecube, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/dolphin', '-v', 'OGL', '-C', 'Logger.Options.Verbosity=5', '-C', 'Logger.Logs.CONSOLE=true', '-C', 'Logger.Logs.CORE=true', '-C', 'Logger.Logs.Video=true', '-e', 'path/to/test']
