#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.pisnes.pisnesGenerator as pisnesGenerator
from configgen.generators.pisnes.pisnesGenerator import PisnesGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    pisnesGenerator.recalboxFiles.pisnesConfigFile = 'tests/tmp/pisnes.cfg'
    return PisnesGenerator()


@pytest.fixture
def system():
    return Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='pisnes', core='pisnes')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_pisnes(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.dsk'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/pisnes', 'path/to/test.dsk']
