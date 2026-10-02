import pytest

from configgen.controllers.GamepadInfo import GamepadInfo, GamepadType, Layout


def test_the_full_controller_chain_honors_the_layout_of_a_typeless_jamma_entry(mocker):
    # The regression hid at the real entry point: the file-loaded type is re-normalized
    # to the string "standard" before the layout-carrying construction
    import os
    import configgen.controllers.controller as controller_module
    mocker.patch.object(
        controller_module, "esInputs",
        os.path.join(os.path.dirname(__file__), "resources", "es_input_typeless_jamma.cfg"),
    )
    Controller = controller_module.Controller
    pads = Controller.LoadUserControllerConfigurations(
        p1index=0, p1guid="1800819562720000010a000000010000", p1name="JammaControllerP1",
        p1devicepath="/dev/input/event5", p1nbaxes=4, p1nbhats=0, p1nbbuttons=13,
        jammalayoutp1="line", system="fbneo",
    )
    assert pads[1].GamepadType.toRetroarchDevice("fbneo") == "1029"


def test_a_normalized_standard_type_with_a_neo_layout_is_inferred_arcade():
    info = GamepadInfo("standard", "line")
    assert info.Type == GamepadType.Arcade6
    assert info.toRetroarchDevice("neogeo") == "1029"


def test_a_jamma_layout_without_a_stick_type_still_reaches_the_core():
    # es_input.cfg entries written before the gamepadtype attribute carry no type,
    # yet the CLI layout only ever comes from Jamma/JVS arcade panels
    info = GamepadInfo("", "line")
    assert info.Type == GamepadType.Arcade6
    assert info.Layout == Layout.NeoLine
    assert info.toRetroarchDevice("neogeo") == "1029"


def test_every_neo_layout_survives_a_typeless_jamma_entry():
    assert GamepadInfo("", "square").toRetroarchDevice("neogeo") == "1285"
    assert GamepadInfo("", "neodefault").toRetroarchDevice("neogeo") == "1541"
    assert GamepadInfo("", "6btns").isSixBtnLayout()


def test_a_standard_layout_on_a_typeless_entry_keeps_the_plain_retropad():
    info = GamepadInfo("", "standard")
    assert info.toRetroarchDevice("neogeo") == "1"


def test_a_declared_non_arcade_stick_never_honors_a_layout():
    info = GamepadInfo("megadrive3", "line")
    assert info.Type == GamepadType.Megadrive3
    assert info.Layout == Layout.NotProvided
    assert info.toRetroarchDevice("neogeo") == "1"


def test_an_explicit_arcade6_stick_keeps_working_as_before():
    info = GamepadInfo("arcade6", "line")
    assert info.Type == GamepadType.Arcade6
    assert info.toRetroarchDevice("neogeo") == "1029"


def test_no_layout_and_no_type_stays_a_standard_pad():
    info = GamepadInfo("", "")
    assert info.Type == GamepadType.Standard
    assert info.Layout == Layout.NotProvided
    assert info.toRetroarchDevice("neogeo") == "1"
