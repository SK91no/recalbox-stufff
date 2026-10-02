import os
import logger
from installers.base.install import InstallBase
from filemanipulation import appendline, stripline


class Install(InstallBase):

    BASE_SOURCE_FOLDER = InstallBase.BASE_SOURCE_FOLDER + "argonone/"
    S99 = "/etc/init.d/S99argononed"

    def __init__(self):
        InstallBase.__init__(self)

    def InstallHardware(self, case):
        logger.hardlog("Installing ArgonOne Case hardware")
        try:
            os.system("mount -o remount,rw /")
            os.system("mount -o remount,rw /boot")

            for line in ["i2c-bcm2708", "i2c-dev"]:
                appendline(line, "/etc/modules.conf")

            for line in ["dtoverlay=", "dtparam=i2c_arm=on", "enable_uart=1"]:
                appendline(line, "/boot/recalbox-user-config.txt")

            if os.system('cp {}assets/S99argononed {}'.format(self.BASE_SOURCE_FOLDER, self.S99)) != 0:
                logger.hardlog("ArgonOne: error copying S99argononed")
                return False
        except Exception as e:
            logger.hardlog("ArgonOne: Exception = {}".format(e))
            return False

        logger.hardlog("ArgonOne Case hardware installed successfully!")
        return True

    def InstallSoftware(self, case):
        logger.hardlog("Installing ArgonOne Case software")
        return case

    def UninstallHardware(self, case):
        logger.hardlog("Uninstalling ArgonOne Case hardware")
        try:
            os.system("mount -o remount,rw /")
            os.system("mount -o remount,rw /boot")
            for line in ["i2c-bcm2708", "i2c-dev"]:
                stripline(line, "/etc/modules.conf")

            for line in ["dtoverlay=", "dtparam=i2c_arm=on", "enable_uart=1"]:
                stripline(line, "/boot/recalbox-user-config.txt")

            if os.system("rm -f {}".format(self.S99)) != 0:
                logger.hardlog("ArgonOne: Error removing S99argononed")
        except Exception as e:
            logger.hardlog("ArgonOne: Exception = {}".format(e))
        return True

    def UninstallSoftware(self, case):
        logger.hardlog("Uninstalling ArgonOne Case software")
        return ""

    def GetInstallScript(self, case):

        return None
