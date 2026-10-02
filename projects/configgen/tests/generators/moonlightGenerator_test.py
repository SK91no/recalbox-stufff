#!/usr/bin/env python
import pytest
import shutil
import os
from configgen.Emulator import Emulator
import configgen.generators.moonlight.moonlightGenerator as moonlightGenerator
from configgen.generators.moonlight.moonlightGenerator import MoonlightGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments
from pathlib import Path


@pytest.fixture
def emulator():
    moonlightGenerator.recalboxFiles.moonlightCustom = 'tests/tmp/moonlight'
    moonlightGenerator.recalboxFiles.moonlightConfig = 'tests/tmp/moonlight/moonlight.conf'
    moonlightGenerator.recalboxFiles.moonlightGamelist = 'tests/tmp/moonlight/gamelist.txt'
    moonlightGenerator.recalboxFiles.moonlightIsQT = 'tests/tmp/is_qt'
    os.makedirs(moonlightGenerator.recalboxFiles.moonlightCustom, exist_ok=True)
    shutil.copyfile('tests/resources/moonlight/gamelist.txt', 'tests/tmp/moonlight/gamelist.txt')
    return MoonlightGenerator()


@pytest.fixture
def system():
    return Emulator(name='moonlight', videoMode='1920x1080', ratio='auto', emulator='moonlight', core='moonlight')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_moonlight_embedded(emulator, system, controller_configuration):
    if os.path.exists(moonlightGenerator.recalboxFiles.moonlightIsQT):
        os.unlink(moonlightGenerator.recalboxFiles.moonlightIsQT)
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('somegame_'))
    assert command.array == ['/usr/bin/moonlight', 'stream',
                             '-config', 'tests/resources/moonlight/moonlight.conf',
                             '-platform', 'auto',
                             '-codec', 'hevc',
                             '-app', 'SOMEGAME']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_moonlight_qt(emulator, system, controller_configuration):
    Path(moonlightGenerator.recalboxFiles.moonlightIsQT).touch()
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('somegame_'))
    assert command.array == ['/usr/bin/moonlight', 'stream',
                             '1.2.3.4',
                             'SOMEGAME']
