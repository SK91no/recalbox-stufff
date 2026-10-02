#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.simcoupe.simcoupeGenerator as simcoupeGenerator
from configgen.generators.simcoupe.simcoupeGenerator import SimCoupeGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    simcoupeGenerator.recalboxFiles.simcoupeConfig = 'tests/tmp/SimCoupe.cfg'
    return SimCoupeGenerator()


@pytest.fixture
def system():
    return Emulator(name='simcoupe', videoMode='1920x1080', ratio='auto', emulator='simcoupe', core='simcoupe')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_simcoupe(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/simcoupe', 'path/to/test']
