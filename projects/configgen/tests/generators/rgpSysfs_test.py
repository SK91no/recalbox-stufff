#!/usr/bin/env python
from configgen.remapper.RgpSysfs import RgpSysfs


def test_available_reflects_combos_attr(tmp_path):
    rgp = RgpSysfs(str(tmp_path))
    assert rgp.available() is False
    (tmp_path / "combos").write_text("")
    assert rgp.available() is True


def test_push_and_clear(tmp_path):
    (tmp_path / "combos").write_text("")
    (tmp_path / "external_maps").write_text("")
    rgp = RgpSysfs(str(tmp_path))

    assert rgp.push_combos("PadA:315:3:1:1:308:hold") is True
    assert (tmp_path / "combos").read_text() == "PadA:315:3:1:1:308:hold"

    rgp.clear()
    # clear must write a newline, not "": a 0-byte write never reaches the kernel
    # combos_store, so an empty write would leave the previous combos in place.
    assert (tmp_path / "combos").read_text() == "\n"
    assert (tmp_path / "external_maps").read_text() == "\n"


def test_empty_push_is_a_newline_not_a_zero_byte_write(tmp_path):
    # Regression: menu state / clear must send a non-empty payload so the module
    # actually wipes; "" (0 bytes) is a no-op at the sysfs layer.
    (tmp_path / "combos").write_text("PadA:315:3:1:1:308:hold")
    rgp = RgpSysfs(str(tmp_path))
    assert rgp.push_combos("") is True
    assert (tmp_path / "combos").read_text() == "\n"


def test_push_returns_false_when_attr_missing(tmp_path):
    # No 'combos' file created -> write to a non-existent path under a dir is OK,
    # but writing under a missing parent dir fails gracefully.
    rgp = RgpSysfs(str(tmp_path / "nope"))
    assert rgp.push_combos("x") is False
