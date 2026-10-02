import os
import logger
from installers.base.install import InstallBase
from installers.argonone.install import Install as InstallArgon
from filemanipulation import appendline, stripline


class Install(InstallArgon):

    ARGONV3_SOURCE_FOLDER = InstallBase.BASE_SOURCE_FOLDER + "argononev3/"

    def __init__(self):
        InstallArgon.__init__(self)

    def InstallHardware(self, case):
        logger.hardlog("Installing ArgonOneV3 Case hardware")
        if super().InstallHardware(case):
            appendline("dtparam=uart0=on", "/boot/recalbox-user-config.txt")
            appendline("dtparam=nvme", "/boot/recalbox-user-config.txt")
            appendline("dtparam=pciex1_gen=3", "/boot/recalbox-user-config.txt")
            stripline("enable_uart=1", "/boot/recalbox-user-config.txt")
            logger.hardlog("ArgonOneV3 Case hardware installed successfully!")
            return True
        logger.hardlog("ArgonOneV3: error editing /boot/recalbox-user-config.txt")
        return False

    def InstallSoftware(self, case):
        logger.hardlog("Installing ArgonOneV3 Case software")
        return super().InstallSoftware(case)

    def UninstallHardware(self, case):
        logger.hardlog("Uninstalling ArgonOneV3 Case hardware")
        if super().UninstallHardware(case):
            try:
                stripline("dtpparam=uart0=on", "/boot/recalbox-user-config.txt")
                stripline("dtparam=nvme", "/boot/recalbox-user-config.txt")
                stripline("dtparam=pciex1_gen=3", "/boot/recalbox-user-config.txt")
            except Exception as e:
                logger.hardlog("ArgonOneV3: Exception = {}".format(e))
            return True

    def UninstallSoftware(self, case):
        logger.hardlog("Uninstalling Retroflag Case software")
        super().UninstallSoftware(case)
        return ""

    def GetInstallScript(self, case):

        return None
