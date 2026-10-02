import os
import logger
import shutil
import time
from installers.base.install import InstallBase
from filemanipulation import appendline, stripline
from settings import keyValueSettings


class Install(InstallBase):

    BASE_SOURCE_FOLDER = InstallBase.BASE_SOURCE_FOLDER + "steamdeck/"
    RECALBOX_CONF = "/recalbox/share/system/recalbox.conf"

    def __init__(self):
        InstallBase.__init__(self)

    def InstallHardware(self, case):
        logger.hardlog("Installing firmware for Steam Deck")
        try:
            os.system("mount -o remount,rw /")
            os.system("/usr/bin/rsync -av /usr/share/recalbox/firmware/steamdeck/. /lib/firmware/")
            logger.hardlog("Steam Deck Case hardware installed successfully!")
        except Exception as e:
            logger.hardlog(f"SteamDeck : Exception in InstallHardware(): {e}")
        finally:
            os.system("mount -o remount,ro /")

        # no need to reboot
        return False

    def InstallSoftware(self, case):
        logger.hardlog("Installing Steam Deck Case software")
        try:
            os.system("mount -o remount,rw /")

            recalboxConf = keyValueSettings(self.RECALBOX_CONF, False)
            recalboxConf.loadFile()
            # these must match default settings from volumed
            recalboxConf.setOption("audio.volume", "50")
            recalboxConf.setOption("emulationstation.brightness", "6")
            recalboxConf.setOption("system.volumed.hotkey", "Steam Deck:316")
            logger.hardlog("SteamDeck: set default brightness level and audio volume")

            appendline("default-sample-rate = 48000", "/etc/pulse/daemon.conf")
            logger.hardlog("SteamDeck: set pulseaudio default sample rate to 48000")
            recalboxConf.saveFile()
        except Exception as e:
            logger.hardlog(f"SteamDeck : Exception = {e}")
            return ""

        # stop acpid, no need to shutdown when power key is pressed
        try:
            os.system("/etc/init.d/S02acpid stop")
            shutil.move('/etc/init.d/S02acpid', '/etc/init.d/s02acpid')
            logger.hardlog("SteamDeck: removed acpid")
        except Exception as e:
            logger.hardlog(f"SteamDeck : non-fatal exception = {e}")

        try:
            os.system("/etc/init.d/S06volumed stop")
            os.system("/etc/init.d/S06pulseaudio stop")
            time.sleep(1) # laziness way to wait for pulseaudio to stop
            os.system("/etc/init.d/S06pulseaudio start")
            os.system("/etc/init.d/S06volumed start")
            logger.hardlog("SteamDeck: restarted pulseaudio and volumed")
        except Exception as e:
            logger.hardlog(f"SteamDeck : non-fatal exception = {e}")

        logger.hardlog("Steam Deck Case software installed successfully!")
        return case

    def UninstallHardware(self, case):
        logger.hardlog("Uninstalling Steam Deck Case hardware")
        # no need to reboot
        return False

    def UninstallSoftware(self, case):
        logger.hardlog("Uninstalling Steam Deck Case software")
        try:
            os.system("mount -o remount,rw /")
            # Uninstall /boot/recalbox-user-config.txt
            shutil.move('/etc/init.d/s02acpid', '/etc/init.d/S02acpid')
            logger.hardlog("SteamDeck: set back S02acpid")
            stripline("default-sample-rate = 48000", "/etc/pulse/daemon.conf")
            logger.hardlog("SteamDeck: removed 48000 pulseaudio default sample rate")

        except Exception as e:
            logger.hardlog(f"SteamDeck : Exception = {e}")
            return False

        finally:
            os.system("mount -o remount,ro /")

        return ""

    def GetInstallScript(self, case):

        return None
