#!/usr/bin/env python
import pytest
from configgen.Emulator import Emulator
from configgen.generators.julius.juliusGenerator import JuliusGenerator
from configgen.settings.keyValueSettings import keyValueSettings

from tests.generators.FakeArguments import Arguments

@pytest.fixture
def emulator():
	return JuliusGenerator()

@pytest.fixture
def system():
	return Emulator(name='julius', videoMode='1920x1080', ratio='auto', emulator='julius', core='julius')

@pytest.mark.usefixtures("controller_configuration")
def test_simple_generate_julius(emulator, system, controller_configuration):
	command = emulator.generate(system, controller_configuration, keyValueSettings("", False), Arguments('/path/to/test'))
	assert command.videomode == '1920x1080'
	assert command.array == ['/usr/bin/julius', '/path/to/test']
