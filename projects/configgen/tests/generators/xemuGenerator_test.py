#!/usr/bin/env python3
import pytest
import shutil
from configgen.Emulator import Emulator
from configgen.generators.xemu.xemuEeprom import XemuEeprom
from configgen.generators.xemu.xemuGenerator import XemuGenerator
from configgen.settings.keyValueSettings import keyValueSettings
from tests.generators.FakeArguments import Arguments

@pytest.fixture
def emulator():
    XemuGenerator.xemuConfigFile = 'tests/tmp/xemu.toml'
    XemuEeprom.eepromFile = 'tests/tmp/xemu_eeprom.bin'
    XemuEeprom.eepromConfigFile = 'tests/tmp/eeprom_config.ini'
    shutil.copyfile('tests/resources/xemu.toml', 'tests/tmp/xemu.toml')
    shutil.copyfile('tests/resources/xemu_eeprom.bin', 'tests/tmp/xemu_eeprom.bin')
    return XemuGenerator()


@pytest.fixture
def system():
    return Emulator(name='xbox', videoMode='16', ratio='auto', emulator='xemu', core='xemu')


def test_simple_generate(emulator, system, mocker):
    mocker.patch("os.path.exists", return_value=True)
    command = emulator.generate(system, dict(), keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/xemu', '-config_path', 'tests/tmp/xemu.toml']
