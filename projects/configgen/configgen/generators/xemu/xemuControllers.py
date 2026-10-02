#!/usr/bin/env python3
from configgen.controllers.controller import ControllerPerPlayer


class XemuControllers:

    def __init__(self, controllers: ControllerPerPlayer):
        self.controllers = controllers

    def generateGameControllerConfig(self) -> list[str]:
        gameControllerConfig = []
        for index, controller in self.controllers.items():
            gameControllerConfig.append(controller.generateSDLGameDBLine())

        return gameControllerConfig
