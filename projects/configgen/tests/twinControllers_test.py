#!/usr/bin/env python
import os
import unittest
from unittest.mock import MagicMock

import configgen.controllers.controller as controllersConfig

TWIN_GUID = "0300457e790000000600000010010000"
PORT_LEFT = "platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0"
PORT_RIGHT = "platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.3:1.0"


def _resource(name: str) -> str:
    return os.path.abspath(os.path.join(os.path.dirname(__file__), "resources", name))


class TestTwinControllers(unittest.TestCase):
    """Two arcade encoders sharing guid and capabilities, wired differently."""

    def setUp(self):
        controllersConfig.esInputs = _resource("es_input.cfg.twins")
        controllersConfig.getControllerIndex = MagicMock(return_value=0)
        controllersConfig.Controller._GetInputCapabilities = MagicMock(return_value={})

    # mame is the one system exempted from the arcade6 reshuffle, which permutes these codes
    SYSTEM = {"system": "mame"}

    @staticmethod
    def _twin(index: int, physicalPath: str) -> dict:
        return {
            "p{}index".format(index): index - 1,
            "p{}guid".format(index): TWIN_GUID,
            "p{}name".format(index): "DragonRise Inc.   Generic   USB  Joystick",
            "p{}devicepath".format(index): "/fakedev/input/event{}".format(index),
            "p{}physicalpath".format(index): physicalPath,
            "p{}nbaxes".format(index): 4,
            "p{}nbhats".format(index): 1,
            "p{}nbbuttons".format(index): 12,
        }

    def test_each_twin_gets_the_configuration_of_the_port_it_is_plugged_into(self):
        players = controllersConfig.Controller.LoadUserControllerConfigurations(
            **self._twin(1, PORT_LEFT), **self._twin(2, PORT_RIGHT), **self.SYSTEM
        )

        self.assertEqual(2, len(players))
        self.assertEqual(295, players[1].North.Code)
        self.assertEqual(297, players[2].North.Code)
        self.assertEqual(289, players[1].East.Code)
        self.assertEqual(296, players[2].East.Code)

    def test_swapping_the_cables_swaps_the_configurations_too(self):
        players = controllersConfig.Controller.LoadUserControllerConfigurations(
            **self._twin(1, PORT_RIGHT), **self._twin(2, PORT_LEFT), **self.SYSTEM
        )

        self.assertEqual(297, players[1].North.Code)
        self.assertEqual(295, players[2].North.Code)

    def test_a_twin_on_an_unknown_port_still_gets_a_configuration(self):
        players = controllersConfig.Controller.LoadUserControllerConfigurations(
            **self._twin(1, "platform-fd500000.pcie-pci-0000:01:00.0-usb-0:9.9:1.0"), **self.SYSTEM
        )

        self.assertEqual(1, len(players))
        self.assertEqual(295, players[1].North.Code)

    def test_a_pad_reaching_configgen_without_a_port_falls_back_on_the_shared_entry(self):
        players = controllersConfig.Controller.LoadUserControllerConfigurations(
            **self._twin(1, ""), **self.SYSTEM
        )

        self.assertEqual(1, len(players))
        self.assertEqual(295, players[1].North.Code)


class TestLegacyControllerFile(unittest.TestCase):
    """A configuration file written before ports were recorded must behave as before."""

    def setUp(self):
        controllersConfig.esInputs = _resource("es_input.cfg.origin")
        controllersConfig.getControllerIndex = MagicMock(return_value=0)
        controllersConfig.Controller._GetInputCapabilities = MagicMock(return_value={})

    def test_a_port_is_ignored_when_no_entry_declares_one(self):
        uuid = "060000004c0500006802000000010000"
        common = dict(p1index=0, p1guid=uuid,
                      p1name="PLAYSTATION(R)3 Controller (00:48:E8:D1:63:25)",
                      p1devicepath="/fakedev/input/event1",
                      p1nbaxes=6, p1nbhats=1, p1nbbuttons=20)

        without = controllersConfig.Controller.LoadUserControllerConfigurations(**common)
        withPort = controllersConfig.Controller.LoadUserControllerConfigurations(
            p1physicalpath="platform-whatever-usb-0:1.1:1.0", **common
        )

        self.assertEqual(1, len(without))
        self.assertEqual(len(without), len(withPort))
        self.assertEqual(without[1].North.Code, withPort[1].North.Code)


if __name__ == "__main__":
    unittest.main()
