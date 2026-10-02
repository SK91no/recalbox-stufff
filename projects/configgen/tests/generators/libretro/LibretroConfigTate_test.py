import os
from typing import Optional, Dict
from unittest.mock import mock_open

import pytest

from configgen.Emulator import Emulator, ExtraArguments
import configgen.controllers.controller as controllersConfig
from configgen.crt.CRTConfigParser import CRTConfigParser
from configgen.crt.CRTTypes import CRTSystemMode, CRTArcadeMode
from configgen.crt.CRTModeOffsetter import CRTModeOffsetter
from configgen.generators.libretro.crt.LibretroConfigCRT import LibretroConfigCRT
from configgen.generators.libretro.libretroControllers import LibretroControllers
from configgen.generators.libretro.libretroGenerator import LibretroGenerator
from configgen.settings.keyValueSettings import keyValueSettings


@pytest.fixture
def system_fbneo():
    return Emulator(name='fbneo', videoMode='1920x1080', ratio='auto', emulator='libretro',
                    core='fbneo')
@pytest.fixture
def system_wswanc():
    return Emulator(name='wswanc', videoMode='1920x1080', ratio='auto', emulator='libretro',
                    core='mednafen_wswan')


def configureForTate(system: Emulator, rotation=0, vertical_game=True, rotatecontrols=False, crtscreentype: str="15kHz"):
    opts = keyValueSettings("")
    extraArgs = ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", crtscreentype, "", rotation=rotation,
                               rotatecontrols=rotatecontrols, verticalgame=vertical_game, )
    system.configure(opts, extraArgs)


def test_given_rotation_0_in_cli_then_dont_rotate(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=0)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 0


def test_given_rotation_1_in_cli_then_rotate(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=1)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 1


def test_given_vertical_game_and_rotated_then_set4_3_ratio(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=1, vertical_game=True)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert config["video_allow_rotate"] == '"true"'
    assert config["aspect_ratio_index"] == 0

def test_given_vertical_game_and_no_rotate_then_dont_set_ratio(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=0, vertical_game=True)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert "aspect_ratio_index" not in config

def test_given_horizontal_game_and_no_rotate_then_dont_set_ratio(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=0, vertical_game=False)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert "aspect_ratio_index" not in config

def test_given_horizontal_game_and_rotate_then_set_3_4_ratio(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=1, vertical_game=False)
    config = []
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert config["aspect_ratio_index"] == 8

def test_given_rotated_then_disable_overlays(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=1)
    config = []
    LibretroGenerator().processOverlays(system_fbneo, "anyrom.zip")
    assert config == []

def test_given_rotated_then_disable_integer_scale(mocker, system_fbneo):
    configureForTate(system_fbneo, rotation=1)
    config, core = LibretroGenerator().createTateModeConfiguration(system_fbneo)

    assert config["video_scale_integer"] == '"false"'


controllersConfig.esInputs = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../resources/es_input.cfg"))


@pytest.mark.usefixtures("controller_configuration")
def test_given_rotated_input_then_return_right_joystick_as_left(system_fbneo, controller_configuration):
    lrControllers = LibretroControllers(system_fbneo, keyValueSettings(""), keyValueSettings(""), keyValueSettings(""), keyValueSettings(""), controller_configuration, False, "")
    configureForTate(system_fbneo, rotation=1, rotatecontrols=True)
    config, remap = lrControllers.fillControllersConfiguration()
    assert config.getString('input_player1_l_x_minus_axis', "") == "+3"
    assert config.getString('input_player1_l_x_plus_axis', "") == "-3"
    assert config.getString('input_player1_l_y_minus_axis', "") == "-2"
    assert config.getString('input_player1_l_y_plus_axis', "") == "+2"


def _fill_tate(system, controllers, rotatecontrols):
    lr = LibretroControllers(system, keyValueSettings(""), keyValueSettings(""),
                             keyValueSettings(""), keyValueSettings(""), controllers, False, "")
    configureForTate(system, rotation=1, rotatecontrols=rotatecontrols)
    config, _ = lr.fillControllersConfiguration()
    return config


