#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.corsixth.corsixthGenerator as corsixTHGenerator
from configgen.generators.corsixth.corsixthGenerator import CorsixTHGenerator
from configgen.settings.keyValueSettings import keyValueSettings

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    corsixTHGenerator.CorsixTHGenerator.configFile = 'tests/tmp/corsixth.cfg'
    corsixTHGenerator.CorsixTHGenerator.controllersFile = 'tests/tmp/recalbox/gamecontrollerdb.txt'
    return CorsixTHGenerator()

@pytest.fixture
def system():
    return Emulator(name='corsixth', videoMode='1920x1080', ratio='auto', emulator='corsixth', core='corsixth')

@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_sdlpop(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/corsix-th', 'path/to/test', '--interpreter=/recalbox/share/roms/ports/Theme Hospital/CorsixTH.Lua', '--config-file=tests/tmp/corsixth.cfg']
