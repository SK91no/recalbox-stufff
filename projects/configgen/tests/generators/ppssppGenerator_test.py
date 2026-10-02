#!/usr/bin/env python
import pytest
import shutil
from configgen.Emulator import Emulator, ExtraArguments
import configgen.generators.ppsspp.ppssppControllers as ppssppControllers
import configgen.generators.ppsspp.ppssppConfig as ppssppConfig
import configgen.generators.ppsspp.ppssppGenerator as ppssppGenerator
from configgen.generators.ppsspp.ppssppGenerator import PPSSPPGenerator
from configgen.settings.keyValueSettings import keyValueSettings

import configgen.controllers.controller as controllersConfig

from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    ppssppGenerator.PPSSPPGenerator.controlsFile = 'tests/tmp/ppsspp-gamecontrollerdb.cfg'
    ppssppConfig.configFile = 'tests/tmp/ppsspp.ini'
    ppssppControllers.configFileName = 'tests/tmp/ppsspp-controls.ini'
    shutil.copyfile('tests/resources/ppsspp-controls.ini', 'tests/tmp/ppsspp-controls.ini')
    return PPSSPPGenerator()


@pytest.fixture
def system_gliden64():
    return Emulator(name='psp', videoMode='1920x1080', ratio='auto', emulator='ppsspp', core='ppsspp')


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_ppsspp(emulator, system_gliden64, controller_configuration):
    command = emulator.generate(system_gliden64, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/PPSSPPSDL', 'path/to/test',
                             ]


def read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable: bool, vulkanDriver: bool, showFps: bool = False):
    import os
    from configgen.settings.iniSettings import IniSettings
    from configgen.utils.Vulkan import Vulkan
    import configgen.recalboxFiles as recalboxFiles

    os.makedirs(str(tmp_path) + '/ppsspp/PSP/SYSTEM')
    monkeypatch.setattr(recalboxFiles, 'CONF', str(tmp_path))
    monkeypatch.setattr(recalboxFiles, 'recalboxConf', 'tests/resources/recalbox.conf')
    monkeypatch.setattr(Vulkan, 'vulkanAvailable', staticmethod(lambda: vulkanAvailable))

    options = keyValueSettings("", False)
    options.setString("global.vulkandriver", '1' if vulkanDriver else '0')
    options.setString("global.showfps", '1' if showFps else '0')
    system = Emulator(name='psp', videoMode='1920x1080', ratio='auto', emulator='ppsspp', core='ppsspp')
    system.configure(options, ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", ""))
    ppssppConfig.writePPSSPPConfig(system)

    settings = IniSettings(str(tmp_path) + '/ppsspp/PSP/SYSTEM/ppsspp.ini', True)
    settings.loadFile(True)
    return settings


def test_ppsspp_runs_on_vulkan_when_the_user_enabled_the_vulkan_driver(tmp_path, monkeypatch):
    settings = read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable=True, vulkanDriver=True)
    assert settings.getString("Graphics", 'GraphicsBackend', "") == "VULKAN"


def test_ppsspp_stays_on_opengl_when_the_vulkan_driver_is_disabled(tmp_path, monkeypatch):
    settings = read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable=True, vulkanDriver=False)
    assert settings.getString("Graphics", 'GraphicsBackend', "") == "OPENGL"


def test_ppsspp_stays_on_opengl_when_the_board_has_no_vulkan_loader(tmp_path, monkeypatch):
    settings = read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable=False, vulkanDriver=True)
    assert settings.getString("Graphics", 'GraphicsBackend', "") == "OPENGL"


def test_ppsspp_displays_fps_and_speed_when_the_user_asked_to_show_the_fps(tmp_path, monkeypatch):
    settings = read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable=False, vulkanDriver=False, showFps=True)
    assert settings.getInt("Graphics", 'iShowStatusFlags', -1) == 6


def test_ppsspp_displays_no_counter_when_the_user_did_not_ask_to_show_the_fps(tmp_path, monkeypatch):
    settings = read_ppsspp_ini(tmp_path, monkeypatch, vulkanAvailable=False, vulkanDriver=False, showFps=False)
    assert settings.getInt("Graphics", 'iShowStatusFlags', -1) == 0
