#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
from configgen.generators.scummvm.scummvmGenerator import ScummVMGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    return ScummVMGenerator()


@pytest.fixture
def system():
    return Emulator(name='scummvm', videoMode='1920x1080', ratio='auto', emulator='scummvm', core='scummvm')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_scummvm(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == 'default'
    assert command.array == ['/usr/bin/scummvm', '--fullscreen',
                             '--joystick=0', '--filtering',
                             '--extrapath=/usr/share/scummvm',
                             '--savepath=/recalbox/share/saves/scummvm',
                             '--path=path/to', '--subtitles',
                             'test']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_scummvm_romdir(emulator, system, mocker, controller_configuration):
    mocker.patch('os.path.isdir', return_value=True)
    mocker.patch('glob.glob', return_value=['path/to/test/thegame.scummvm'])
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == 'default'
    assert command.array == ['/usr/bin/scummvm', '--fullscreen',
                             '--joystick=0', '--filtering',
                             '--extrapath=/usr/share/scummvm',
                             '--savepath=/recalbox/share/saves/scummvm',
                             '--path=path/to/test', '--subtitles',
                             'thegame']
