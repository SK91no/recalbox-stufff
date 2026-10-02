#!/usr/bin/env python
import os
import shutil
from unittest.mock import MagicMock
import pytest
from configgen.Emulator import Emulator, ExtraArguments
import configgen.generators.supermodel.supermodelGenerator as supermodelGenerator
from configgen.generators.supermodel.supermodelGenerator import SupermodelGenerator
import configgen.generators.supermodel.supermodelControllers as supermodelControllers
import configgen.controllers.controller as controllersConfig
import configgen.recalboxFiles as recalboxFiles
from configgen.settings.keyValueSettings import keyValueSettings
from configgen.utils.videoMode import getCrtResolution, getCrtResolutionDetail
from tests.generators.FakeArguments import Arguments


@pytest.fixture
def emulator():
    supermodelGenerator.SupermodelGenerator.configFile = 'tests/resources/ConfigModel3.ini'
    supermodelGenerator.SupermodelGenerator.configFile = 'tests/resources/ConfigModel3.ini'
    supermodelGenerator.SupermodelGenerator.controlsFile = 'tests/tmp/supermodel.ini'

    return SupermodelGenerator()


@pytest.fixture
def system():
    return Emulator(name='supermodel', videoMode='1920x1080', ratio='auto', emulator='supermodel', core='supermodel')

def configureForCrt(emulator: Emulator, crtvideostandard="auto", crtresolutiontype="auto", crtscreentype="15kHz",
                    crtadaptor="recalboxrgbdual", crtregion="auto", crtscanlines="", rotation=0, verticalgame=False,
                    crtsuperrez = "1", crtsignaltype = "RGB", widescreen=False, avoid_interlaced = False, avoid_interlaced_on_tate_and_handhelds=False
                    ):
    recalCrtConf = keyValueSettings("")
    recalCrtConf.setBool("options.video.avoidinterlaced", avoid_interlaced)
    recalCrtConf.setBool("options.video.avoidinterlacedontatehandhelds", avoid_interlaced_on_tate_and_handhelds)
    emulator.configure(keyValueSettings(""),
                       ExtraArguments("", "", "", "", "", "", "", "", "", crtvideostandard, crtresolutiontype,
                                      crtscreentype,
                                      crtadaptor, crtregion,
                                      crtscanlines, rotation=rotation, verticalgame=verticalgame,
                                      crtsuperrez=crtsuperrez, crtsignaltype=crtsignaltype),
                       recalCrtConf)
    emulator.WideScreenMode = widescreen
    return emulator


@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_supermodel_on_pi5_wlr(emulator, system, mocker, controller_configuration, fake_process):
    fake_process.register_subprocess(
        ["tvservice -s"],
        stdout=['[{ "code":1, "width":1920, "height":1080 }]']
    )
    mocker.patch('platform.machine', return_value='arm64')
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['bash', '-c', "wlr-randr --output '' --mode 1920x1080@60Hz && '/usr/bin/supermodel' 'path/to/test' '-sound-volume=100' '-music-volume=100' '-no-flip-stereo' '-sound' '-dsb' '-new-scsp' '-fullscreen' '-res=1920,1080' '-ss=1' '-no-wide-screen' '-upscalemode=0' '-throttle' '-vsync' '-crosshairs=1' '-no-multi-texture' '-legacy3d' '-gpu-multi-threaded' '-ppc-frequency=100' '-log-output=/recalbox/share/system/configs/model3/Supermodel.log' '-log-level=info'",
]

