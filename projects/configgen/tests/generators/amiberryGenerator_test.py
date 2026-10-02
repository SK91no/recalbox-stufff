#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
import configgen.recalboxFiles as recalboxFiles
import configgen.generators.amiberry.amiberryKickstarts as amiberryKickstarts
from configgen.generators.amiberry.amiberryGenerator import AmiberryGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig
from tests.generators.FakeArguments import Arguments

amiberryKickstarts.recalboxFiles.BIOS = 'tests/resources/bios'


@pytest.fixture
def emulator(mocker, fake_process):
    fake_process.register_subprocess(
        ["unzip", "-l", "path/to/test.zip"], stdout=["zip result, test.slave"]
    )
    fake_process.register_subprocess(
        ["mkdir", "-p", "/recalbox/share/saves/amiga1200/whdl"]
    )
    fake_process.register_subprocess(
        ["cp", "-r", "/usr/share/amiberry/whdboot", "/tmp/amiga"]
    )
    fake_process.register_subprocess(
        ["ln", "-s", "/recalbox/share/saves/amiga1200/whdl", "/tmp/amiga/whdboot/save-data/Savegames"]
    )
    mocker.patch('os.removedirs')
    mocker.patch('os.symlink')
    return AmiberryGenerator()


@pytest.fixture(params=[
    pytest.lazy_fixture('system_a600'),
    pytest.lazy_fixture('system_a1200'),
    pytest.lazy_fixture('system_cd32'),
])
def system_hddiso(request):
    return request.param


@pytest.fixture(params=[
    pytest.lazy_fixture('system_a600'),
    pytest.lazy_fixture('system_a1200'),
])
def system_floppy(request):
    return request.param


@pytest.fixture
def system_a600():
    return Emulator(name='amiga600', videoMode='16', ratio='auto', emulator='amiberry', core='amiberry')


@pytest.fixture
def system_a1200():
    return Emulator(name='amiga1200', videoMode='16', ratio='auto', emulator='amiberry', core='amiberry')


@pytest.fixture
def system_cd32():
    return Emulator(name='amigacd32', videoMode='16', ratio='auto', emulator='amiberry', core='amiberry')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_rom_iso(emulator, system_hddiso, controller_configuration):
    command = emulator.generate(system_hddiso, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.iso'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/amiberry', '--config', '/tmp/amiga/conf/default.uae', '-G']


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_rom_zip(emulator, system_hddiso, controller_configuration):
    command = emulator.generate(system_hddiso, controller_configuration, keyValueSettings("", False), Arguments('path/to/test.zip'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/amiberry', '--autoload', 'path/to/test.zip', '-G']


def test_simple_generate_rom_adf(emulator, system_floppy):
    command = emulator.generate(system_floppy, dict(), keyValueSettings("", False), Arguments('path/to/test.adf'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/amiberry', '--config', '/tmp/amiga/conf/default.uae', '-G']


def test_simple_generate_rom_rp9(emulator, system_floppy):
    command = emulator.generate(system_floppy, dict(), keyValueSettings("", False), Arguments('path/to/test.rp9'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/amiberry', '--config', 'path/to/test.rp9', '-G']


def test_simple_generate_rom_lha(emulator, system_hddiso):
    command = emulator.generate(system_hddiso, dict(), keyValueSettings("", False), Arguments('path/to/test.lha'))
    assert command.videomode == '16'
    assert command.array == ['/usr/bin/amiberry', '--autoload', 'path/to/test.lha', '-G']


def readGeneratedConfig() -> dict:
    settings = keyValueSettings('/tmp/amiga/conf/default.uae')
    settings.loadFile(True)
    return settings


# The game hdf boots on DH0, saves go on DH1: the companion hdf of the amiga saves share when
# there is one, a plain directory mount otherwise.
def test_generate_rom_hdf_mounts_save_drive(emulator, system_floppy, tmp_path, monkeypatch):
    monkeypatch.setattr(recalboxFiles, 'SAVES', str(tmp_path))

    command = emulator.generate(system_floppy, dict(), keyValueSettings("", False), Arguments('path/to/test.hdf'))
    assert command.array == ['/usr/bin/amiberry', '--config', '/tmp/amiga/conf/default.uae', '-G']

    settings = readGeneratedConfig()
    assert settings.getString('uaehf0', '').startswith('hdf,rw,DH0:path/to/test.hdf,')
    # No test-Saves.hdf around: fallback directory mount, created with its WHDSaves drawer
    assert settings.getString('uaehf1', '') == 'dir,rw,DH1:Saves:{}/amiga/test,-128'.format(tmp_path)
    assert (tmp_path / 'amiga' / 'test' / 'WHDSaves').is_dir()

    # Now with the companion save hdf: it takes over the directory mount
    savesHdf = tmp_path / 'amiga' / 'test-Saves.hdf'
    savesHdf.touch()
    emulator.generate(system_floppy, dict(), keyValueSettings("", False), Arguments('path/to/test.hdf'))
    assert readGeneratedConfig().getString('uaehf1', '').startswith('hdf,rw,DH1:{},'.format(savesHdf))
