#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
from configgen.generators.kodi.kodiGenerator import KodiGenerator
import configgen.generators.kodi.kodiConfig as kodiConfig
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    kodiConfig.recalboxFiles.kodiJoystick = 'tests/tmp/kodi-joystick/'
    kodiConfig.recalboxFiles.kodiAdvancedSettings = 'tests/tmp/advanced-settings.xml'
    return KodiGenerator()


@pytest.fixture
def system():
    return Emulator(name='kodi', videoMode='1920x1080', ratio='auto', emulator='kodi', core='kodi')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/recalbox/scripts/kodilauncher.sh']
