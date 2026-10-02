#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.duckstation.duckstationGenerator as duckstationGenerator
from configgen.generators.duckstation.duckstationGenerator import DuckstationGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    duckstationGenerator.recalboxFiles.duckstationConfig = 'tests/tmp/duckstation.ini'
    duckstationGenerator.recalboxFiles.SAVES = 'tests/tmp/savedir/'
    duckstationGenerator.DuckstationGenerator.SETTINGS_FILE = 'tests/tmp/recalbox/settings.ini'
    duckstationGenerator.DuckstationGenerator.DATABASE_FILE = 'tests/tmp/recalbox/gamecontrollerdb.txt'
    controllersConfig.esInputs = 'tests/resources/es_input.cfg'
    return DuckstationGenerator()


@pytest.fixture
def system():
    return Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='duckstation', core='duckstation')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_duckstation(emulator, system, mocker, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.dsk'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/duckstation', 'path/to/test.dsk']
