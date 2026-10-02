from unittest.mock import MagicMock
import pytest
import configgen.controllers.controller as controllersConfig
import os
import shutil


def fake_capabilities():
    return {
        1: [
            305,
            304,
            545,
            316,
            312,
            317,
            546,
            311,
            310,
            313,
            318,
            547,
            314,
            315,
            544,
            307,
            308,
        ]
    }


@pytest.fixture
def controller_configuration():
    shutil.copyfile(os.path.abspath(os.path.join(os.path.dirname(__file__), "resources/es_input.cfg.origin")),
            os.path.abspath(os.path.join(os.path.dirname(__file__), "resources/es_input.cfg")))

    controllersConfig.esInputs = os.path.abspath(os.path.join(os.path.dirname(__file__), "resources/es_input.cfg"))
    controllersConfig.getControllerIndex = MagicMock(
        side_effect=[0, -1, -1, -1, -1, -1, -1, -1, -1, -1]
    )
    controllersConfig.Controller._GetInputCapabilities = MagicMock(
        return_value=fake_capabilities()
    )
    uuid = "060000004c0500006802000000010000"
    return controllersConfig.Controller.LoadUserControllerConfigurations(
        p1index=0,
        p1guid=uuid,
        p1name="PLAYSTATION(R)3 Controller (00:48:E8:D1:63:25)",
        p1devicepath="/fakedev/input/event1",
        p1nbaxes=6,
        p1nbhats=1,
        p1nbbuttons=20,
        p2index=1,
        p2guid="",
        p2name="",
        p2devicepath="",
        p2nbaxes=-1,
        p2nbhats=-1,
        p2nbbuttons=-1,
        p3index=2,
        p3guid="",
        p3name="",
        p3devicepath="",
        p3nbaxes=-1,
        p3nbhats=-1,
        p3nbbuttons=-1,
        p4index=3,
        p4guid="",
        p4name="",
        p4devicepath="",
        p4nbaxes=-1,
        p4nbhats=-1,
        p4nbbuttons=-1,
        p5index=4,
        p5guid="",
        p5name="",
        p5devicepath="",
        p5nbaxes=-1,
        p5nbhats=-1,
        p5nbbuttons=-1,
        p6index=5,
        p6guid="",
        p6name="",
        p6devicepath="",
        p6nbaxes=-1,
        p6nbhats=-1,
        p6nbbuttons=-1,
        p7index=6,
        p7guid="",
        p7name="",
        p7devicepath="",
        p7nbaxes=-1,
        p7nbhats=-1,
        p7nbbuttons=-1,
        p8index=7,
        p8guid="",
        p8name="",
        p8devicepath="",
        p8nbaxes=-1,
        p8nbhats=-1,
        p8nbbuttons=-1,
        p9index=8,
        p9guid="",
        p9name="",
        p9devicepath="",
        p9nbaxes=-1,
        p9nbhats=-1,
        p9nbbuttons=-1,
        p10index=9,
        p10guid="",
        p10name="",
        p10devicepath="",
        p10nbaxes=-1,
        p10nbhats=-1,
        p10nbbuttons=-1,
    )
