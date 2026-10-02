#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.daphne.daphneControllers as daphneGenerator
from configgen.generators.daphne.daphneGenerator import DaphneGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig
from tests.generators.FakeArguments import Arguments
from configgen.utils.Vulkan import Vulkan

@pytest.fixture
def emulator(fake_process):
    daphneGenerator.recalboxFiles.daphneInputIni = 'tests/tmp/dapinput.ini'
    return DaphneGenerator()


@pytest.fixture
def system():
    return Emulator(name='daphne', videoMode='16', ratio='auto', emulator='daphne', core='daphne')


@pytest.fixture
def system_xy():
    return Emulator(name='daphne', videoMode='1920x1080', ratio='auto', emulator='daphne', core='daphne')


def test_simple_generate_singe_file(emulator, system, mocker):
    mocker.patch("os.path.exists", return_value=True)
    command = emulator.generate(system, dict(), keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/hypseus', 'singe', 'vldp',
                             '-framefile', 'path/to/test/test.txt',
                             '-fullscreen',
                             '-script', 'path/to/test/test.singe',
                             '-retropath',
                             "-texturestream",
                             '-datadir', '/usr/share/daphne',
                             '-homedir', 'path/to/test',
                             '-singedir', 'path/to/test',
                             '-bezel', 'path/to/test/../bezels/test.png',
                             '-force_aspect_ratio',
                             '-vulkan',
                             ]


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_other_rom(emulator, system, controller_configuration):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/hypseus', 'test', 'vldp',
                             '-framefile', 'path/to/test/test.txt',
                             '-fullscreen',
                             '-datadir', '/usr/share/daphne',
                             '-homedir', 'path/to/test',
                             '-opengl',
                             ]


def test_simple_generate_rpi_resolution(emulator, system_xy, mocker):
    mocker.patch("configgen.utils.architecture.Architecture", return_value="rpi3")
    command = emulator.generate(system_xy, dict(), keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/hypseus', 'test', 'vldp',
                             '-framefile', 'path/to/test/test.txt',
                             '-fullscreen',
                             '-datadir', '/usr/share/daphne',
                             '-homedir', 'path/to/test',
                             '-opengl',
                             '-x', '1920',
                             '-y', '1080'
                             ]
