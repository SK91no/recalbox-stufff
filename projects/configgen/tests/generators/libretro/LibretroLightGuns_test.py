"""Unit tests for libretroLightGun.__applyGunCon2Config().

The method is invoked only when guntype == "guncon2" AND the per-game
lookup succeeded AND the system is on a CRT (caller-side guard), so
all tests run on CRT-enabled emulators. The screen type drives the
behaviour we want to verify.
"""

from typing import List, Tuple
from unittest.mock import patch, mock_open

import pytest

from configgen.Emulator import Emulator, ExtraArguments
from configgen.crt.CRTTypes import CRTSuperRez
from configgen.generators.libretro.libretroLightGuns import libretroLightGun
from configgen.settings.keyValueSettings import keyValueSettings


def _make_gun(crtscreentype: str,
              crtresolutiontype: str = "auto",
              hdgame: bool = False) -> libretroLightGun:
    system = Emulator(name='dreamcast', videoMode='640x480',
                      ratio='auto', emulator='libretro', core='flycast')
    extra = ExtraArguments('640x480', "", "", "", "", "", "", "", "", "",
                           crtresolutiontype, crtscreentype, "",
                           hdgame=hdgame)
    system.configure(keyValueSettings(""), extra)
    ra = keyValueSettings("")
    core = keyValueSettings("")
    remap = keyValueSettings("")
    return libretroLightGun(system, '/tmp/dummy.chd', False, ra, core, remap)


def _apply(gun: libretroLightGun) -> Tuple[keyValueSettings, List[str]]:
    """Run __applyGunCon2Config with switchres.ini writes redirected
    to an in-memory list. Returns (retroarch config, switchres lines)."""
    written: List[str] = []
    m = mock_open()
    handle = m.return_value
    handle.write.side_effect = lambda s: written.append(s)
    with patch('builtins.open', m):
        gun._libretroLightGun__applyGunCon2Config()
    return gun._libretroLightGun__retroarchConfig, written


ALL_CRT_SCREENS = ["kHz15", "kHzMulti1525", "kHz31",
                   "kHzTriFreq", "kHzMulti1531"]
# Screens that can switch to a 31 kHz mode → we force the GunCon 2
# friendly modeline via switchres.
SWITCHABLE_TO_31KHZ_SCREENS = ["kHz31", "kHzTriFreq", "kHzMulti1531"]
# Pure 15 kHz screens — the gun works natively here (firmware mode
# "TV/15kHz") but we leave switchres alone since we can't reach the
# 31 kHz modeline anyway.
ONLY_15KHZ_SCREENS = ["kHz15", "kHzMulti1525"]


# ---------------------------------------------------------------------------
# Tweaks applied for every CRT screen type
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("crtscreentype", ALL_CRT_SCREENS)
@pytest.mark.parametrize("pre_video_driver", ["", "gl", '"gl"', "glcore", "vulkan"])
def test_lightgun_flash_shader_loaded(crtscreentype, pre_video_driver):
    # setupGunCon2 must force glcore + .slangp regardless of any driver posted upstream.
    gun = _make_gun(crtscreentype)
    if pre_video_driver:
        gun._libretroLightGun__retroarchConfig.setString("video_driver", pre_video_driver)
    config, _ = _apply(gun)

    assert config.getString("video_shader_enable", "") == "true"
    assert config.getString("video_shader", "") == \
           "/recalbox/share/shaders/lightgun-flash.slangp"
    assert config.getString("video_shader_dir", "") == \
           "/recalbox/share/shaders/"
    assert config.getString("video_driver", "").strip().strip('"') == "glcore"


@pytest.mark.parametrize("crtscreentype", ALL_CRT_SCREENS)
def test_video_settings_applied(crtscreentype):
    gun = _make_gun(crtscreentype)
    config, _ = _apply(gun)

    assert config.getString("video_max_swapchain_images", "") == "2"
    assert config.getString("video_hard_sync", "") == "false"
    assert config.getString("video_font_size", "") == "24"


@pytest.mark.parametrize("crtscreentype", ALL_CRT_SCREENS)
def test_run_ahead_and_preemptive_frames_disabled(crtscreentype):
    """Both must be false: they advance our FSM multiple times per
    video frame and break the trigger timing."""
    gun = _make_gun(crtscreentype)
    config, _ = _apply(gun)

    assert config.getString("run_ahead_enabled", "") == "false"
    assert config.getString("preemptive_frames_enable", "") == "false"


@pytest.mark.parametrize("crtscreentype", ALL_CRT_SCREENS)
def test_crt_super_rez_native(crtscreentype):
    """super-rez native is forced for every CRT screen type."""
    gun = _make_gun(crtscreentype)
    config, _ = _apply(gun)

    assert config.getString("crt_switch_resolution_super", "") == \
           CRTSuperRez.native


# ---------------------------------------------------------------------------
# switchres.ini behaviour by CRT screen type
# ---------------------------------------------------------------------------

def _assert_switchres_written(config: keyValueSettings, written: List[str]):
    blob = "".join(written)
    assert "monitor     custom" in blob
    assert "31450-31550" in blob
    assert "59-60" in blob
    assert "1.6, 4.7, 1.0" in blob
    assert "478, 480" in blob
    assert "interlace   0" in blob
    assert config.getString("crt_switch_resolution", "") != ""
    assert config.getString(
        "crt_switch_resolution_no_interlaced", "") == "1"


