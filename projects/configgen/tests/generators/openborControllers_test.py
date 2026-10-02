#!/usr/bin/env python

from pprint import pprint
import shutil
import configgen.controllers.controller as controllersConfig
from configgen.generators.openbor.openborControllers import OpenborControllers
from configgen.settings.keyValueSettings import keyValueSettings
import pytest


controllersConfig.esInputs = "tests/resources/es_input.cfg"


@pytest.fixture
def openbor_controller_init(self):
    shutil.copyfile(
        "tests/resources/es_input.cfg.origin", "tests/resources/es_input.cfg"
    )
    self.controllers = (
        controllersConfig.Controller.LoadControllerConfigurationFromFile()
    )


@pytest.fixture
def settings():
    return keyValueSettings("/tmp/dumb")


@pytest.mark.usefixtures("controller_configuration")
def test_openborControllers(controller_configuration, settings):
    controllers = OpenborControllers(controller_configuration)
    controllers.addControllers(settings)
    assert (
        settings.getInt("JoystickButton-P0-MOVEDOWN", "undefined-not-good") == 615
        and settings.getInt("JoystickButton-P0-MOVEUP", "undefined-not-good") == 614
        and settings.getInt("JoystickButton-P0-MOVELEFT", "undefined-not-good") == 616
        and settings.getInt("JoystickButton-P0-MOVERIGHT", "undefined-not-good") == 617
        and settings.getInt("JoystickButton-P0-ATTACK", "undefined-not-good") == 602
        and settings.getInt("JoystickButton-P0-ATTACK2", "undefined-not-good") == 601
        and settings.getInt("JoystickButton-P0-ATTACK3", "undefined-not-good") == 603
        and settings.getInt("JoystickButton-P0-ATTACK4", "undefined-not-good") == 604
        and settings.getInt("JoystickButton-P0-HOTKEY", "undefined-not-good") == 611
        and settings.getInt("JoystickButton-P0-JUMP", "undefined-not-good") == 605
    )
