#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
from configgen.generators.pcsx2.pcsx2Generator import Pcsx2Generator
from configgen.settings.keyValueSettings import keyValueSettings

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    return Pcsx2Generator()


@pytest.fixture
def system():
    return Emulator(name='pcsx2', videoMode='1920x1080', ratio='auto', emulator='pcsx2', core='pcsx2')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_pcsx2(emulator, system, controller_configuration, fake_process):
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/PCSX2/PCSX2', '--fullscreen', '--fullboot', 'path/to/test']