@pytest.mark.usefixtures("controller_configuration")
def test_tate_rotates_face_buttons_and_analog_but_not_dpad(system_fbneo, controller_configuration):
    """High-level characterization of TATE control rotation via the full
    generated RetroArch config, so the rotation can be refactored safely.
    Relational (base vs rotated) so it does not depend on the physical ids."""
    base = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=False)
    rot = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=True)

    def g(cfg, key):
        return cfg.getString(key, "")

    # Face buttons rotate 90 degrees: physical A->b, B->y, X->a, Y->x
    assert g(rot, 'input_player1_b_btn') == g(base, 'input_player1_a_btn')  # A now drives retropad b
    assert g(rot, 'input_player1_y_btn') == g(base, 'input_player1_b_btn')  # B -> y
    assert g(rot, 'input_player1_a_btn') == g(base, 'input_player1_x_btn')  # X -> a
    assert g(rot, 'input_player1_x_btn') == g(base, 'input_player1_y_btn')  # Y -> x

    # D-pad is NOT rotated (a rotated cab's stick is physically rotated)
    for d in ('up', 'down', 'left', 'right'):
        assert g(rot, f'input_player1_{d}_btn') == g(base, f'input_player1_{d}_btn'), d

    # Shoulders / triggers / start / select are untouched by the rotation
    for k in ('l_btn', 'r_btn', 'l2_btn', 'r2_btn', 'l3_btn', 'r3_btn', 'start_btn', 'select_btn'):
        assert g(rot, f'input_player1_{k}') == g(base, f'input_player1_{k}'), k

    # Analog sticks ARE rotated: left<->right swap with sign flips
    assert (g(rot, 'input_player1_l_x_minus_axis'), g(rot, 'input_player1_l_x_plus_axis')) == ("+3", "-3")
    assert (g(rot, 'input_player1_l_y_minus_axis'), g(rot, 'input_player1_l_y_plus_axis')) == ("-2", "+2")
    assert (g(rot, 'input_player1_r_x_minus_axis'), g(rot, 'input_player1_r_x_plus_axis')) == ("-0", "+0")
    assert (g(rot, 'input_player1_r_y_minus_axis'), g(rot, 'input_player1_r_y_plus_axis')) == ("-1", "+1")


@pytest.mark.usefixtures("controller_configuration")
def test_no_tate_leaves_face_buttons_and_analog_identity(system_fbneo, controller_configuration):
    """Guard the non-rotated baseline: face buttons map 1:1, analog not swapped."""
    base = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=False)
    # a->a, b->b, x->x, y->y (no rotation). Button ids are stored as ints.
    assert base.getString('input_player1_a_btn', "") == 1
    assert base.getString('input_player1_b_btn', "") == 0
    assert base.getString('input_player1_x_btn', "") == 2
    assert base.getString('input_player1_y_btn', "") == 3
    # analog identity (l stays l, no sign flip)
    assert base.getString('input_player1_l_x_minus_axis', "") == "-0"
    assert base.getString('input_player1_l_y_minus_axis', "") == "-1"


@pytest.mark.usefixtures("controller_configuration")
def test_tate_swaps_analog_sticks_left_right(system_fbneo, controller_configuration):
    """TATE rotates the sticks 90 degrees: the left physical stick drives the
    RIGHT retroarch axes and vice-versa. Relational on the axis id (sign aside)."""
    base = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=False)
    rot = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=True)

    def aid(cfg, key):  # axis id, sign stripped
        return cfg.getString(key, "").lstrip("+-")

    # left physical stick (base l_*) now drives the right retroarch axes
    assert aid(rot, 'input_player1_r_x_minus_axis') == aid(base, 'input_player1_l_x_minus_axis')
    assert aid(rot, 'input_player1_r_y_minus_axis') == aid(base, 'input_player1_l_y_minus_axis')
    # right physical stick (base r_*) now drives the left retroarch axes
    assert aid(rot, 'input_player1_l_x_minus_axis') == aid(base, 'input_player1_r_y_minus_axis')
    assert aid(rot, 'input_player1_l_y_minus_axis') == aid(base, 'input_player1_r_x_minus_axis')


