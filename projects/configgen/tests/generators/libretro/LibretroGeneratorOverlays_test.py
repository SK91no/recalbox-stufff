import os
from typing import Optional, Dict
from unittest.mock import mock_open

import pytest

from configgen.Emulator import Emulator, ExtraArguments
import configgen.controllers.controller as controllersConfig
from configgen.crt.CRTConfigParser import CRTConfigParser
from configgen.crt.CRTTypes import CRTSystemMode, CRTArcadeMode
from configgen.crt.CRTModeOffsetter import CRTModeOffsetter
from configgen.generators.libretro.crt.LibretroConfigCRT import LibretroConfigCRT
from configgen.generators.libretro.libretroControllers import LibretroControllers
from configgen.generators.libretro.libretroGenerator import LibretroGenerator
from configgen.settings.keyValueSettings import keyValueSettings
from tests.Givens import givenThoseFiles


def create_system(name, resolution, ratio, overlaysEnabled:bool, sgb=False, widescreen=False):

    recalbox_options = keyValueSettings("")
    recalbox_options.setBool("global.recalboxoverlays", overlaysEnabled)
    recalbox_options.setBool("global.widescreenmode", widescreen)
    emulator = Emulator(name=name, videoMode=resolution, ratio=ratio, emulator='libretro', core='snes9x')
    emulator.configure(recalbox_options,
                   ExtraArguments(resolution, "", "", "", "", "", "", "", "", "", "", "", "",
                                  rotation=0, sgb=sgb))
    return emulator


def test_given_recalbox_overlays_and_1080p_resolution_then_set_overlays_for_the_system(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})

    snes = create_system("snes", "1920x1080", "auto", True)
    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == ['/recalbox/share_init/overlays/snes/snes.cfg']

def test_given_recalbox_overlays_disabled_set_do_not_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})

    snes = create_system("snes", "1920x1080", "auto", False)
    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == []


def test_given_recalbox_overlays_and_800x600_resolution_then_do_not_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})
    snes = create_system("snes", "800x600", "auto", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == []


def test_given_recalbox_overlays_and_1080p_plus_forced_16_9_ratio_then_do_not_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})
    snes = create_system("snes", "1920x1080", "16/9", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == []

def test_given_recalbox_overlays_and_1080p_plus_forced_16_10_ratio_then_do_not_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})
    snes = create_system("snes", "1920x1080", "16/10", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == []

def test_given_sgb_then_do_not_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/gb/gb.cfg": ""})
    snes = create_system("gb", "1920x1080", "auto", True, sgb=True)
    configs = LibretroGenerator.processOverlays(snes, "Tetris.zip")
    assert configs == []

def test_given_widescreen_then_do_not_set_overlays(mocker):
    # But snes should not
    givenThoseFiles(mocker, {"/recalbox/share_init/overlays/snes/snes.cfg": ""})
    snes = create_system("snes", "1920x1080", "auto", True, widescreen=True)
    configs = LibretroGenerator.processOverlays(snes, "SMK.zip")
    assert configs == []


# User overlays
def test_given_user_for_system_overlays_then_do_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share/overlays/snes/.overlay.cfg": ""})
    snes = create_system("snes", "1920x1080", "auto", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == ["/recalbox/share/overlays/snes/.overlay.cfg"]


def test_given_user_global_overlays_then_do_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share/overlays/.overlay.cfg": ""})
    snes = create_system("snes", "1920x1080", "auto", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == ["/recalbox/share/overlays/.overlay.cfg"]


def test_given_user_game_overlays_then_do_set_overlays(mocker):
    givenThoseFiles(mocker, {"/recalbox/share/overlays/snes/SuperMarioWorld.zip.cfg": ""})
    snes = create_system("snes", "1920x1080", "auto", True)

    configs = LibretroGenerator.processOverlays(snes, "SuperMarioWorld.zip")
    assert configs == ["/recalbox/share/overlays/snes/SuperMarioWorld.zip.cfg"]
