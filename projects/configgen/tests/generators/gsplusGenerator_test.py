#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.generators.gsplus.gsplusGenerator as gsplusGenerator
from configgen.generators.gsplus.gsplusGenerator import GSplusGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    gsplusGenerator.recalboxFiles.gsplusConfig = 'tests/tmp/gsplus.conf'
    return GSplusGenerator()


@pytest.fixture
def system():
    return Emulator(name='apple2gs', videoMode='1920x1080', ratio='auto', emulator='gsplus', core='gsplus')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_apple_ii(emulator, system, mocker, controller_configuration):
    mocker.patch("os.path.getsize", return_value=143360)
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/GSplus', '-fullscreen',
                             '-ssdir', '/recalbox/share/screenshots',
                             '-config', 'tests/tmp/gsplus.conf',
                             '-mem', '14680064',
                             '-slot', '6', '-x', '1920',
                             '-y', '1080',
                             '-joy', '0', '-joy-b0', '1',
                             '-joy-b1', '0', '-joy-b2', '2',
                             '-joy-b3', '3', '-joy-bhk', '10',
                             '-joy-bstart', '9', '-joy-x', '0',
                             '-joy-y', '1', '-joy-x2', '2',
                             '-joy-y2', '3', '-joy-up-button', '13',
                             '-joy-down-button', '14', '-joy-left-button', '15',
                             '-joy-right-button', '16',
                             ]


def test_simple_generate_apple_iigs(emulator, system, mocker):
    mocker.patch("os.path.getsize", return_value=819200)
    command = emulator.generate(system, dict(), keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/GSplus', '-fullscreen',
                             '-ssdir', '/recalbox/share/screenshots',
                             '-config', 'tests/tmp/gsplus.conf',
                             '-mem', '14680064',
                             '-slot', '5', '-x', '1920',
                             '-y', '1080']


def test_simple_generate_apple_iigs_hd(emulator, system, mocker):
    mocker.patch("os.path.getsize", return_value=9000000)
    command = emulator.generate(system, dict(), keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/GSplus', '-fullscreen',
                             '-ssdir', '/recalbox/share/screenshots',
                             '-config', 'tests/tmp/gsplus.conf',
                             '-mem', '14680064',
                             '-slot', '7', '-x', '1920',
                             '-y', '1080']