@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_supermodel_pc(emulator, system, mocker, controller_configuration, fake_process):
    fake_process.register_subprocess(
        ["tvservice -s"],
        stdout=['[{ "code":1, "width":1920, "height":1080 }]']
    )
    mocker.patch('platform.machine', return_value='x86_64')
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)
    command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('path/to/test'))
    assert command.videomode == '1920x1080'
    assert command.array == ['/usr/bin/supermodel', 'path/to/test',
                             '-sound-volume=100', '-music-volume=100',
                             '-no-flip-stereo', '-sound',
                             '-dsb', '-new-scsp',
                             '-fullscreen', '-res=1920,1080',
                             '-ss=1', '-no-wide-screen',
                             '-upscalemode=0', '-throttle',
                             '-vsync', '-crosshairs=1',
                             '-no-multi-texture', '-legacy3d',
                             '-gpu-multi-threaded', '-ppc-frequency=100',
                             '-log-output=/recalbox/share/system/configs/model3/Supermodel.log',
                             '-log-level=info']

def test_find_resolution_on_1080p(emulator, system, mocker, controller_configuration, fake_process):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    assert SupermodelGenerator.FindResolution(system) == (1920, 1080, True)

def test_find_resolution_on_640x480p(emulator, system, mocker, controller_configuration, fake_process):
    model3ini = keyValueSettings(supermodelGenerator.SupermodelGenerator.configFile).loadFile()
    model3ini.setString("resolution", "auto").saveFile()
    system =  Emulator(name='supermodel', videoMode='640x480', ratio='auto', emulator='supermodel', core='supermodel')
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)
    assert emulator.FindResolution(system) == (640, 480, False)

def test_find_resolution_on_forced_config(emulator, system, mocker, controller_configuration, fake_process):
    model3ini = keyValueSettings(supermodelGenerator.SupermodelGenerator.configFile).loadFile()
    model3ini.setString("resolution", "1024,768").saveFile()
    system =  Emulator(name='supermodel', videoMode='640x480', ratio='auto', emulator='supermodel', core='supermodel')
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)
    assert emulator.FindResolution(system) == (1024, 768, False)

def crtModel3(mocker, interlacedCapable: bool, screenType="15kHz", resolutionType="auto", avoidInterlaced=False):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=not interlacedCapable,
                 new_callable=mocker.PropertyMock)
    return configureForCrt(
        Emulator(name='model3', videoMode='320x240', ratio='auto', emulator='supermodel', core='supermodel'),
        crtadaptor="recalboxrgbdual", crtscreentype=screenType, crtresolutiontype=resolutionType,
        avoid_interlaced=avoidInterlaced)


def test_find_resolution_on_crt(emulator, system, mocker, controller_configuration, fake_process):
    model3ini = keyValueSettings(supermodelGenerator.SupermodelGenerator.configFile).loadFile()
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=True, new_callable=mocker.PropertyMock)
    model3ini.setString("resolution", "auto").saveFile()
    system = configureForCrt(
        Emulator(name='supermodel', videoMode='320x240', ratio='auto', emulator='supermodel',core='supermodel'),
        crtadaptor="recalboxrgbdual")
    assert emulator.FindResolution(system) == (320, 240, False)


def test_model3_in_auto_runs_480i_on_a_15khz_screen(mocker):
    system = crtModel3(mocker, interlacedCapable=True)
    assert getCrtResolutionDetail(system) == (640, 480, True)
    assert getCrtResolution(system) == "640x480i"


def test_model3_in_auto_keeps_its_native_384p_on_a_15_25khz_screen(mocker):
    system = crtModel3(mocker, interlacedCapable=True, screenType="kHzMulti1525")
    assert getCrtResolutionDetail(system) == (496, 384, False)


def test_model3_in_auto_keeps_its_native_384p_on_a_tri_frequency_screen(mocker):
    system = crtModel3(mocker, interlacedCapable=True, screenType="kHzTriFreq")
    assert getCrtResolutionDetail(system) == (496, 384, False)


def test_model3_in_auto_runs_480p_on_a_15_31khz_screen(mocker):
    system = crtModel3(mocker, interlacedCapable=True, screenType="kHzMulti1531")
    assert getCrtResolutionDetail(system) == (640, 480, False)


def test_model3_in_auto_stays_240p_when_the_adapter_cannot_do_interlaced(mocker):
    system = crtModel3(mocker, interlacedCapable=False)
    assert getCrtResolutionDetail(system) == (320, 240, False)


