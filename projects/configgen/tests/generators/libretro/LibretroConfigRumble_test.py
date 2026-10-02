import os

import pytest

from configgen.Emulator import Emulator, ExtraArguments
import configgen.controllers.controller as controllersConfig
from configgen.generators.libretro.libretroControllers import LibretroControllers
from configgen.settings.keyValueSettings import keyValueSettings

controllersConfig.esInputs = os.path.abspath(os.path.join(os.path.dirname(__file__), "../../resources/es_input.cfg"))

@pytest.mark.usefixtures("controller_configuration")
def test_given_rumble_on_playstation_then_set_core_option(controller_configuration):
    psx = Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro',
                        core='pcsx_rearmed')
    coreSettings = keyValueSettings("")
    lrControllers = LibretroControllers(psx, keyValueSettings(""), keyValueSettings(""), coreSettings, keyValueSettings(""), controller_configuration, False, "")

    recalboxConf = keyValueSettings("")
    recalboxConf.setBool("global.rumble", True)
    extraArgs = ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", "")
    psx.configure(recalboxConf, extraArgs)

    config = lrControllers.fillControllersConfiguration()
    assert coreSettings.getString('pcsx_rearmed_vibration', "") == '"enabled"'

@pytest.mark.usefixtures("controller_configuration")
def test_given_rumble_disabled_on_playstation_then_set_core_option_to_disabled(controller_configuration):
    psx = Emulator(name='psx', videoMode='1920x1080', ratio='auto', emulator='libretro',
                        core='pcsx_rearmed')
    coreSettings = keyValueSettings("")
    lrControllers = LibretroControllers(psx, keyValueSettings(""), keyValueSettings(""), coreSettings, keyValueSettings(""), controller_configuration, False, "")

    recalboxConf = keyValueSettings("")
    recalboxConf.setBool("global.rumble", False)
    extraArgs = ExtraArguments('1920x1080', "", "", "", "", "", "", "", "", "", "", "", "")
    psx.configure(recalboxConf, extraArgs)

    config = lrControllers.fillControllersConfiguration()
    assert coreSettings.getString('pcsx_rearmed_vibration', "") == '"disabled"'
