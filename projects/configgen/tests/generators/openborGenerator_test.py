#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.openbor.openborGenerator as openborGenerator
from configgen.generators.openbor.openborGenerator import OpenborGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    openborGenerator.recalboxFiles.openborConfig = 'tests/tmp/openbor.conf'
    return OpenborGenerator()


@pytest.fixture
def system():
    return Emulator(name='openbor', videoMode='1920x1080', ratio='auto', emulator='openbor', core='openbor')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_openbor(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/OpenBOR', 'path/to/test']
