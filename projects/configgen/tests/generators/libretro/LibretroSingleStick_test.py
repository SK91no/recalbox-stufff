import os
from unittest.mock import MagicMock

import pytest

import configgen.controllers.controller as controllersConfig
from configgen.Emulator import Emulator, ExtraArguments
from configgen.generators.libretro.libretroControllers import LibretroControllers
from configgen.settings.keyValueSettings import keyValueSettings


@pytest.fixture
def single_stick_controller():
    # Arcade stick: the sole analog stick is bound as up/down/left/right, no joystick1/2.
    controllersConfig.esInputs = os.path.abspath(
        os.path.join(os.path.dirname(__file__), "../../resources/es_input.cfg.arcadestick"))
    controllersConfig.getControllerIndex = MagicMock(side_effect=[0, -1, -1, -1, -1, -1, -1, -1, -1, -1])
    controllersConfig.Controller._GetInputCapabilities = MagicMock(
        return_value={1: [308, 307, 304, 317, 305, 318, 315, 316]})
    return controllersConfig.Controller.LoadUserControllerConfigurations(
        p1index=0, p1guid="0300e9dac82d00000631000014010000", p1name="Single Stick Arcade",
        p1devicepath="/fakedev/input/event1", p1nbaxes=2, p1nbhats=0, p1nbbuttons=11,
        p2index=1, p2guid="", p2name="", p2devicepath="", p2nbaxes=-1, p2nbhats=-1, p2nbbuttons=-1,
        p3index=2, p3guid="", p3name="", p3devicepath="", p3nbaxes=-1, p3nbhats=-1, p3nbbuttons=-1,
        p4index=3, p4guid="", p4name="", p4devicepath="", p4nbaxes=-1, p4nbhats=-1, p4nbbuttons=-1,
        p5index=4, p5guid="", p5name="", p5devicepath="", p5nbaxes=-1, p5nbhats=-1, p5nbbuttons=-1,
        p6index=5, p6guid="", p6name="", p6devicepath="", p6nbaxes=-1, p6nbhats=-1, p6nbbuttons=-1,
        p7index=6, p7guid="", p7name="", p7devicepath="", p7nbaxes=-1, p7nbhats=-1, p7nbbuttons=-1,
        p8index=7, p8guid="", p8name="", p8devicepath="", p8nbaxes=-1, p8nbhats=-1, p8nbbuttons=-1,
        p9index=8, p9guid="", p9name="", p9devicepath="", p9nbaxes=-1, p9nbhats=-1, p9nbbuttons=-1,
        p10index=9, p10guid="", p10name="", p10devicepath="", p10nbaxes=-1, p10nbhats=-1, p10nbbuttons=-1)


def _configure(controllers, rom):
    naomi2 = Emulator(name='naomi2', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast-next')
    retroarchConfig = keyValueSettings("")
    lr = LibretroControllers(naomi2, keyValueSettings(""), retroarchConfig, keyValueSettings(""),
                             keyValueSettings(""), controllers, False, rom)
    naomi2.configure(keyValueSettings(""),
                     ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", ""))
    lr.fillControllersConfiguration()
    return retroarchConfig


def test_analog_game_routes_single_stick_to_left_analog(single_stick_controller):
    # Club Kart is analog: the stick must drive l_x/l_y and NOT the d-pad (down would be VIEW).
    s = _configure(single_stick_controller, "clubkrt.zip")
    assert s.getString("input_player1_l_x_minus_axis", "") == "-0"
    assert s.getString("input_player1_l_x_plus_axis", "") == "+0"
    assert s.getString("input_player1_l_y_minus_axis", "") == "-1"
    assert s.getString("input_player1_l_y_plus_axis", "") == "+1"
    assert not s.hasOption("input_player1_up_axis")
    assert not s.hasOption("input_player1_down_axis")


def test_digital_game_keeps_single_stick_on_dpad(single_stick_controller):
    # Beach Spikers is digital: the stick stays on the d-pad, no analog binding.
    s = _configure(single_stick_controller, "beachspi.zip")
    assert s.getString("input_player1_up_axis", "") == "-1"
    assert s.getString("input_player1_down_axis", "") == "+1"
    assert s.getString("input_player1_left_axis", "") == "-0"
    assert s.getString("input_player1_right_axis", "") == "+0"
    assert not s.hasOption("input_player1_l_x_minus_axis")
    assert not s.hasOption("input_player1_l_y_minus_axis")


def test_pedal_game_maps_accel_brake_to_face_buttons(single_stick_controller):
    # Club Kart is a racing game with no analog triggers on this stick: every driving control moves to a
    # face button — B=accel (R2), A=brake (L2), Y=up (gear up/drive), X=down (gear down/reverse) — the
    # stick keeps steering (analog), and each button loses its native role.
    s = _configure(single_stick_controller, "clubkrt.zip")
    assert s.getString("input_player1_r2_btn", "") != ""          # accel on B
    assert s.getString("input_player1_l2_btn", "") != ""          # brake on A
    assert s.getString("input_player1_up_btn", "") != ""          # gear up on Y
    assert s.getString("input_player1_down_btn", "") != ""        # gear down on X
    assert s.getString("input_player1_l_x_minus_axis", "") == "-0"  # stick still steers
    # native face-button roles dropped, and the stick is not on the d-pad.
    for opt in ("a_btn", "b_btn", "x_btn", "y_btn", "up_axis", "down_axis"):
        assert not s.hasOption("input_player1_" + opt)


def test_non_pedal_game_keeps_face_buttons(single_stick_controller):
    # Beach Spikers has no pedals: A/B keep their native role, no trigger remap.
    s = _configure(single_stick_controller, "beachspi.zip")
    assert s.hasOption("input_player1_a_btn")
    assert s.hasOption("input_player1_b_btn")
    assert not s.hasOption("input_player1_r2_btn")
    assert not s.hasOption("input_player1_l2_btn")
