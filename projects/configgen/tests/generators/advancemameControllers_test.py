#!/usr/bin/env python

import shutil
import unittest
import configgen.generators.advancemame.advMameControllers as advMameControllers
import configgen.controllers.controller as controllersConfig
import pytest


@pytest.mark.usefixtures("controller_configuration")
def test_generateCombo(controller_configuration):
    controllerConfig = advMameControllers.getControllerConfig(controller_configuration[1])
    print("controllerConfig: {}".format(controllerConfig))
    import pprint
    pprint.pprint(controllerConfig)
    assert(controllerConfig == {
        'input_map[coin1]': 'joystick_button[0,8]',
        'input_map[p1_button10]': 'joystick_button[0,12]',
        'input_map[p1_button1]': 'joystick_button[0,0]',
        'input_map[p1_button2]': 'joystick_button[0,1]',
        'input_map[p1_button3]': 'joystick_button[0,3]',
        'input_map[p1_button4]': 'joystick_button[0,2]',
        'input_map[p1_button5]': 'joystick_button[0,4]',
        'input_map[p1_button6]': 'joystick_button[0,5]',
        'input_map[p1_button7]': 'joystick_button[0,6]',
        'input_map[p1_button9]': 'joystick_button[0,11]',
        'input_map[p1_doubleleft_down]': 'joystick_digital[0,0,1,0]',
        'input_map[p1_doubleleft_left]': 'joystick_digital[0,0,0,1]',
        'input_map[p1_doubleleft_right]': 'joystick_digital[0,0,0,0]',
        'input_map[p1_doubleleft_up]': 'joystick_digital[0,0,1,1]',
        'input_map[p1_doubleright_down]': 'joystick_digital[0,0,3,0]',
        'input_map[p1_doubleright_left]': 'joystick_digital[0,0,2,1]',
        'input_map[p1_doubleright_right]': 'joystick_digital[0,0,2,0]',
        'input_map[p1_doubleright_up]': 'joystick_digital[0,0,3,1]',
        'input_map[p1_down]': 'joystick_button[0,14] or joystick_digital[0,0,1,0]',
        'input_map[p1_left]': 'joystick_digital[0,0,0,1] or joystick_button[0,15]',
        'input_map[p1_right]': 'joystick_digital[0,0,0,0] or joystick_button[0,16]',
        'input_map[p1_up]': 'joystick_digital[0,0,1,1] or joystick_button[0,13]',
        'input_map[start1]': 'joystick_button[0,9]',
        'input_map[ui_cancel]': 'joystick_button[0,10] joystick_button[0,9]',
        'input_map[ui_configure]': 'joystick_button[0,7]',
        'input_map[ui_down]': 'joystick_button[0,14] or joystick_digital[0,0,1,0]',
        'input_map[ui_left]': 'joystick_digital[0,0,0,1] or joystick_button[0,15]',
        'input_map[ui_load_state]': 'joystick_button[0,10] joystick_button[0,2]',
        'input_map[ui_mode_pred]': 'joystick_button[0,10] joystick_button[0,6]',
        'input_map[ui_right]': 'joystick_digital[0,0,0,0] or joystick_button[0,16]',
        'input_map[ui_save_state]': 'joystick_button[0,10] joystick_button[0,3]',
        'input_map[ui_select]': 'joystick_button[0,0]',
        'input_map[ui_soft_reset]': 'joystick_button[0,10] joystick_button[0,1]',
        'input_map[ui_turbo]': 'joystick_button[0,10] joystick_button[0,16]',
        'input_map[ui_up]': 'joystick_digital[0,0,1,1] or joystick_button[0,13]'
    })
