#!/usr/bin/env python
from configgen.remapper.RgpComboResolver import RgpComboResolver, EV_KEY, EV_ABS

TARGET = "RecalboxJVS Gamepad #1"
# name -> (ev_type, ev_code, trig_val); trig_val is the axis direction for ABS.
NAME_TO_EV = {
    "start": (EV_KEY, 315, 1),   # BTN_START
    "up":    (EV_ABS, 1, -1),    # ABS_Y up = negative
    "down":  (EV_ABS, 1, 1),     # ABS_Y down = positive
    "a":     (EV_KEY, 304, 1),   # BTN_SOUTH
    "y":     (EV_KEY, 308, 1),   # BTN_NORTH
    "b":     (EV_KEY, 305, 1),   # BTN_EAST
}


def _r():
    return RgpComboResolver(NAME_TO_EV, TARGET)


def test_abs_trigger_direction_comes_from_the_name_table():
    s = _r().combos_to_sysfs(
        [{"modifier": "start", "trigger": "down", "emit": "y", "mode": "hold"}])
    # down -> ABS_Y (+1); no "dir" in the combo
    assert s == "RecalboxJVS Gamepad #1:315:3:1:1:308:hold"


def test_up_carries_negative_direction():
    s = _r().combos_to_sysfs(
        [{"modifier": "start", "trigger": "up", "emit": "a", "mode": "hold"}])
    assert s == "RecalboxJVS Gamepad #1:315:3:1:-1:304:hold"


def test_key_trigger_uses_one():
    s = _r().combos_to_sysfs(
        [{"modifier": "start", "trigger": "b", "emit": "y"}])
    # trigger 'b' is EV_KEY -> trig-val 1; mode is always "hold"
    assert s == "RecalboxJVS Gamepad #1:315:1:305:1:308:hold"


def test_multiple_combos_are_semicolon_joined():
    s = _r().combos_to_sysfs([
        {"modifier": "start", "trigger": "up",   "emit": "a", "mode": "hold"},
        {"modifier": "start", "trigger": "down", "emit": "y", "mode": "hold"},
    ])
    assert s == ("RecalboxJVS Gamepad #1:315:3:1:-1:304:hold;"
                 "RecalboxJVS Gamepad #1:315:3:1:1:308:hold")


def test_names_are_case_insensitive():
    s = _r().combos_to_sysfs(
        [{"modifier": "Start", "trigger": "Down", "emit": "Y", "mode": "hold"}])
    assert s == "RecalboxJVS Gamepad #1:315:3:1:1:308:hold"


def test_empty_list_is_empty_string():
    assert _r().combos_to_sysfs([]) == ""
