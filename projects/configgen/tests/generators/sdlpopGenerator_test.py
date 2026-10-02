#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.sdlpop.sdlpopGenerator as sdlpopGenerator
from configgen.generators.sdlpop.sdlpopGenerator import SdlpopGenerator
from configgen.settings.keyValueSettings import keyValueSettings
import configgen.controllers.controller as controllersConfig
from tests.generators.FakeArguments import Arguments

@pytest.fixture
def emulator():
    sdlpopGenerator.recalboxFiles.sdlpopConfigFile = 'tests/tmp/sdlpop.cfg'
    sdlpopGenerator.recalboxFiles.sdlpopControllersFile = 'tests/tmp/recalbox/gamecontrollerdb.txt'
    return SdlpopGenerator()

@pytest.fixture
def system():
    return Emulator(name='sdlpop', videoMode='1920x1080', ratio='auto', emulator='sdlpop', core='sdlpop')

@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_sdlpop(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/prince', 'path/to/test']
