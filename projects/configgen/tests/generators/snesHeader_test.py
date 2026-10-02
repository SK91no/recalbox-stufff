#!/usr/bin/env python
import struct
import zipfile

import pytest

from configgen.remapper.SnesHeader import SnesHeader


def make_rom(title, slot=0x7FC0, size=0x80000, smc=False, valid_cksum=True):
    """Synthesize a rom image with `title` at the given header `slot`."""
    data = bytearray(size)
    data[slot:slot + 21] = title.encode("ascii").ljust(21, b" ")[:21]
    if valid_cksum:
        cksum = 0x1234
        struct.pack_into("<HH", data, slot + 0x1C, cksum ^ 0xFFFF, cksum)
    return (b"\x00" * 512 + bytes(data)) if smc else bytes(data)


# -- pure title_from_bytes ----------------------------------------------------

def test_lorom_title():
    assert SnesHeader.title_from_bytes(make_rom("SUPER MARIO KART", 0x7FC0)) == "SUPER MARIO KART"


def test_hirom_title():
    # Real Super Mario Kart carries its title in the 0xFFC0 slot.
    assert SnesHeader.title_from_bytes(make_rom("SUPER MARIO KART", 0xFFC0)) == "SUPER MARIO KART"


def test_exhirom_title():
    rom = make_rom("BIG GAME", 0x40FFC0, size=0x410000)
    assert SnesHeader.title_from_bytes(rom) == "BIG GAME"


def test_smc_copier_header_is_stripped():
    rom = make_rom("SUPER MARIO KART", 0x7FC0, smc=True)
    assert len(rom) % 1024 == 512
    assert SnesHeader.title_from_bytes(rom) == "SUPER MARIO KART"


def test_trailing_padding_collapsed_and_uppercased():
    # lowercase + padding in the rom, normalized on the way out
    assert SnesHeader.title_from_bytes(make_rom("street  fighter", 0x7FC0)) == "STREET FIGHTER"


def test_valid_checksum_slot_wins_over_printable_garbage():
    # LoROM slot holds printable-but-wrong text with a bad checksum; HiROM holds
    # the real header with a valid checksum -> HiROM must win.
    data = bytearray(make_rom("REAL GAME", 0xFFC0, valid_cksum=True))
    data[0x7FC0:0x7FC0 + 21] = b"GARBAGE TEXT HERE!!!!"
    struct.pack_into("<HH", data, 0x7FC0 + 0x1C, 0x0000, 0x0000)  # invalid: comp^cksum != 0xFFFF
    assert SnesHeader.title_from_bytes(bytes(data)) == "REAL GAME"


def test_empty_slots_return_none():
    assert SnesHeader.title_from_bytes(bytes(0x80000)) is None


def test_none_and_short_data_return_none():
    assert SnesHeader.title_from_bytes(None) is None
    assert SnesHeader.title_from_bytes(b"") is None
    assert SnesHeader.title_from_bytes(b"\x00" * 100) is None


# -- matches ------------------------------------------------------------------

def test_matches_normalizes_both_sides():
    assert SnesHeader.matches("SUPER MARIO KART", ["Super Mario Kart"])
    assert SnesHeader.matches("super mario kart ", ["SUPER  MARIO  KART"])


def test_matches_rejects_other_game_and_empty():
    assert not SnesHeader.matches("SUPER MARIO WORLD", ["SUPER MARIO KART"])
    assert not SnesHeader.matches(None, ["SUPER MARIO KART"])
    assert not SnesHeader.matches("", ["SUPER MARIO KART"])


# -- file/archive wrapper -----------------------------------------------------

def test_title_from_raw_file(tmp_path):
    p = tmp_path / "Super Mario Kart (USA).sfc"
    p.write_bytes(make_rom("SUPER MARIO KART", 0xFFC0))
    assert SnesHeader.title_from_file(str(p)) == "SUPER MARIO KART"


def test_title_from_zip(tmp_path):
    z = tmp_path / "smk.zip"
    with zipfile.ZipFile(z, "w") as zf:
        zf.writestr("Super Mario Kart (USA).sfc", make_rom("SUPER MARIO KART", 0xFFC0))
    assert SnesHeader.title_from_file(str(z)) == "SUPER MARIO KART"


def test_missing_file_returns_none(tmp_path):
    assert SnesHeader.title_from_file(str(tmp_path / "nope.sfc")) is None
