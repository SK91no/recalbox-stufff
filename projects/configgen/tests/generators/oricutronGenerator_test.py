#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.oricutron.oricutronGenerator as oricutronGenerator
from configgen.generators.oricutron.oricutronGenerator import OricutronGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    oricutronGenerator.recalboxFiles.oricutronConfig = 'tests/tmp/oricutron.conf'
    return OricutronGenerator()


@pytest.fixture
def system():
    return Emulator(name='oricatmos', videoMode='1920x1080', ratio='auto', emulator='oricutron', core='oricutron')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_oricutron_disk(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.dsk'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/oricutron/oricutron',
                             '--turbotape', 'on',
                             '--vsynchack', 'on',
                             '--disk', 'path/to/test.dsk']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_oricutron_tape(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.tap'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/oricutron/oricutron',
                             '--turbotape', 'on',
                             '--vsynchack', 'on',
                             '--tape', 'path/to/test.tap']