def _assert_switchres_NOT_written(config: keyValueSettings, written: List[str]):
    assert written == []
    assert config.getString("crt_switch_resolution", "") == ""
    assert config.getString(
        "crt_switch_resolution_no_interlaced", "") == ""


# kHz31 → modeline forcée quoi qu'il arrive (l'écran ne sait que 31 kHz).
@pytest.mark.parametrize("crtresolutiontype", ["auto", "480", "240"])
@pytest.mark.parametrize("hdgame", [False, True])
def test_switchres_ini_always_written_on_kHz31(crtresolutiontype, hdgame):
    gun = _make_gun("kHz31", crtresolutiontype=crtresolutiontype,
                    hdgame=hdgame)
    config, written = _apply(gun)
    _assert_switchres_written(config, written)


# Multi-sync (TriFreq / Multi1531) : modeline 31 kHz forcée UNIQUEMENT
# si on est en Auto + jeu HD. Dans tous les autres cas l'écran reste
# en 15 kHz natif et on ne touche pas à switchres.
@pytest.mark.parametrize("crtscreentype", ["kHzTriFreq", "kHzMulti1531"])
def test_switchres_ini_written_on_multisync_when_auto_and_HD(crtscreentype):
    gun = _make_gun(crtscreentype, crtresolutiontype="auto", hdgame=True)
    config, written = _apply(gun)
    _assert_switchres_written(config, written)


@pytest.mark.parametrize("crtscreentype", ["kHzTriFreq", "kHzMulti1531"])
def test_switchres_ini_NOT_written_on_multisync_when_auto_and_SD(crtscreentype):
    """SD game on Auto: switchres stays in 15 kHz native, no modeline
    constraint needed."""
    gun = _make_gun(crtscreentype, crtresolutiontype="auto", hdgame=False)
    config, written = _apply(gun)
    _assert_switchres_NOT_written(config, written)


@pytest.mark.parametrize("crtscreentype", ["kHzTriFreq", "kHzMulti1531"])
@pytest.mark.parametrize("hdgame", [False, True])
def test_switchres_ini_written_on_multisync_when_force_480(crtscreentype,
                                                            hdgame):
    """User explicitly forced 480p → we always end up on 31 kHz,
    constrain the modeline."""
    gun = _make_gun(crtscreentype, crtresolutiontype="480", hdgame=hdgame)
    config, written = _apply(gun)
    _assert_switchres_written(config, written)


@pytest.mark.parametrize("crtscreentype", ["kHzTriFreq", "kHzMulti1531"])
@pytest.mark.parametrize("crtresolutiontype", ["240", "480i"])
@pytest.mark.parametrize("hdgame", [False, True])
def test_switchres_ini_NOT_written_on_multisync_when_15khz_resolution(
        crtscreentype, crtresolutiontype, hdgame):
    """Force240 and 480i both run on 15 kHz — leave switchres alone."""
    gun = _make_gun(crtscreentype, crtresolutiontype=crtresolutiontype,
                    hdgame=hdgame)
    config, written = _apply(gun)
    _assert_switchres_NOT_written(config, written)


@pytest.mark.parametrize("crtscreentype", ONLY_15KHZ_SCREENS)
def test_switchres_ini_NOT_written_on_15khz_only_screens(crtscreentype):
    """Pure 15 kHz screens — the gun works fine in firmware
    "TV/15kHz" mode but we can't reach the 31 kHz modeline, so
    leave switchres untouched."""
    gun = _make_gun(crtscreentype)
    config, written = _apply(gun)
    _assert_switchres_NOT_written(config, written)


def test_every_gunless_port_is_disarmed_not_just_the_first_four():
    """A port left armed is polled as a lightgun by some cores (mame), which opens
    a calibration for a player who has no gun and no targets to shoot."""
    gun = _make_gun("kHz15")
    ra = gun._libretroLightGun__retroarchConfig
    # Two guns: players 1 and 2 keep their mouse, every other port is gunless.
    ra.setInt("input_player1_mouse_index", 0)
    ra.setInt("input_player2_mouse_index", 1)

    gun._libretroLightGun__cancelDevicesNotUsed()

    remap = gun._libretroLightGun__retroarchRemap
    assert remap.getInt("input_libretro_device_p1", -1) == -1, "player 1 keeps its device"
    assert remap.getInt("input_libretro_device_p2", -1) == -1, "player 2 keeps its device"
    for player in range(3, 17):
        assert remap.getInt("input_libretro_device_p{}".format(player), -1) == 0, \
            "port {} still armed".format(player)


@pytest.mark.parametrize("core, applies", [
    ("mame0258", True),          # the running core the lookup rewrites the node with
    ("mame0278", True),          # and every other version of the family
    ("mame2003_plus", False),    # has its own filter, must not catch this one
    ("fbneo", False),
])
def test_a_mame_filter_applies_to_the_versioned_cores(core, applies):
    """__lookupCore rewrites the node "mame" into the running core, so a verbatim
    comparison never matched and no mame core option was ever set."""
    match = libretroLightGun._libretroLightGun__coreMatchesFilter("mame", core)
    assert match == applies


def test_a_core_filter_still_matches_its_own_core_and_the_wildcard():
    assert libretroLightGun._libretroLightGun__coreMatchesFilter("snes9x", "snes9x")
    assert libretroLightGun._libretroLightGun__coreMatchesFilter("*", "whatever")
    assert not libretroLightGun._libretroLightGun__coreMatchesFilter("snes9x", "fbneo")
