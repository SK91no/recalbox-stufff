#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.openlara.openlaraGenerator as openlaraGenerator
from configgen.generators.openlara.openlaraGenerator import OpenlaraGenerator
from configgen.settings.keyValueSettings import keyValueSettings

from tests.generators.FakeArguments import Arguments

@pytest.fixture
def emulator():
    openlaraGenerator.OpenlaraGenerator.controllersFile = 'tests/tmp/recalbox/gamecontrollerdb.txt'
    return OpenlaraGenerator()

@pytest.fixture
def system():
    return Emulator(name='openlara', videoMode='1920x1080', ratio='auto', emulator='openlara', core='openlara')

@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_openlara(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/openlara', '-d', 'path/to', '-f']
