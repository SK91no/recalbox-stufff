from typing import Optional, Dict
from unittest.mock import mock_open
import pytest

from configgen.Emulator import Emulator, ExtraArguments
from configgen.generators.libretro.libretroGenerator import LibretroGenerator
from configgen.settings.keyValueSettings import keyValueSettings


def configure(system: Emulator, widescreen=False, hd=False, rotation=0):
    opts = keyValueSettings("")
    opts.setBool("global.widescreenmode", widescreen)
    opts.setBool("global.hdmode", hd)
    system.configure(opts, ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", "", rotation=rotation))
    return system


def test_given_widescreen_mode_select_genesiswide_on_megadrive(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(
        configure(
            Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='picodrive'),
            widescreen=True)
    )
    assert newCore == "genesisplusgxwide"
    assert coreConfig["genesis_plus_gx_wide_h40_extra_columns"] == '10'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(
        configure(
            Emulator(name='megadrive', videoMode='1920x1080', ratio='auto', emulator='libretro', core='genesisplusgxwide'),
            widescreen=False)
    )
    assert coreConfig["genesis_plus_gx_wide_h40_extra_columns"] == '0'

def test_given_widescreen_mode_select_bsneshd_on_snes(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='snes9x'),
        widescreen=True))
    assert newCore == "bsneshd"
    assert coreConfig["bsnes_mode7_wsMode"] == '"all"'
    assert coreConfig["bsnes_mode7_scale"] == '"1x"'
    assert coreConfig["bsnes_mode7_wsobj"] == '"unsafe"'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='snes', videoMode='1920x1080', ratio='auto', emulator='libretro', core='bsneshd'),
        widescreen=False))
    assert newCore == "bsneshd"
    assert coreConfig["bsnes_mode7_wsMode"] == '"none"'
    assert coreConfig["bsnes_mode7_scale"] == '"1x"'
    assert coreConfig["bsnes_mode7_wsobj"] == '"safe"'

def test_given_widescreen_mode_select_and_configure_flycast(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(
        configure(
            Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='whatever'),
            widescreen=True)
    )
    assert newCore == "flycast"
    assert coreConfig["reicast_widescreen_hack"] == '"enabled"'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(
        configure(
            Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='flycast'),
            widescreen=False)
    )
    assert coreConfig["reicast_widescreen_hack"] == '"disabled"'


def test_given_hd_mode_select_and_configure_flycast(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro', core='whatever'),
        hd=True))
    assert newCore == "flycast"
    assert coreConfig["reicast_internal_resolution"] == '"1024x768"'
    assert coreConfig["reicast_cable_type"] == '"VGA (RGB)"'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='dreamcast', videoMode='1920x1080', ratio='auto', emulator='libretro',core='flycast'),
        hd=False))
    assert coreConfig["reicast_internal_resolution"] == '"640x480"'
    assert coreConfig["reicast_cable_type"] == '"VGA (RGB)"'

def test_given_hd_mode_select_and_configure_pcsx_rearmed(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='whatever'),
        hd=True))
    assert newCore == "pcsx_rearmed"
    assert coreConfig["pcsx_rearmed_neon_enhancement_enable"] == '"enabled"'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro', core='pcsx_rearmed'),
        hd=False))
    assert coreConfig["pcsx_rearmed_neon_enhancement_enable"] == '"disabled"'

def test_given_hd_mode_select_and_configure_yabasanshiro(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='saturn', videoMode='1920x1080', ratio='auto', emulator='libretro', core='whatever'),
        hd=True))
    assert newCore == "yabasanshiro"
    assert coreConfig["yabasanshiro_resolution_mode"] == '"2x"'

    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='saturn', videoMode='1920x1080', ratio='auto', emulator='libretro', core='yabasanshiro'),
        hd=False))
    assert coreConfig["yabasanshiro_resolution_mode"] == '"1x"'

def test_given_widescreen_and_tate_disable(mocker):
    libretroConfig, coreConfig, newCore = LibretroGenerator().createHDWidescreenConfig(configure(
        Emulator(name='saturn', videoMode='1920x1080', ratio='auto', emulator='libretro', core='whatever'),
        widescreen=True, rotation=1))
    assert newCore == "whatever"
