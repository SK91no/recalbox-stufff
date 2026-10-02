#!/usr/bin/env python
import json

import pytest

from configgen.controllers.inputItem import InputItem
from configgen.remapper.Remapper import Remapper
from configgen.remapper.RemapProfileStore import RemapProfileStore
from configgen.remapper.RgpSysfs import RgpSysfs

PROFILES = {
    "naomi2": {
        "initd": {
            "match": ["initd"],
            "byController": [
                {"types": ["arcade6", "arcade8"], "remaps": {"l2": "b", "r2": "a"}}
            ],
            "jvs": {
                "byCabType": [
                    {"cabTypes": ["crazy_taxi"],
                     "remaps": {"up": "b", "down": "a", "west": "down"},
                     "combos": [{"modifier": "start", "trigger": "up", "emit": "west"}]},
                ]
            },
        }
    }
}


class FakeInput:
    def __init__(self, code, kind="button", value=1):
        self.Code = code
        self.Value = value
        self._kind = kind

    @property
    def IsButton(self): return self._kind == "button"
    @property
    def IsKey(self): return self._kind == "key"
    @property
    def IsAxis(self): return self._kind == "axis"
    @property
    def IsHat(self): return self._kind == "hat"


class FakeGamepad:
    def __init__(self, type_str): self.Type = type("T", (), {"value": type_str})()


class FakeController:
    def __init__(self, type_str, inputs):
        self.GamepadType = FakeGamepad(type_str)
        self._inputs = inputs  # {item_id: FakeInput}

    def HasInput(self, item): return item in self._inputs
    def Input(self, item): return self._inputs[item]


@pytest.fixture
def store(tmp_path):
    p = tmp_path / "profiles.json"
    p.write_text(json.dumps(PROFILES))
    return RemapProfileStore(str(p))


@pytest.fixture
def rgp(tmp_path):
    (tmp_path / "combos").write_text("")
    (tmp_path / "external_maps").write_text("EXISTING")
    return RgpSysfs(str(tmp_path)), tmp_path


def _pad(type_str="arcade6"):
    # start=modifier (BTN_START 315), up=hat vertical up, west=emit (BTN_WEST 308)
    return FakeController(type_str, {
        InputItem.ItemStart: FakeInput(315, "button"),
        InputItem.ItemUp:    FakeInput(0, "hat", value=-1),
        InputItem.ItemWest:  FakeInput(308, "button"),
    })


def test_base_map_when_no_profile(store, rgp):
    ai = Remapper(store, rgp[0])
    b = ai.button_map("naomi2", "unknown", _pad("arcade6"), cab_type="")
    assert b[InputItem.ItemEast] == "a" and b[InputItem.ItemStart] == "start"


def test_bycontroller_remaps_on_plain_stick(store, rgp):
    ai = Remapper(store, rgp[0])
    b = ai.button_map("naomi2", "initd", _pad("arcade6"), cab_type="joystick")
    assert b[InputItem.ItemL2] == "b" and b[InputItem.ItemR2] == "a"


def test_button_map_overlays_remaps_on_given_base(store, rgp):
    # ungate: remaps overlay onto the caller's base map (megadrive/dreamcast/
    # standard), preserving untouched keys — used for plain arcade sticks too.
    ai = Remapper(store, rgp[0])
    base = {InputItem.ItemEast: "custom_a", InputItem.ItemL2: "l2"}
    b = ai.button_map("naomi2", "initd", _pad("arcade6"), None, base=base)
    assert b[InputItem.ItemEast] == "custom_a"   # base key preserved where not remapped
    assert b[InputItem.ItemL2] == "b"         # byController remap (l2->b) overlaid


def test_rotate_applies_tate_face_button_rotation(rgp):
    from configgen.remapper.RemapProfileStore import RemapProfileStore
    ai = Remapper(RemapProfileStore(), rgp[0])  # shipped default carries `_rotate`
    b = ai.button_map("snes", "sf2", _pad("arcade6"), None, rotate=True)
    # TATE 90-degree rotation: a->b, b->y, x->a, y->x; d-pad untouched
    assert b[InputItem.ItemEast] == "b" and b[InputItem.ItemSouth] == "y"
    assert b[InputItem.ItemNorth] == "a" and b[InputItem.ItemWest] == "x"
    # not rotated -> identity face buttons
    n = ai.button_map("snes", "sf2", _pad("arcade6"), None, rotate=False)
    assert n[InputItem.ItemEast] == "a" and n[InputItem.ItemWest] == "y"


