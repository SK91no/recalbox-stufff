#!/usr/bin/env python
import json
import struct

import pytest

from configgen.remapper.RemapProfileStore import RemapProfileStore


def _rom_bytes(title, slot=0xFFC0, size=0x80000):
    data = bytearray(size)
    data[slot:slot + 21] = title.encode("ascii").ljust(21, b" ")[:21]
    struct.pack_into("<HH", data, slot + 0x1C, 0x1234 ^ 0xFFFF, 0x1234)
    return bytes(data)

PROFILES = {
    "naomi": {
        "default": {
            "byController": [
                {"types": ["arcade6", "arcade8"], "remaps": {"l2": "b", "r2": "a"}}
            ]
        },
        "18wheelr": {
            "match": ["18wheelr", "18wheelro", "18wheelrt"],
            "byController": [
                {"types": ["arcade6", "arcade8"], "remaps": {"b": "r2", "a": "l2"}}
            ],
            "jvs": {
                "byCabType": [
                    {"cabTypes": ["18_wheeler"]},
                    {"cabTypes": ["crazy_taxi", "jambo_safari"],
                     "combos": [
                         {"modifier": "start", "trigger": "up",   "dir": -1, "emit": "a", "mode": "hold"},
                         {"modifier": "start", "trigger": "down", "dir":  1, "emit": "y", "mode": "hold"},
                     ]},
                ]
            },
        },
    }
}


@pytest.fixture
def store(tmp_path):
    p = tmp_path / "profiles.json"
    p.write_text(json.dumps(PROFILES))
    return RemapProfileStore(str(p))


def test_exact_rom_matches_game_controller_layer(store):
    r = store.resolve("naomi", "18wheelr", "arcade6")
    assert r["remaps"] == {"b": "r2", "a": "l2"}


def test_rom_variant_in_match_list(store):
    r = store.resolve("naomi", "18wheelro", "arcade6")
    assert r["remaps"] == {"b": "r2", "a": "l2"}


def test_rom_prefix_match(store):
    # A rom starting with a listed pattern matches (region/revision suffixes).
    r = store.resolve("naomi", "18wheelrxyz", "arcade6")
    assert r["remaps"] == {"b": "r2", "a": "l2"}


def test_unknown_rom_falls_back_to_default(store):
    r = store.resolve("naomi", "crzytaxi", "arcade8")
    assert r["remaps"] == {"l2": "b", "r2": "a"}


def test_jvs_cab_specific_wins_over_controller(store):
    r = store.resolve("naomi", "18wheelr", "arcade6", cab_type="crazy_taxi")
    assert "combos" in r and len(r["combos"]) == 2
    assert "remaps" not in r  # the cab entry replaces the controller layer


def test_jvs_native_cab_returns_empty_rules(store):
    # 18_wheeler cab has the physical buttons -> native, no rule
    r = store.resolve("naomi", "18wheelr", "arcade6", cab_type="18_wheeler")
    assert r.get("combos") is None and r.get("remaps") is None


def test_jvs_unlisted_cab_falls_back_to_controller(store):
    # a plain joystick JVS cab has no dedicated cabType entry -> controller layer
    r = store.resolve("naomi", "18wheelr", "arcade6", cab_type="joystick")
    assert r["remaps"] == {"b": "r2", "a": "l2"}


def test_controller_type_not_covered_returns_empty(store):
    # arcade4 is not listed for 18wheelr, and a matched game replaces default
    r = store.resolve("naomi", "18wheelr", "arcade4")
    assert r == {}


def test_unknown_system_returns_empty(store):
    assert store.resolve("megadrive", "sonic", "arcade6") == {}


@pytest.fixture
def header_store(tmp_path):
    profiles = {"snes": {
        "smk": {
            "match": ["smk"],
            "matchHeader": ["SUPER MARIO KART"],
            "jvs": {"byCabType": [{"cabTypes": ["crazy_taxi"], "remaps": {"r2": "b"}}]},
        }
    }}
    p = tmp_path / "profiles.json"
    p.write_text(json.dumps(profiles))
    return RemapProfileStore(str(p))


def test_matchheader_identifies_game_regardless_of_filename(header_store, tmp_path):
    rom = tmp_path / "some-weird-dump-name.sfc"
    rom.write_bytes(_rom_bytes("SUPER MARIO KART"))
    r = header_store.resolve("snes", "some-weird-dump-name", "arcade6",
                             cab_type="crazy_taxi", rom_path=str(rom))
    assert r["remaps"] == {"r2": "b"}


def test_matchheader_rejects_a_different_game(header_store, tmp_path):
    rom = tmp_path / "smw.sfc"
    rom.write_bytes(_rom_bytes("SUPER MARIO WORLD"))
    r = header_store.resolve("snes", "smw", "arcade6",
                             cab_type="crazy_taxi", rom_path=str(rom))
    assert r == {}


def test_matchheader_falls_back_to_filename_without_rom_path(header_store):
    # No rom_path (or unreadable file) -> the filename `match` still works.
    r = header_store.resolve("snes", "smk", "arcade6", cab_type="crazy_taxi")
    assert r["remaps"] == {"r2": "b"}


def test_shipped_default_json_is_valid():
    # Guards against a syntax error in the committed data file.
    store = RemapProfileStore()
    r = store.resolve("naomi", "18wheelr", "arcade6", cab_type="crazy_taxi")
    assert len(r.get("combos", [])) == 2


def test_naomi_generic_games_get_no_arcade_profile_remap():
    # Naomi/naomi2/atomiswave pedal and non-pedal games without a dedicated entry
    # get NO remap from the arcade profiles: racing-on-a-stick (analog steering +
    # driving on face buttons) is handled imperatively by the flycast layer
    # (isFlycastPedalGame / route_stick_to_analog), not duplicated here.
    store = RemapProfileStore()
    for rom in ("clubkrt", "maxspeed", "cvsnaomi"):
        assert store.resolve("naomi", rom, "arcade6").get("remaps") is None, rom
        assert store.resolve("naomi", rom, "arcade6", cab_type="race").get("remaps") is None, rom
