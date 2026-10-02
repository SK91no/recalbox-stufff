import logger
from installers.steamdeck.install import Install as InstallSteamDeck


class Install(InstallSteamDeck):

    def __init__(self):
        super().__init__()

    def InstallHardware(self, case):
        logger.hardlog("Installing Steam Deck LCD Case hardware")
        ret = super().InstallHardware(case)
        logger.hardlog("Steam Deck LCD Case hardware installed successfully!")
        return ret

    def InstallSoftware(self, case):
        logger.hardlog("Installing Steam Deck LCD Case software")
        case = super().InstallSoftware(case)
        logger.hardlog("Steam Deck LCD Case software installed successfully!")
        return case

    def UninstallHardware(self, case):
        logger.hardlog("Uninstalling Steam Deck LCD Case hardware")
        return super().UninstallHardware(case)

    def UninstallSoftware(self, case):
        logger.hardlog("Uninstalling Steam Deck LCD Case software")
        return super().UninstallSoftware(case)

    def GetInstallScript(self, case):

        return super().GetInstallScript(case)
