#!/usr/bin/env python
import os
import shutil
from unittest.mock import MagicMock

import pytest

import configgen.controllers.controller as controllersConfig
import configgen.generators.supermodel.supermodelControllers as supermodelControllers
from configgen.Emulator import Emulator
from configgen.generators.supermodel.supermodelGenerator import SupermodelGenerator
from configgen.settings.iniSettings import IniSettings
from tests.conftest import fake_capabilities


def resource(name: str) -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'resources', name))


@pytest.fixture
def pro2(monkeypatch):
    monkeypatch.setattr(controllersConfig, 'esInputs', resource('es_input_8bitdo_pro2.cfg'))
    monkeypatch.setattr(controllersConfig, 'getControllerIndex', MagicMock(side_effect=[0] + [-1] * 9))
    monkeypatch.setattr(controllersConfig.Controller, '_GetInputCapabilities',
                        MagicMock(return_value=fake_capabilities()))
    return controllersConfig.Controller.LoadUserControllerConfigurations(
        p1index=0,
        p1guid="03007f6ec82d00001030000011010000",
        p1name="8BitDo Pro 2 Wired Controller",
        p1devicepath="/fakedev/input/event1",
        p1nbaxes=6,
        p1nbhats=1,
        p1nbbuttons=28,
    )


@pytest.fixture
def written(pro2, tmp_path, monkeypatch) -> IniSettings:
    conf = tmp_path / 'configs'
    (conf / 'model3').mkdir(parents=True)
    shutil.copyfile(resource('ConfigModel3.ini'), str(conf / 'model3' / 'ConfigModel3.ini'))
    monkeypatch.setattr(supermodelControllers.recalboxFiles, 'CONF', str(conf))
    supermodelControllers.generateControllerConfig(
        SupermodelGenerator(), pro2,
        Emulator(name='model3', videoMode='1920x1080', ratio='auto',
                 emulator='supermodel', core='supermodel'))
    settings = IniSettings(str(conf / 'model3' / 'Supermodel.ini'), True)
    settings.loadFile(True)
    return settings


def test_the_pedals_follow_the_axis_the_pad_declares_not_the_usual_layout(written):
    assert written.getString('Global', 'InputAccelerator', '') == '"JOY1_AXIS5_POS"'
    assert written.getString('Global', 'InputBrake', '') == '"JOY1_AXIS6_POS"'


def test_no_axis_is_named_after_a_layout_this_pad_does_not_have(written):
    for _, value in written.items('Global'):
        assert 'RXAXIS' not in value and 'RZAXIS' not in value and 'ZAXIS' not in value
