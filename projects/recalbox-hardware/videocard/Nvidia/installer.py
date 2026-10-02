#!/usr/bin/env python
import glob
import logging
import os
import re
import sys

from Nvidia.NvidiaInstaller import GpuError, NvidiaInstaller
import Utils.PciUtils

BLACKLIST_FILE = "/etc/modprobe.d/blacklist.conf"
NVIDIA_DRM_FILE = "/etc/modprobe.d/nvidia-drm.conf"
XORG_CONF_FILE = "/etc/X11/xorg.conf"

RC_OK = 0
RC_GPU_ERROR = 1
RC_RUNTIME_ERROR = 2
RC_GENERAL_ERROR = 3
RC_NO_NVIDIA_GPU = 4

log = logging.getLogger(__name__)


def blacklist_nouveau():
    """
    add nouveau driver to blacklist
    this will disallow automatic loading of the driver
    """
    with open(BLACKLIST_FILE, "w", encoding="utf8") as file:
        file.write("blacklist nouveau")


def enable_nvidia_drm_modeset():
    """
    add nvidia-drm modeset parameter to 1
    this will enable DRM modesetting
    """
    with open(NVIDIA_DRM_FILE, "w", encoding="utf8") as file:
        file.write("options nvidia-drm modeset=1")


def is_nouveau_blacklisted() -> bool:
    """
    test if nouveau module is blacklisted
    """
    try:
        with open(BLACKLIST_FILE, encoding="utf8") as file:
            for line in file.readlines():
                if line == "blacklist nouveau":
                    return True
        raise Exception("not blacklisted")
    except Exception as e:
        return False


def run_depmod():
    """
    execute depmod -a
    """
    os.system("depmod -a")


def unprotect_root():
    """
    remount root file system read/write
    """
    os.system("mount -o remount,rw /")


def protect_root():
    """
    remount root file system read only
    """
    os.system("mount -o remount,ro /")


# return the card id that was used to display boot sequence
def get_bootvga_card():
    bootvga = None
    cards = glob.glob("/sys/class/drm/card[0-9]")
    for card in cards:
        bootvga_path = os.path.join(card, "device/boot_vga")
        try:
            with open(bootvga_path, "r") as file:
                t = file.read()
                file.close()
                if t:
                    if int(t.strip()) > 0:
                        bootvga = card
                        break
        except:
            pass
    return bootvga


# return card vendor id from card id
def get_card_vendor(card):
    vendor_path = os.path.join(card, "device/vendor")
    with open(vendor_path, "r") as file:
        t = file.read()
        file.close()
        return t.strip()
    return ""


# if booted from Intel or Amd iGPU, return True
# else False
def has_booted_on_igpu():
    card = get_bootvga_card()
    if card is not None:
        vendor = get_card_vendor(card)
        log.info("Boot vga card is %s from vendor %s", card, vendor)
        if vendor in ["0x8086", "0x1002"]:
            return True
    return False


# effectively configure nvidia prime offloading
def configure_prime():
    if os.path.isfile(XORG_CONF_FILE):
        os.unlink(XORG_CONF_FILE)
    os.system("/usr/bin/nvidia-xconfig --prime")


# test if prime is needed and enable it
def eventualy_configure_prime():
    if has_booted_on_igpu():
        log.info("Enable prime offloading")
        configure_prime()


def main(args) -> int:

    if not args.force and is_nouveau_blacklisted():
        # already blacklisted
        if not args.early:
            log.info("Already installed, regenerating xorg.conf")
            unprotect_root()
            eventualy_configure_prime()
            protect_root()
        else:
            log.info("Already installed and early detected, doing nothing")
        return RC_OK

    installer = NvidiaInstaller(args.version)
    if args.search_path:
        installer.set_search_path(args.search_path)

    if len(Utils.PciUtils.find_nvidia_controller()) == 0:
        log.info("No Nvidia controller found, exiting...")
        return RC_NO_NVIDIA_GPU

    installed = False
    rc = -1
    for pci_card in Utils.PciUtils.find_nvidia_controller():
        pci_ids = iter(pci_card)
        vendor = next(pci_ids)
        product = next(pci_ids)
        log.info("Trying to install Nvidia controller %s:%s", vendor, product)

        try:
            unprotect_root()
            installer.install(f"0x{product}")
            # this part is reach if installer.install() found a supported nvidia controller
            blacklist_nouveau()
            run_depmod()
            enable_nvidia_drm_modeset()
            installed = True
        except GpuError as exc:
            log.error("GPU error(%s): %s", exc.product_id, exc.message)
            rc = RC_GPU_ERROR
        except RuntimeError as exc:
            log.error("Can't install Nvidia drivers: %s", exc)
            rc = RC_RUNTIME_ERROR
        except FileNotFoundError as exc:
            log.error("General error: %s", exc)
            rc = RC_GENERAL_ERROR
        finally:
            protect_root()

    if installed:
        rc = RC_OK
    return rc