def test_rotate_axes_and_invert_resolve_from_shipped_profile(rgp):
    # TATE analog-stick rotation resolved to InputItems (90-degree swap + sign flip)
    ai = Remapper(RemapProfileStore(), rgp[0])
    axes = ai.rotate_axes()
    assert axes[InputItem.ItemJoy1Up] == "r_y" and axes[InputItem.ItemJoy1Left] == "r_x"
    assert axes[InputItem.ItemJoy2Up] == "l_x" and axes[InputItem.ItemJoy2Left] == "l_y"
    assert ai.rotate_invert_axes() == {InputItem.ItemJoy2Up}


def test_jvs_cab_remaps_applied(store, rgp):
    ai = Remapper(store, rgp[0])
    b = ai.button_map("naomi2", "initd", _pad("arcade6"), cab_type="crazy_taxi")
    assert b[InputItem.ItemUp] == "b" and b[InputItem.ItemDown] == "a" and b[InputItem.ItemWest] == "down"


def test_jvs_cab_combos_pushed_to_rgp(store, rgp):
    sysfs, path = rgp
    Remapper(store, sysfs).button_map("naomi2", "initd", _pad("arcade6"), cab_type="crazy_taxi")
    # start=315 (EV_KEY), up=hat -> ABS_HAT0Y (17) dir -1 (EV_ABS), emit west=308, hold
    assert (path / "combos").read_text() == "RecalboxJVS Gamepad #1:315:3:17:-1:308:hold"


def test_combo_naming_an_unknown_input_is_skipped_and_logged(capsys):
    # A profile naming an input the pad does not expose (typo, or a name left over from
    # the legacy a/b/x/y vocabulary) drops that combo alone: the others still go out.
    from configgen.remapper.RgpComboResolver import RgpComboResolver, EV_KEY
    resolver = RgpComboResolver({"start": (EV_KEY, 315, 1), "west": (EV_KEY, 308, 1)}, "TARGET")
    out = resolver.combos_to_sysfs([{"modifier": "start", "trigger": "y", "emit": "west"},
                                    {"modifier": "start", "trigger": "west", "emit": "west"}])
    assert out == "TARGET:315:1:308:1:308:hold"   # only the resolvable one survives
    assert "unknown input(s) ['y']" in capsys.readouterr().out


def test_no_combos_leaves_existing_untouched(store, rgp):
    # A controller with no matching combos must NOT wipe combos another controller
    # pushed: combos are one global R.G.P. state written once per controller, and a
    # plain gamepad on a JVS cab (cab_type None -> no byCabType match) comes through
    # here. Stale combos are cleared once via clear_combos(), not per controller.
    sysfs, path = rgp
    (path / "combos").write_text("PUSHED_BY_JVS")
    Remapper(store, sysfs).button_map("naomi2", "initd", _pad("arcade6"), cab_type="joystick")
    assert (path / "combos").read_text() == "PUSHED_BY_JVS"


def test_clear_combos_wipes_without_killing_watch(store, rgp):
    sysfs, path = rgp
    (path / "combos").write_text("STALE")
    Remapper(store, sysfs).clear_combos()
    assert (path / "combos").read_text() == "\n"  # wiped via newline (0-byte write is a no-op)


def test_second_non_jvs_controller_does_not_wipe_jvs_combos(store, rgp):
    # Regression: p1=JVS pushes the combo, p2=plain gamepad (cab_type None) must
    # leave it in place — otherwise a gamepad added to a JVS cab kills the combos.
    sysfs, path = rgp
    Remapper(store, sysfs).button_map("naomi2", "initd", _pad("arcade6"), cab_type="crazy_taxi")
    assert "RecalboxJVS Gamepad #1:315:3:17:-1:308:hold" in (path / "combos").read_text()
    Remapper(store, sysfs).button_map("naomi2", "initd", _pad("arcade6"), cab_type="joystick")
    assert "RecalboxJVS Gamepad #1:315:3:17:-1:308:hold" in (path / "combos").read_text()