def test_model3_in_auto_stays_240p_when_the_user_avoids_interlaced(mocker):
    system = crtModel3(mocker, interlacedCapable=True, avoidInterlaced=True)
    assert getCrtResolutionDetail(system) == (320, 240, False)


def test_model3_forced_to_240p_ignores_the_auto_480i_rule(mocker):
    system = crtModel3(mocker, interlacedCapable=True, resolutionType="240")
    assert getCrtResolutionDetail(system) == (320, 240, False)


def test_another_system_in_auto_keeps_its_own_crt_resolutions(mocker):
    mocker.patch('configgen.utils.architecture.Architecture.isPi5', return_value=False, new_callable=mocker.PropertyMock)
    system = configureForCrt(
        Emulator(name='snes', videoMode='320x240', ratio='auto', emulator='libretro', core='snes9x'),
        crtadaptor="recalboxrgbdual")
    assert getCrtResolutionDetail(system) == (320, 240, False)


def test_model3_in_auto_asks_supermodel_for_640x480(emulator, mocker):
    system = crtModel3(mocker, interlacedCapable=True)
    assert emulator.GetResolution(system) == ["-fullscreen", "-res=640,480"]


# --- Controller mapping coverage --------------------------------------------

_PS3_GUID = "060000004c0500006802000000010000"
_PS3_NAME = "PLAYSTATION(R)3 Controller (00:48:E8:D1:63:25)"
_GUNCON2_GUID = "030033ba9a0b00006a01000000010000"
_GUNCON2_NAME = "Namco GunCon 2"


def _fake_capabilities():
    return {1: [305, 304, 545, 316, 312, 317, 546, 311, 310, 313, 318, 547, 314, 315, 544, 307, 308]}


def _load_controllers(player_count: int, guid: str = _PS3_GUID, name: str = _PS3_NAME,
                      axes: int = 6, hats: int = 1, buttons: int = 20):
    res = os.path.join(os.path.dirname(__file__), "..", "resources")
    shutil.copyfile(os.path.join(res, "es_input.cfg.origin"), os.path.join(res, "es_input.cfg"))
    controllersConfig.esInputs = os.path.abspath(os.path.join(res, "es_input.cfg"))
    controllersConfig.getControllerIndex = MagicMock(side_effect=list(range(player_count)) + [-1] * 10)
    controllersConfig.Controller._GetInputCapabilities = MagicMock(return_value=_fake_capabilities())
    kwargs = {}
    for i in range(1, player_count + 1):
        kwargs.update({
            "p{}index".format(i): i - 1, "p{}guid".format(i): guid, "p{}name".format(i): name,
            "p{}devicepath".format(i): "/fakedev/input/event{}".format(i),
            "p{}nbaxes".format(i): axes, "p{}nbhats".format(i): hats, "p{}nbbuttons".format(i): buttons,
        })
    return controllersConfig.Controller.LoadUserControllerConfigurations(**kwargs)


def _generate_controls(controllers, system, tmp_path, monkeypatch):
    conf = tmp_path / "configs"
    (conf / "model3").mkdir(parents=True)
    monkeypatch.setattr(recalboxFiles, "CONF", str(conf))
    supermodelControllers.generateControllerConfig(SupermodelGenerator(), controllers, system)
    out = {}
    for line in (conf / "model3" / "Supermodel.ini").read_text().splitlines():
        line = line.strip()
        if "=" in line and not line.startswith("["):
            key, _, value = line.partition("=")
            out[key.strip()] = value.strip()
    return out


def test_controls_player1_and_player2_are_both_mapped(system, tmp_path, monkeypatch):
    controls = _generate_controls(_load_controllers(2), system, tmp_path, monkeypatch)
    # Each player writes to its own JOY index (P1 -> JOY1, P2 -> JOY2).
    assert controls["InputCoin1"].startswith('"JOY1_')
    assert controls["InputCoin2"].startswith('"JOY2_')
    assert controls["InputStart1"].startswith('"JOY1_')
    assert controls["InputStart2"].startswith('"JOY2_')
    assert controls["InputJoyUp"].startswith('"JOY1_')
    assert controls["InputJoyUp2"].startswith('"JOY2_')
    # Same pad model → same physical button, only the JOY index differs.
    assert controls["InputCoin1"].split("_", 1)[1] == controls["InputCoin2"].split("_", 1)[1]


