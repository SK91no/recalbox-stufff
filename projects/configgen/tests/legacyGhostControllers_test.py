from unittest.mock import MagicMock

import configgen.controllers.controller as controllersConfig
from configgen.controllers.controller import Controller

GUID = "03008269380700008483000011010000"

LEGACY = """	<inputConfig type="joystick" deviceGUID="{guid}" deviceNbAxes="6" deviceNbHats="1" deviceNbButtons="14">
		<input name="a" type="button" id="2" value="1" code="306" />
		<input name="start" type="button" id="9" value="1" code="313" />
		<input name="hotkey" type="button" id="12" value="1" code="316" />
	</inputConfig>
"""

MODERN = """	<inputConfig type="joystick" deviceName="Mad Catz FightStick TE S+ PS4" deviceGUID="{guid}" deviceNbAxes="6" deviceNbHats="1" deviceNbButtons="14" gamepadtype="arcade8" version="2">
		<input name="a" type="button" id="2" value="1" code="306" />
		<input name="start" type="button" id="9" value="1" code="313" />
		<input name="hotkey" type="button" id="11" value="1" code="315" />
	</inputConfig>
"""

MODERN_TWIN = MODERN.replace('code="315"', 'code="314"').replace('deviceName="Mad', 'deviceName="Twin Mad')


def _players(tmp_path, monkeypatch, *entries: str):
    esInput = tmp_path / "es_input.cfg"
    esInput.write_text('<?xml version="1.0"?>\n<inputList>\n'
                       + "".join(entry.format(guid=GUID) for entry in entries)
                       + "</inputList>\n")
    monkeypatch.setattr(controllersConfig, "esInputs", str(esInput))
    monkeypatch.setattr(controllersConfig, "getControllerIndex", MagicMock(return_value=0))
    monkeypatch.setattr(Controller, "_GetInputCapabilities", MagicMock(return_value={}))
    return Controller.LoadUserControllerConfigurations(
        p1index=0, p1guid=GUID, p1name="Mad Catz FightStick TE S+ PS4",
        p1devicepath="/fakedev/input/event1", p1nbaxes=6, p1nbhats=1, p1nbbuttons=14,
        system="mame")


def test_a_reconfigured_pad_wins_over_the_nameless_fossil_an_old_recalbox_left(tmp_path, monkeypatch):
    # The frontend no longer matches the legacy entry: the player sees the modern
    # hotkey while the game answered to the fossil's one — hotkey+start half dead
    players = _players(tmp_path, monkeypatch, LEGACY, MODERN)
    assert players[1].Hotkey.Code == 315


def test_the_fossil_never_wins_whatever_side_of_the_file_it_sits_on(tmp_path, monkeypatch):
    players = _players(tmp_path, monkeypatch, MODERN, LEGACY)
    assert players[1].Hotkey.Code == 315


def test_between_two_modern_entries_the_last_still_wins(tmp_path, monkeypatch):
    # The historical rule between entries of the same generation is untouched
    players = _players(tmp_path, monkeypatch, MODERN, MODERN_TWIN)
    assert players[1].Hotkey.Code == 314


def test_a_pad_with_only_its_legacy_entry_still_gets_it(tmp_path, monkeypatch):
    # Better an old mapping than none at all: replacement needs a modern entry to exist
    players = _players(tmp_path, monkeypatch, LEGACY)
    assert players[1].Hotkey.Code == 316
