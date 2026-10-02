#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.pcsx.pcsxGenerator as pcsxGenerator
from configgen.generators.pcsx.pcsxGenerator import PcsxGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    pcsxGenerator.recalboxFiles.pcsxConfigFile = 'tests/tmp/pcsx.cfg'
    return PcsxGenerator()


@pytest.fixture
def system():
    return Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='pcsx_rearmed', core='pcsx_rearmed')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_pcsx(emulator, system, mocker, controller_configuration):
    mocker.patch("configgen.generators.pcsx.pcsxGenerator.PcsxGenerator.EvdevGetJoystickName", return_value="Microsoft Xbox 360 controller")
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.dsk'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/pcsx_rearmed', '-cdfile', 'path/to/test.dsk']