def test_controls_single_player_does_not_emit_player2(system, tmp_path, monkeypatch):
    controls = _generate_controls(_load_controllers(1), system, tmp_path, monkeypatch)
    assert "InputCoin1" in controls
    assert "InputCoin2" not in controls
    assert "InputJoyUp2" not in controls

def _load_guncons(player_count: int):
    return _load_controllers(player_count, guid=_GUNCON2_GUID, name=_GUNCON2_NAME, axes=2, hats=1, buttons=8)


def test_guncon_aims_with_its_own_axes_and_fires_with_its_trigger(system, tmp_path, monkeypatch):
    controls = _generate_controls(_load_guncons(1), system, tmp_path, monkeypatch)
    # es_input.cfg maps neither the gun axes nor the trigger, so nothing may be
    # left over from a previous pad session.
    assert controls["InputGunX"] == '"JOY1_XAXIS"'
    assert controls["InputGunY"] == '"JOY1_YAXIS"'
    assert controls["InputAnalogGunX"] == '"JOY1_XAXIS"'
    assert controls["InputAnalogGunY"] == '"JOY1_YAXIS"'
    # BUTTON6 is BTN_LEFT: the physical trigger, not the A/B buttons.
    assert controls["InputTrigger"] == '"JOY1_BUTTON6"'
    assert controls["InputAnalogTriggerLeft"] == '"JOY1_BUTTON6"'
    assert controls["InputAnalogJoyTrigger"] == '"JOY1_BUTTON6"'


def test_second_guncon_gets_the_player2_gun_keys(system, tmp_path, monkeypatch):
    controls = _generate_controls(_load_guncons(2), system, tmp_path, monkeypatch)
    assert controls["InputGunX2"] == '"JOY2_XAXIS"'
    assert controls["InputTrigger2"] == '"JOY2_BUTTON6"'
    assert controls["InputAnalogTriggerLeft2"] == '"JOY2_BUTTON6"'


def test_dolphinbar_gun_aims_and_fires_with_the_mouse(system, tmp_path, monkeypatch):
    monkeypatch.setattr(supermodelControllers, "hasDolphinBar", lambda: True)
    controls = _generate_controls(_load_controllers(1), system, tmp_path, monkeypatch)
    assert controls["InputGunX"] == '"MOUSE_XAXIS"'
    assert controls["InputAnalogGunY"] == '"MOUSE_YAXIS"'
    # The wiimote trigger is a left click, never a pad button.
    assert controls["InputTrigger"] == '"MOUSE_LEFT_BUTTON"'
    assert controls["InputAnalogTriggerLeft"] == '"MOUSE_LEFT_BUTTON"'
    assert controls["InputAnalogTriggerRight"] == '"MOUSE_RIGHT_BUTTON"'


def test_dolphinbar_leaves_player2_on_its_pad(system, tmp_path, monkeypatch):
    monkeypatch.setattr(supermodelControllers, "hasDolphinBar", lambda: True)
    controls = _generate_controls(_load_controllers(2), system, tmp_path, monkeypatch)
    assert controls["InputGunX2"] == '"JOY2_XAXIS"'


def test_a_guncon_is_preferred_over_a_detected_dolphinbar(system, tmp_path, monkeypatch):
    monkeypatch.setattr(supermodelControllers, "hasDolphinBar", lambda: True)
    controls = _generate_controls(_load_guncons(1), system, tmp_path, monkeypatch)
    assert controls["InputGunX"] == '"JOY1_XAXIS"'
    assert controls["InputTrigger"] == '"JOY1_BUTTON6"'