@pytest.mark.usefixtures("controller_configuration")
def test_rg353m_flag_drives_full_tate_rotation(system_fbneo, controller_configuration):
    """rg353m scenario end-to-end: a PLAIN gamepad (2 sticks + d-pad + face
    buttons, NOT a JVS pad, NOT an arcade stick) on a vertical game. The
    -rotatecontrols flag (Emulator.RotateControls) is the sole trigger, so we
    pin the ABSOLUTE RetroArch bindings for both flag states (the sibling
    characterization tests are only relational)."""
    p1 = controller_configuration[1]
    # This is the rotatable-handheld case: a regular pad, neither JVS nor arcade.
    assert not p1.DeviceName.startswith("RecalboxJVS Gamepad")
    assert not p1.GamepadType.isArcade()

    base = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=False)
    rot = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=True)

    # --- flag OFF: identity (nothing rotated). Button ids stored as ints. ---
    assert base.getString('input_player1_a_btn', "") == 1
    assert base.getString('input_player1_b_btn', "") == 0
    assert base.getString('input_player1_x_btn', "") == 2
    assert base.getString('input_player1_y_btn', "") == 3
    # left physical stick stays on l_* (Joy1), right stays on r_* (Joy2), no flip.
    assert base.getString('input_player1_l_x_minus_axis', "") == "-0"
    assert base.getString('input_player1_l_y_minus_axis', "") == "-1"
    assert base.getString('input_player1_r_x_minus_axis', "") == "-2"

    # --- flag ON: face buttons rotate 90° (A->b, B->y, X->a, Y->x) ---
    assert rot.getString('input_player1_b_btn', "") == 1   # physical A -> retropad b
    assert rot.getString('input_player1_y_btn', "") == 0   # physical B -> retropad y
    assert rot.getString('input_player1_a_btn', "") == 2   # physical X -> retropad a
    assert rot.getString('input_player1_x_btn', "") == 3   # physical Y -> retropad x

    # --- flag ON: analog sticks swap L<->R with the joystick2up sign flip
    #     (_rotate.axes + _rotate.invertAxes) ---
    assert (rot.getString('input_player1_l_x_minus_axis', ""),
            rot.getString('input_player1_l_x_plus_axis', "")) == ("+3", "-3")
    assert (rot.getString('input_player1_l_y_minus_axis', ""),
            rot.getString('input_player1_l_y_plus_axis', "")) == ("-2", "+2")
    assert (rot.getString('input_player1_r_x_minus_axis', ""),
            rot.getString('input_player1_r_x_plus_axis', "")) == ("-0", "+0")
    assert (rot.getString('input_player1_r_y_minus_axis', ""),
            rot.getString('input_player1_r_y_plus_axis', "")) == ("-1", "+1")


@pytest.mark.usefixtures("controller_configuration")
def test_rg353m_tate_dpad_keeps_native_binding(system_fbneo, controller_configuration):
    """The d-pad must NOT be rotated (a rotatable handheld's whole pad turns with
    the screen): up/down/left/right keep the exact native binding they have
    without the flag."""
    base = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=False)
    rot = _fill_tate(system_fbneo, controller_configuration, rotatecontrols=True)
    for d in ('up', 'down', 'left', 'right'):
        native = base.getString('input_player1_%s_btn' % d, "")
        assert native != "", d                                    # native binding exists
        assert rot.getString('input_player1_%s_btn' % d, "") == native, d


def test_given_tate_mode_wonderswan_game_then_configure_core_and_retroarch(system_wswanc):
    configureForTate(system_wswanc, rotation=1, vertical_game=True)
    config, core = LibretroGenerator().createTateModeConfiguration(system_wswanc)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 2
    assert core["wswan_rotate_keymap"] == '"enabled"'
    assert core["wswan_rotate_display"] == '"manual"'


def test_given_tate_mode_dreamcast_game_then_configure_retroarch_with_specific_rotation():
    dreamcast = Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro',
                    core='flycast')
    configureForTate(dreamcast, rotation=1)
    config, core = LibretroGenerator().createTateModeConfiguration(dreamcast)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 2
    assert config["input_player1_analog_dpad_mode"] == 3


def test_given_tate_mode_naomi_game_then_configure_retroarch():
    dreamcast = Emulator(name='naomi', videoMode='1920x1080', ratio='auto', emulator='libretro',
                    core='flycast')
    configureForTate(dreamcast, rotation=1)
    config, core = LibretroGenerator().createTateModeConfiguration(dreamcast)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 1
    assert config["input_player1_analog_dpad_mode"] == 3

def test_given_tate_mode_saturn_game_then_configure_retroarch_with_specific_rotation(controller_configuration):
    saturn = Emulator(name='saturn', videoMode='1920x1080', ratio='auto', emulator='libretro',
                         core='yabasanshiro')
    configureForTate(saturn, rotation=1)
    config, core = LibretroGenerator().createTateModeConfiguration(saturn)

    assert config["video_allow_rotate"] == '"true"'
    assert config["video_rotation"] == 2