def test_clear_only_touches_combos(store, rgp):
    sysfs, path = rgp
    (path / "combos").write_text("SOMETHING")
    Remapper(store, sysfs).clear()
    assert (path / "combos").read_text() == "\n"  # wiped via newline, not a 0-byte no-op
    assert (path / "external_maps").read_text() == "EXISTING"  # untouched


def test_rgp_absent_is_a_noop(store, tmp_path):
    # base path without combos file -> available() False, no crash
    ai = Remapper(store, RgpSysfs(str(tmp_path / "nope")))
    b = ai.button_map("naomi2", "initd", _pad("arcade6"), cab_type="crazy_taxi")
    assert b[InputItem.ItemUp] == "b"  # remaps still applied


# --- RAM-watch state switching (Super Mario Kart-style console racer) ---
SMK_PROFILES = {"snes": {"smk": {"match": ["smk"], "jvs": {"byCabType": [
    {"cabTypes": ["crazy_taxi"],
     "ramwatch": {"busAddr": "7e0148", "sysramOffset": "148", "menuValue": 0},
     "states": {
         "menu": {"combos": [{"trigger": "l2", "emit": "south"}]},
         "race": {"combos": [{"trigger": "up", "emit": "east"}, {"trigger": "l2", "emit": "west"}]}}}]}}}}


def _smk_pad():
    return FakeController("arcade6", {
        InputItem.ItemL2: FakeInput(2, "axis", value=1),
        InputItem.ItemUp: FakeInput(0, "hat", value=-1),
        InputItem.ItemEast:  FakeInput(304, "button"),
        InputItem.ItemWest:  FakeInput(308, "button"),
        InputItem.ItemSouth:  FakeInput(305, "button"),
    })


def test_ramwatch_thread_switches_menu_to_race(tmp_path, rgp, monkeypatch):
    import time
    import configgen.remapper.GameStateWatch as rw
    import configgen.remapper.Remapper as AImod

    seq = [0x00, 0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E] + [0x1E] * 200
    it = iter(seq)

    osd_msgs = []

    class FakeNC:
        def __init__(self, *a, **k): pass
        def read_byte(self, b, o):
            try: return next(it)
            except StopIteration: return 0x1E
        def show_osd(self, text): osd_msgs.append(text)
    monkeypatch.setattr(rw, "NetCmd", FakeNC)

    p = tmp_path / "smk.json"; p.write_text(json.dumps(SMK_PROFILES))
    store = RemapProfileStore(str(p))
    combos = rgp[1] / "combos"

    # Wait on exactly what is asserted below, never on something weaker: the loop
    # used to break on ":304:" alone and then assert ":304:hold", ":308:hold" and
    # two OSD notices — conditions the watch thread had not necessarily reached
    # yet, since apply_state writes the combos before it flashes the notice.
    def until(ready, seconds=2.0):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            if ready():
                return True
            time.sleep(0.02)
        return ready()

    Remapper(store, rgp[0]).button_map("snes", "smk", _smk_pad(), cab_type="crazy_taxi")
    assert until(lambda: ":304:hold" in combos.read_text()
                         and ":308:hold" in combos.read_text()), combos.read_text()
    txt = combos.read_text()
    assert ":304:hold" in txt and ":308:hold" in txt, txt      # race set applied
    assert ":0:" in txt                                        # mod-code 0 = plain remap
    # an OSD notice is flashed for both the initial menu state and the race switch
    assert until(lambda: "Recalbox JVS: switched controls to Menu Mode" in osd_msgs), osd_msgs
    assert until(lambda: "Recalbox JVS: switched controls to Race Mode" in osd_msgs), osd_msgs

    Remapper(store, rgp[0]).clear()
    assert AImod._watch_thread is None                          # thread stopped
    assert combos.read_text() == "\n"                          # combos cleared (newline wipe)
